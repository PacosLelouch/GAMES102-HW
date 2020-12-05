// Author: LiJiayu (JerryLi)
// Mail: lijiayu83@gmail.com (fullike@163.com)
// Copyright 2019. All Rights Reserved.

#include "ArteriesActor.h"
#include "Arteries.h"
#include "Async/Async.h"
#include "Kismet/GameplayStatics.h"
uint32 FArteriesRunnable::TlsSlot = 0;
AArteriesActor* FArteriesRunnable::GetThreadOwner()
{
	FArteriesRunnable* Runnable = (FArteriesRunnable*)FPlatformTLS::GetTlsValue(TlsSlot);
	return Runnable ? Runnable->Owner : NULL;
}
ULuaContext* FArteriesRunnable::GetLuaContext()
{
	FArteriesRunnable* Runnable = (FArteriesRunnable*)FPlatformTLS::GetTlsValue(TlsSlot);
	return Runnable ? Runnable->LuaContext : NULL;
}
void FArteriesRunnable::Build(const TCHAR* Name)
{
	LuaContext = NewObject<ULuaContext>();
	LuaContext->AddToRoot();
	LuaContext->Init();
	FRunnableThread::Create(this, Name, 0, TPri_Normal);
}
bool FArteriesRunnable::Init()
{
	FPlatformTLS::SetTlsValue(TlsSlot, this);
	return true;
}
uint32 FArteriesBuilder::Run()
{
	Owner->BuilderThreadMain();
	return 0;
}
void FArteriesBuilder::Exit()
{
	LuaContext->RemoveFromRoot();
	Owner->Builder = NULL;
	delete this;
}
void FArteriesTaskQueue::AddTask(TFunction<void()>&& Task)
{
	FScopeLock Lock(&Critical);
	Tasks.Enqueue(Task);
}
bool FArteriesTaskQueue::GetTask(TFunction<void()>& Task)
{
	FScopeLock Lock(&Critical);
	return Tasks.Dequeue(Task);
}
void FArteriesTaskQueue::Increment(const FName& GroupName)
{
	FScopeLock Lock(&Critical);
	if (Counter.Contains(GroupName))
		Counter[GroupName]++;
	else
		Counter.Add(GroupName, 1);
}
int FArteriesTaskQueue::GetCount(const FName& GroupName)
{
	FScopeLock Lock(&Critical);
	int* Count = Counter.Find(GroupName);
	return Count ? *Count : 0;
}
void FArteriesTaskQueue::WaitForCount(const FName& GroupName, int Count, float Timeout)
{
	float SleepTime = 0.01f;
	int MaxLoop = FMath::RoundToInt(Timeout / SleepTime);
	for (int i = 0; i < MaxLoop; i++)
	{
		if (GetCount(GroupName) >= Count)
			break;
		FPlatformProcess::Sleep(SleepTime);
	}
}
uint32 FArteriesTaskQueue::Run()
{
	while (bRunning)
	{
		TFunction<void()> Task;
		if (GetTask(Task))
			Task();
		else
			FPlatformProcess::Sleep(0.01f);
	}
	return 0;
}
void FArteriesTaskQueue::Exit()
{
	LuaContext->RemoveFromRoot();
	Owner->TaskQueue = NULL;
	delete this;
}



AArteriesActor::AArteriesActor(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer), FinalObject(NULL), Builder(NULL), TaskQueue(NULL)
{
#if WITH_EDITORONLY_DATA
	DisplayNode = NULL;
#endif
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
}
#if WITH_EDITOR
void AArteriesActor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	if (GetMutableDefault<UArteriesSettings>()->bBuildWhenPropertiesChanged &&!HasAnyFlags(RF_ClassDefaultObject))
	{
		if (PropertyChangedEvent.MemberProperty && PropertyChangedEvent.MemberProperty->GetOuter() == GetClass())
			Build(true);
	}
	AActor::PostEditChangeProperty(PropertyChangedEvent);
}
void AArteriesActor::OnResultSubmitted(UEdGraphNode* Node, UArteriesObject* Object)
{
	FScopeLock Lock(&Critical);
	if (Object)
	{
		if (UArteriesObject** Old = Results.Find(Node))
			RemoveRef(*Old);
		AddRef(Object);
		Results.Add(Node, Object);
	}
}
void AArteriesActor::RemoveAllResults()
{
	for (auto It = Results.CreateIterator(); It; ++It)
		RemoveRef(It->Value);
	check(Objects.Num() == 0);
	Results.Empty();
}
void AArteriesActor::SetDisplayNode(const UEdGraphNode* Node)
{
	CleanAttachedActors();
	DisplayNode = Node;
	if (UArteriesObject* Object = CurrentObject())
		BuildObject(Object);
}
UArteriesObject* AArteriesActor::CurrentObject()
{
	UArteriesObject** Object = Results.Find(DisplayNode);
	return Object ? *Object : FinalObject;
}
#endif
void AArteriesActor::Increment(FName GroupName)
{
	TaskQueue->Increment(GroupName);
}
void AArteriesActor::WaitForCount(FName GroupName, int Count, float Timeout)
{
	TaskQueue->WaitForCount(GroupName,Count,Timeout);
}
void AArteriesActor::CleanOwnedActors()
{
	for (int i = Children.Num() - 1; i >= 0; i--)
		GetWorld()->DestroyActor(Children[i]);
}
void AArteriesActor::CleanAttachedActors()
{
	const TArray<USceneComponent*>& AttachChildren = RootComponent->GetAttachChildren();
	for (int i = AttachChildren.Num() - 1; i >= 0; i--)
	{
		AActor* ComponentOwner = AttachChildren[i]->GetOwner();
		if (ComponentOwner != this)
			GetWorld()->DestroyActor(ComponentOwner);
	}
}
void AArteriesActor::BuildObject(UArteriesObject* Object)
{
	Object->Build(Sections);
	CreateComponents(Object);
}
void AArteriesActor::CreateComponents(UArteriesObject* Object)
{
	TArray<UProceduralMeshComponent*> ProceduralMeshComponents;
	GetComponents<UProceduralMeshComponent>(ProceduralMeshComponents);
	for (UProceduralMeshComponent* Component : ProceduralMeshComponents)
	{
		Component->DestroyComponent();
	}
	TArray<UHierarchicalInstancedStaticMeshComponent*> InstancedStaticMeshComponents;
	GetComponents<UHierarchicalInstancedStaticMeshComponent>(InstancedStaticMeshComponents);
	for (UHierarchicalInstancedStaticMeshComponent* Component : InstancedStaticMeshComponents)
	{
		Component->DestroyComponent();
	}
	UProceduralMeshComponent* ProcComponent = CreateProceduralMeshComponent();
	int Index = 0;
	for (auto It = Sections.CreateIterator(); It; ++It)
	{
		It->Value.bEnableCollision = true;
		ProcComponent->SetMaterial(Index, It->Key);
		ProcComponent->SetProcMeshSection(Index, It->Value);
		Index++;
	}
	Sections.Empty();
	for (auto It = Object->InstancesMap.CreateIterator(); It; ++It)
	{
		UHierarchicalInstancedStaticMeshComponent* InstComponent = CreateInstancedStaticMeshComponent();
		InstComponent->bDisableCollision = 1;
		InstComponent->SetStaticMesh(It->Key);
		for (FTransform& Transform : It->Value.Transforms)
			InstComponent->AddInstance(Transform);
	}
	RegisterAllComponents();
#if WITH_EDITORONLY_DATA
	if (OnEditorBuildCompleted.IsBound())
		OnEditorBuildCompleted.Broadcast(Object);
#endif
}
void AArteriesActor::Build(bool bForceRebuild)
{
	if (!bForceRebuild && FinalObject)
		return;
	if (!IsRunning())
	{
		//-stompmalloc
		if (IsInGameThread())
		{
			ForEachObjectWithOuter(GetTransientPackage(), [](UObject* Obj)
			{
				if (Obj->IsA<UArteriesObject>() || Obj->IsA<ULuaContext>())
					Obj->ClearInternalFlags(EInternalObjectFlags::Async);
			});
			CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS);
		}
		FinalObject = NULL;
		CleanAttachedActors();
		Builder = new FArteriesBuilder(this);
		TaskQueue = new FArteriesTaskQueue(this);
		Builder->Build(TEXT("ArteriesBuilder"));
		TaskQueue->Build(TEXT("ArteriesTaskQueue"));
	}
}
void AArteriesActor::BuilderThreadMain()
{
#if WITH_EDITOR
	RemoveAllResults();
#endif
	OnBuild();
	AArteriesActor* Parent = Cast<AArteriesActor>(GetOwner());
	// If parent forget to wait for the result, Parent->TaskQueue may be NULL
	if (Parent && Parent->TaskQueue)
	{
		Parent->TaskQueue->AddTask([this]()
		{
			if (OnBuildCompleted.IsBound())
				OnBuildCompleted.Broadcast(FinalObject, GetTransform());
		});
	}
	AsyncTask(ENamedThreads::GameThread, [this]()
	{
		CleanOwnedActors();
	});
	// Owned actors don't need further build
	if (!Parent)
	{
		if (FinalObject)
		{
			FinalObject->Build(Sections);
			AsyncTask(ENamedThreads::GameThread, [this]()
			{
				CreateComponents(FinalObject);
			});
		}
	}
	TaskQueue->Close();
}
void AArteriesActor::Destroyed()
{
	CleanAttachedActors();
	AActor::Destroyed();
}