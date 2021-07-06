// Copyright 2020 PacosLelouch, Inc. All Rights Reserved.
// https://github.com/PacosLelouch/

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MeshTessellation.generated.h"

class UArteriesObject;
struct FArteriesPoint;

UCLASS(BlueprintType, Blueprintable)
class ARTERIESPROCESSING_API UArteriesMeshTessellation : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category = "Mesh Tessellation")
	static void CentroidalVoronoiTessellationInPlaceForEachPrimitive(UArteriesObject* Obj, int32 Seed = 0, int32 Count = 20, float Density = 0.f, int32 Iterations = 10);
	
	UFUNCTION(BlueprintCallable, Category = "Mesh Tessellation")
	static UArteriesObject* CentroidalVoronoiTessellation(const TArray<FVector>& SortedBoundaryPoints, int32 Seed = 0, int32 Count = 20, float Density = 0.f, int32 Iterations = 10);

public:
	static void CentroidalVoronoiTessellationInPlaceByBoundary(UArteriesObject* Obj, const TArray<FArteriesPoint*>& SortedBoundaryPoints, int32 Seed = 0, int32 Count = 20, float Density = 0.f, int32 Iterations = 10);

	static void GetSortedBoundary(TArray<FArteriesPoint*>& SortedBoundaryPoints, const TSet<FArteriesPoint*>& BoundaryPoints);
};
