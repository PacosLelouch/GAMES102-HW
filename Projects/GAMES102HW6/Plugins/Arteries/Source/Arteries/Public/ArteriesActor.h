// Author: LiJiayu (JerryLi)
// Mail: lijiayu83@gmail.com (fullike@163.com)
// Copyright 2019. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"
#include "HAL/Runnable.h"
#include "HAL/RunnableThread.h"
#include "GameFramework/Actor.h"
#include "ProceduralMeshComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "ArteriesObject.h"
#include "LuaContext.h"
#include "ArteriesActor.generated.h"
class AArteriesActor;
class FArteriesRunnable : public FRunnable
{
public:
	static uint32 TlsSlot;
	static AArteriesActor* GetThreadOwner();
	static ULuaContext* GetLuaContext();
	FArteriesRunnable(AArteriesActor* InActor) :Owner(InActor) {}
	void Build(const TCHAR* Name);
	virtual bool Init();
	AArteriesActor* Owner;
	ULuaContext* LuaContext;
//	FRunnableThread* Thread;
	FCriticalSection Critical;
};
class FArteriesBuilder : public FArteriesRunnable
{
public:
	FArteriesBuilder(AArteriesActor* InActor) :FArteriesRunnable(InActor) {}
	virtual uint32 Run();
	virtual void Exit();
};
class FArteriesTaskQueue : public FArteriesRunnable
{
public:
	FArteriesTaskQueue(AArteriesActor* InActor) :FArteriesRunnable(InActor), bRunning(true) {}
	void AddTask(TFunction<void()>&& Task);
	bool GetTask(TFunction<void()>& Task);
	void Close() { bRunning = false; }
	void Increment(const FName& GroupName);
	int GetCount(const FName& GroupName);
	void WaitForCount(const FName& GroupName, int Count, float Timeout);
	virtual uint32 Run();
	virtual void Exit();
	TQueue<TFunction<void()>> Tasks;
	TMap<FName, int> Counter;
	bool bRunning;
};
UCLASS(BlueprintType, Blueprintable)
class ARTERIES_API AArteriesActor :public AActor
{
	GENERATED_UCLASS_BODY()
public:
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnBuildCompleted, UArteriesObject*, Object, FTransform, Transform);
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnEditorBuildCompleted, UArteriesObject*);
	UProceduralMeshComponent* CreateProceduralMeshComponent()
	{
		UProceduralMeshComponent* Component = NewObject<UProceduralMeshComponent>(this);
		Component->Mobility = RootComponent->Mobility;
		Component->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);
		AddInstanceComponent(Component);
		return Component;
	}
	UHierarchicalInstancedStaticMeshComponent* CreateInstancedStaticMeshComponent()
	{
		UHierarchicalInstancedStaticMeshComponent* Component = NewObject<UHierarchicalInstancedStaticMeshComponent>(this);
		Component->Mobility = RootComponent->Mobility;
		Component->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);
		AddInstanceComponent(Component);
		return Component;
	}
	//Don't reset properties in main thread because they are being used in worker thread.
	virtual void RerunConstructionScripts() {}
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	void OnResultSubmitted(UEdGraphNode* Node, UArteriesObject* Object);
	void RemoveAllResults();
	void SetDisplayNode(const UEdGraphNode* Node);
	void AddRef(UArteriesObject* Object)
	{
		if (Objects.Contains(Object))
			Objects[Object]++;
		else
		{
			Object->AddToRoot();
			Objects.Add(Object, 1);
		}
	}
	void RemoveRef(UArteriesObject* Object)
	{
		if (--Objects[Object] == 0)
		{
			Object->RemoveFromRoot();
			Objects.Remove(Object);
		}
	}
	UArteriesObject* CurrentObject();
#else
	UArteriesObject* CurrentObject() { return FinalObject; }
#endif
	UFUNCTION(BlueprintCallable, Category = Arteries, meta = (AdvancedDisplay = 1))
	void Increment(FName GroupName);
	UFUNCTION(BlueprintCallable, Category = Arteries, meta = (AdvancedDisplay = 1))
	void WaitForCount(FName GroupName, int Count, float Timeout = 10.f);
	bool IsRunning() { return Builder || TaskQueue; }
	// Owned actors are sub actors that we want to process (merge,copy/move etc) their build results.
	// Owned actors are never attached to the world or any root of other actors. They are cleaned up when their build results are merged to any UArteriesObject
	void CleanOwnedActors();
	// Attached actors are sub actors which will be attached to root of their parent actor.
	// We don't care about when their building is finished. They are destroyed only when their parent is destroyed or rebuild.
	void CleanAttachedActors();
	void BuildObject(UArteriesObject* Object);
	void CreateComponents(UArteriesObject* Object);
	UFUNCTION(BlueprintCallable, Category = Arteries, meta = (AdvancedDisplay = 1))
	void Build(bool bForceRebuild);
	void BuilderThreadMain();
	virtual void Destroyed();
	UFUNCTION(BlueprintImplementableEvent)
	void OnBuild();
	UPROPERTY(BlueprintAssignable)
	FOnBuildCompleted OnBuildCompleted;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arteries")
	UArteriesObject* FinalObject;
#if WITH_EDITORONLY_DATA
	const UEdGraphNode* DisplayNode;
	TMap<UEdGraphNode*, UArteriesObject*> Results;
	TMap<UArteriesObject*, int> Objects;
	FOnEditorBuildCompleted OnEditorBuildCompleted;
#endif
	TMap<UMaterialInterface*, FProcMeshSection> Sections;
	FCriticalSection Critical;
	FArteriesBuilder* Builder;
	FArteriesTaskQueue* TaskQueue;
};

