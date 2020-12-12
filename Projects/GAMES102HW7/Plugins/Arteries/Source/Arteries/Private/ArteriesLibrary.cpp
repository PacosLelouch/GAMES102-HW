// Author: LiJiayu (JerryLi)
// Mail: lijiayu83@gmail.com (fullike@163.com)
// Copyright 2019. All Rights Reserved.

#include "ArteriesLibrary.h"
#include "Engine/Engine.h"
#include "Async/Async.h"
UArteriesLibrary::UArteriesLibrary(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}
AArteriesActor* UArteriesLibrary::BeginDeferredActorSpawnFromClass(const UObject* WorldContextObject, TSubclassOf<AArteriesActor> ActorClass, const FTransform& SpawnTransform)
{
	AArteriesActor* Parent = FArteriesRunnable::GetThreadOwner();
	UWorld* World = Parent->GetWorld();
	//Don't use transaction flag here
	AArteriesActor* Actor = NewObject<AArteriesActor>(World->PersistentLevel, *ActorClass);
	Actor->SetActorTransform(SpawnTransform);
	return Actor;
}
AArteriesActor* UArteriesLibrary::FinishSpawningActor(AArteriesActor* Actor, bool AttachToCaller)
{
	AArteriesActor* Parent = FArteriesRunnable::GetThreadOwner();
	if (!AttachToCaller)
		Actor->SetOwner(Parent);
	Actor->Build(true);
	AsyncTask(ENamedThreads::GameThread, [Actor, Parent]()
	{
		UWorld* World = Parent->GetWorld();
		World->PersistentLevel->Actors.Add(Actor);
		World->PersistentLevel->ActorsForGC.Add(Actor);
#if WITH_EDITOR
		GEngine->BroadcastLevelActorAdded(Actor);
#endif
		if (!Actor->GetOwner())
			Actor->AttachToActor(Parent, FAttachmentTransformRules::KeepRelativeTransform);
	});
	return Actor;
}