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
	static void MinimalSurfaceInPlace(UArteriesObject* Obj, float MoveRatio = 0.25f, int32 MaxLoopCount = 10);

	UFUNCTION(BlueprintCallable, Category = "Mesh Denoising")
	static void UpdateVertexInfos(UArteriesObject* Obj);

public:
	//static const FName LastPosTag;
	static const FName LaplaceBeltramiTag;
	static const FName MeanCurvatureTag;
	static const FName GaussianCurvatureTag;
};
