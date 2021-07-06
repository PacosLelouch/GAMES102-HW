// Copyright 2020 PacosLelouch, Inc. All Rights Reserved.
// https://github.com/PacosLelouch/

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MeshSimplification.generated.h"


class UArteriesObject;
class UProceduralMeshComponent;
class UStaticMesh;
struct FProcMeshSection;
struct FArteriesPoint;

UCLASS(BlueprintType, Blueprintable)
class ARTERIESPROCESSING_API UArteriesMeshSimplification : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category = "Mesh Simplification")
	static void QuadricErrorMetrics(UArteriesObject* Obj, float SimpRatio = 0.25f);

	UFUNCTION(BlueprintCallable, Category = "Mesh Simplification")
	static void QuadricErrorMetricsDirect(UProceduralMeshComponent* Mesh, float SimpRatio = 0.25f, bool bKeepBoundary = true);

public:
	//static const FName LastPosTag;
	static const FName LaplaceBeltramiTag;
	static const FName MeanCurvatureTag;
	static const FName GaussianCurvatureTag;
};
