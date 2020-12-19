// Copyright 2020 PacosLelouch, Inc. All Rights Reserved.
// https://github.com/PacosLelouch/

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MeshDenoising.generated.h"


class UArteriesObject;
class UProceduralMeshComponent;
class UStaticMesh;
struct FProcMeshSection;
struct FArteriesPoint;

UCLASS(BlueprintType, Blueprintable)
class ARTERIESPROCESSING_API UArteriesMeshDenoising : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category = "Mesh Denoising")
	static void MinimalSurfaceLocalInPlace(UArteriesObject* Obj, float MoveRatio = 0.25f, int32 MaxLoopCount = 10);

	UFUNCTION(BlueprintCallable, Category = "Mesh Denoising")
	static void MinimalSurfaceGlobalSoftInPlace(UArteriesObject* Obj, float MoveRatio = 1.f);

	UFUNCTION(BlueprintCallable, Category = "Mesh Denoising")
	static void MinimalSurfaceGlobalHardInPlace(UArteriesObject* Obj, float MoveRatio = 1.f);

	UFUNCTION(BlueprintCallable, Category = "Mesh Denoising")
	static void UpdateVertexInfos(UArteriesObject* Obj);

public:
	static void MinimalSurfaceValueGlobalHardInPlace(UArteriesObject* Obj, const TSet<FArteriesPoint*>& BoundaryPoints, int32 Dim = 3, FName Tag = NAME_None);

public:
	//static const FName LastPosTag;
	static const FName LaplaceBeltramiTag;
	static const FName MeanCurvatureTag;
	static const FName GaussianCurvatureTag;
};
