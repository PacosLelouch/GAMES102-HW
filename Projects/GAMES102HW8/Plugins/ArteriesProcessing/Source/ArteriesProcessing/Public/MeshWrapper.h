// Copyright 2020 PacosLelouch, Inc. All Rights Reserved.
// https://github.com/PacosLelouch/

#pragma once

#include "CoreMinimal.h"
#include "MeshWrapper.generated.h"

class UProceduralMeshComponent;
class UArteriesObject;
class UStaticMesh;
class UMaterialInterface;

UCLASS(BlueprintType, Blueprintable)
class ARTERIESPROCESSING_API AMeshWrapper : public AActor
{
	GENERATED_BODY()
public:
	AMeshWrapper(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UFUNCTION(BlueprintCallable, Category = "Mesh Wrapper")
	void LoadStaticMesh(UStaticMesh* InStaticMesh, int32 LODIndex = 0, bool bCreateCollision = false);

	UFUNCTION(BlueprintCallable, Category = "Mesh Wrapper")
	void LoadMaterial(UMaterialInterface* Material);
	
	UFUNCTION(BlueprintCallable, Category = "Mesh Wrapper")
	void ConvertToMinimalSurfaceGlobal(float MoveRatio =  1.f, bool bSoft = false, bool bReduce = false, bool bUpdateImmediately = false);

	UFUNCTION(BlueprintCallable, Category = "Mesh Wrapper")
	void CentroidalVoronoiTessellation(int32 Seed = 0, int32 Count = 20, float Density = 0.f, int32 Iterations = 10);

	//void ConvertToMinimalSurfaceGlobal(float MoveRatio = 1.f, bool bSoft = false, bool bReduce = false, bool bUpdateImmediately = false);

	UFUNCTION(BlueprintCallable, Category = "Mesh Wrapper")
	void ConvertToMinimalSurfaceLocal(float MoveRatio = 0.25f, int32 MaxLoopCount = 10, bool bReduce = false, bool bUpdateImmediately = false);

	UFUNCTION(BlueprintCallable, Category = "Mesh Wrapper")
	void Parametrization();

	UFUNCTION(BlueprintCallable, Category = "Mesh Wrapper")
	void SetVertexColorWhite();

	UFUNCTION(BlueprintCallable, Category = "Mesh Wrapper")
	void SetVertexColorByNormal();

	UFUNCTION(BlueprintCallable, Category = "Mesh Wrapper")
	void SetVertexColorByGaussianCurvature();

	UFUNCTION(BlueprintCallable, Category = "Mesh Wrapper")
	void SetVertexColorByMeanCurvature();

	UFUNCTION(BlueprintCallable, Category = "Mesh Wrapper")
	void UpdateRender();

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh Wrapper")
	UProceduralMeshComponent* RenderMesh = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh Wrapper")
	UArteriesObject* ComputeMesh = nullptr;

public:

protected:
	FLinearColor GetColorFromColorBar(float Value, float AbsMax = 2.f);

	//UPROPERTY(BlueprintReadWrite, Category = "Mesh Wrapper")
	//UArteriesObject* OriginalMesh = nullptr;
};
