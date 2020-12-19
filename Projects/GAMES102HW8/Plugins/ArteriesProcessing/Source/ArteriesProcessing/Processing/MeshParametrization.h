// Copyright 2020 PacosLelouch, Inc. All Rights Reserved.
// https://github.com/PacosLelouch/

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MeshParametrization.generated.h"

class UArteriesObject;
struct FArteriesPoint;

UCLASS(BlueprintType, Blueprintable)
class ARTERIESPROCESSING_API UArteriesMeshParametrization : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category = "Mesh Parametrization")
	static void FloaterParametrizationInPlace(UArteriesObject* Obj);

	static void MapUV(UArteriesObject* Obj, const TSet<FArteriesPoint*>& BoundaryPoints);
};