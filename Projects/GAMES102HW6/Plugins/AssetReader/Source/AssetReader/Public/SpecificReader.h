// Copyright 2020 PacosLelouch, Inc. All Rights Reserved.
// https://github.com/PacosLelouch/

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "AssetData.h"
#include "SpecificReader.generated.h"

class UTexture2D;

UCLASS()
class ASSETREADER_API USpecificReader : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category = "Asset Reader")
	static void Load(TArray<FAssetData>& OutAssetArray, TSubclassOf<UObject> Class, FString Path = TEXT("/Game/"));

	UFUNCTION(BlueprintCallable, Category = "Asset Reader")
	static UTexture2D* GetThumbnailFromAssetData(const FAssetData& AssetData);
};
