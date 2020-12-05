// Copyright 2020 PacosLelouch, Inc. All Rights Reserved.
// https://github.com/PacosLelouch/

#pragma once

#include "CoreMinimal.h"
#include "MeshWrapper.generated.h"

class UProceduralMeshComponent;
class UArteriesObject;
class UStaticMesh;

UCLASS(BlueprintType, Blueprintable)
class ARTERIESPROCESSING_API AMeshWrapper : public AActor
{
	GENERATED_BODY()
public:
	AMeshWrapper(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UFUNCTION(BlueprintCallable, Category = "Mesh Wrapper")
	void LoadStaticMesh(UStaticMesh* InStaticMesh, int32 LODIndex = 0, bool bCreateCollision = false);

	UFUNCTION(BlueprintCallable, Category = "Mesh Wrapper")
	void ConvertToMinimalSurface(float MoveRatio = 0.25f, int32 MaxLoopCount = 10, bool bUpdateImmediately = false);

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
