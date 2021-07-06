// Author: LiJiayu (JerryLi)
// Mail: lijiayu83@gmail.com (fullike@163.com)
// Copyright 2019. All Rights Reserved.

#pragma once
#include "ActorFactories/ActorFactory.h"
#include "ArteriesActorFactory.generated.h"
UCLASS()
class UArteriesActorFactory : public UActorFactory
{
	GENERATED_UCLASS_BODY()
	virtual void PostSpawnActor(UObject* Asset, AActor* NewActor) override;
	virtual void PostCreateBlueprint(UObject* Asset, AActor* CDO) override;
	virtual bool CanCreateActorFrom(const FAssetData& AssetData, FText& OutErrorMsg) override;
};