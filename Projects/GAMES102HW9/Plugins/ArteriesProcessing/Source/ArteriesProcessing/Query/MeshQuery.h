// Copyright 2020 PacosLelouch, Inc. All Rights Reserved.
// https://github.com/PacosLelouch/

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MeshQuery.generated.h"

class UArteriesObject;
class UProceduralMeshComponent;
class UStaticMesh;
struct FProcMeshSection;
struct FArteriesPoint;

UCLASS(BlueprintType, Blueprintable)
class ARTERIESPROCESSING_API UArteriesMeshQuery : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	//UFUNCTION(BlueprintPure, Category = "Mesh Query")
	static void FindBoundary(TSet<FArteriesPoint*>& OutBoundaryPoints, UArteriesObject* Obj);

	static void FindBoundary(TSet<int32>& OutBoundaryVertices, FProcMeshSection* SectionPtr);
};
