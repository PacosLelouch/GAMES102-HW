// Copyright 2020 PacosLelouch, Inc. All Rights Reserved.
// https://github.com/PacosLelouch/

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MeshConversion.generated.h"

class UArteriesObject;
class UProceduralMeshComponent;
class UStaticMesh;
struct FProcMeshSection;

UCLASS(BlueprintType, Blueprintable)
class ARTERIESPROCESSING_API UArteriesMeshConversion : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category = "Mesh Conversion")
	static UArteriesObject* ConvertMeshFromRenderToCompute(UProceduralMeshComponent* InPMC, UObject* Outer = nullptr);

	UFUNCTION(BlueprintCallable, Category = "Mesh Conversion")
	static bool ConvertMeshFromRenderToComputeInPlace(UArteriesObject* OutObj, UProceduralMeshComponent* InPMC, UObject* Outer = nullptr);

	UFUNCTION(BlueprintCallable, Category = "Mesh Conversion")
	static UProceduralMeshComponent* ConvertMeshFromComputeToRender(UArteriesObject* InObj, UObject* Outer = nullptr);

	UFUNCTION(BlueprintCallable, Category = "Mesh Conversion")
	static bool ConvertMeshFromComputeToRenderInPlace(UProceduralMeshComponent* OutPMC, UArteriesObject* InObj, UObject* Outer = nullptr);

public:
	static void CreateProcMeshSectionFromStaticMesh(TArray<FProcMeshSection>& OutSections, UStaticMesh* StaticMesh, int32 LODIndex);

	static UProceduralMeshComponent* CreateProcMeshComponentFromStaticMesh(UStaticMesh* StaticMesh, int32 LODIndex, bool bCreateCollision = false, UObject* Outer = nullptr);

	static void MergePoints(FProcMeshSection& OutSection, const FProcMeshSection& InSection, float InvSnapDist = 128.f);
public:
	static const FName IndexPrefix;
};
