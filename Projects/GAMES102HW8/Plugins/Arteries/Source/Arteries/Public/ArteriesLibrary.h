// Author: LiJiayu (JerryLi)
// Mail: lijiayu83@gmail.com (fullike@163.com)
// Copyright 2019. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"
#include "ArteriesActor.h"
#include "ArteriesLibrary.generated.h"
UCLASS()
class ARTERIES_API UArteriesLibrary : public UObject
{
	GENERATED_UCLASS_BODY()
	UFUNCTION(BlueprintCallable, Category = Arteries, meta = (AdvancedDisplay = 1))
	static AArteriesActor* BeginDeferredActorSpawnFromClass(const UObject* WorldContextObject, TSubclassOf<AArteriesActor> ActorClass, const FTransform& SpawnTransform);
	UFUNCTION(BlueprintCallable, Category = Arteries, meta = (AdvancedDisplay = 1))
	static AArteriesActor* FinishSpawningActor(AArteriesActor* Actor, bool AttachToCaller);
};