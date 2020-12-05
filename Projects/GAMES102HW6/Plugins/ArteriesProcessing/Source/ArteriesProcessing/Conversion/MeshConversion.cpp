// Copyright 2020 PacosLelouch, Inc. All Rights Reserved.
// https://github.com/PacosLelouch/

#include "MeshConversion.h"
#include "ArteriesObject.h"
#include "ProceduralMeshComponent.h"
#include "KismetProceduralMeshLibrary.h"
#include "Materials/MaterialInterface.h"
#include "Engine/StaticMesh.h"
#include "PhysicsEngine/BodySetup.h"

const FName UArteriesMeshConversion::IndexPrefix = TEXT("ID_");

UArteriesObject* UArteriesMeshConversion::ConvertMeshFromRenderToCompute(UProceduralMeshComponent* InPMC, UObject* Outer)
{
	if (!InPMC->IsValidLowLevel())
	{
		return nullptr;
	}
	UArteriesObject* OutObj = NewObject<UArteriesObject>(Outer);

	if (!ConvertMeshFromRenderToComputeInPlace(OutObj, InPMC, Outer))
	{
		return nullptr;
	}

	return OutObj;
}

bool UArteriesMeshConversion::ConvertMeshFromRenderToComputeInPlace(UArteriesObject* OutObj, UProceduralMeshComponent* InPMC, UObject* Outer)
{
	if (!OutObj)
	{
		return false;
	}
	
	OutObj->ClearAll();
	for (int32 i = 0; i < InPMC->GetNumSections(); ++i)
	{
		FProcMeshSection* MeshPtr = InPMC->GetProcMeshSection(i);
		if (!MeshPtr)
		{
			continue;
		}
		FName GroupName(*(IndexPrefix.ToString() + FString::FromInt(i)));

		UMaterialInterface* Material = InPMC->GetMaterial(i);

		for (const FProcMeshVertex& Vertex : MeshPtr->ProcVertexBuffer)
		{
			FArteriesPoint* NewPoint = OutObj->AddPoint(Vertex.Position);
			NewPoint->SetVec3(Vertex.Tangent.bFlipTangentY ? AVN_TangentY : AVN_TangentX, Vertex.Tangent.TangentX);
			NewPoint->SetVec3(AVN_TangentZ, Vertex.Normal);
			NewPoint->SetVec3(AVN_Color, FVector(FLinearColor::FromSRGBColor(Vertex.Color)));
			NewPoint->SetVec2(AVN_UV0, Vertex.UV0);
			NewPoint->SetVec2(AVN_UV1, Vertex.UV1);
			NewPoint->SetVec2(AVN_UV2, Vertex.UV2);
			NewPoint->SetVec2(AVN_UV3, Vertex.UV3);

			OutObj->SetPointGroup(NewPoint, GroupName, 1, false);
		}

		for (int32 j = 0; j < MeshPtr->ProcIndexBuffer.Num(); j += 3)
		{
			int32 Index0 = MeshPtr->ProcIndexBuffer[j];
			int32 Index1 = MeshPtr->ProcIndexBuffer[j + 1];
			int32 Index2 = MeshPtr->ProcIndexBuffer[j + 2];
			TArray<FArteriesPoint*> Points{
				&OutObj->Points[Index0],
				&OutObj->Points[Index1],
				&OutObj->Points[Index2],
			};

			FArteriesPrimitive* NewPrimitive = OutObj->AddPrimitive(Points, true);
			NewPrimitive->SetInt(AVN_Collision, MeshPtr->bEnableCollision);
			NewPrimitive->SetInt(AVN_Visible, MeshPtr->bSectionVisible);
			if (Material)
			{
				NewPrimitive->Material = Material;
			}
			OutObj->SetPrimitiveGroup(NewPrimitive, GroupName, 1, false);
		}
	}

	return true;
}

UProceduralMeshComponent* UArteriesMeshConversion::ConvertMeshFromComputeToRender(UArteriesObject* InObj, UObject* Outer)
{
	if (!InObj->IsValidLowLevel())
	{
		return nullptr;
	}
	UProceduralMeshComponent* OutPMC = NewObject<UProceduralMeshComponent>(Outer);
	
	if (!ConvertMeshFromComputeToRenderInPlace(OutPMC, InObj, Outer))
	{
		return nullptr;
	}

	return OutPMC;
}

bool UArteriesMeshConversion::ConvertMeshFromComputeToRenderInPlace(UProceduralMeshComponent* OutPMC, UArteriesObject* InObj, UObject* Outer)
{
	if (!OutPMC)
	{
		return false;
	}
	OutPMC->ClearAllMeshSections();
	TMap<UMaterialInterface*, FProcMeshSection> Sections;
	InObj->Build(Sections);
	int32 Index = 0;
	for (auto& SectionPair : Sections)
	{
		OutPMC->SetMaterial(Index, SectionPair.Key);
		OutPMC->SetProcMeshSection(Index, SectionPair.Value);
	}
	return true;
}

void UArteriesMeshConversion::CreateProcMeshSectionFromStaticMesh(TArray<FProcMeshSection>& OutSections, UStaticMesh* StaticMesh, int32 LODIndex)
{
	if (!StaticMesh) {
		return;
	}
	//// MESH DATA

	int32 NumSections = StaticMesh->GetNumSections(LODIndex);
	for (int32 SectionIndex = 0; SectionIndex < NumSections; SectionIndex++)
	//for (int32 SectionIndex : SectionIndices)
	{
		if (SectionIndex >= NumSections) {
			continue;
		}
		// Buffers for copying geom data
		TArray<FVector> Vertices;
		TArray<int32> Triangles;
		TArray<FVector> Normals;
		TArray<FVector2D> UVs;
		TArray<FVector2D> UVs1;
		TArray<FVector2D> UVs2;
		TArray<FVector2D> UVs3;
		TArray<FProcMeshTangent> Tangents;

		// Get geom data from static mesh
		UKismetProceduralMeshLibrary::GetSectionFromStaticMesh(StaticMesh, LODIndex, SectionIndex, Vertices, Triangles, Normals, UVs, Tangents);

		// Create section using data
		FProcMeshSection& RealSection = OutSections.AddDefaulted_GetRef();
		FProcMeshSection Section;
		Section.ProcIndexBuffer.Reserve(Triangles.Num());
		for (int32 Index : Triangles) {
			Section.ProcIndexBuffer.Add(Index);
		}
		Section.ProcVertexBuffer.Reserve(Vertices.Num());
		for (int32 i = 0; i < Vertices.Num(); ++i) {
			FProcMeshVertex& V = Section.ProcVertexBuffer.AddDefaulted_GetRef();
			V.Position = Vertices[i];
			Section.SectionLocalBox += V.Position;
			if (i < Normals.Num()) {
				V.Normal = Normals[i];
			}
			if (i < UVs.Num()) {
				V.UV0 = UVs[i];
			}
			if (i < UVs1.Num()) {
				V.UV1 = UVs1[i];
			}
			if (i < UVs2.Num()) {
				V.UV2 = UVs2[i];
			}
			if (i < UVs3.Num()) {
				V.UV3 = UVs3[i];
			}
			if (i < Tangents.Num()) {
				V.Tangent = Tangents[i];
			}
		}
		MergePoints(RealSection, Section);
	}
}

UProceduralMeshComponent* UArteriesMeshConversion::CreateProcMeshComponentFromStaticMesh(UStaticMesh* StaticMesh, int32 LODIndex, bool bCreateCollision, UObject* Outer)
{
	if (StaticMesh != nullptr)
	{
		StaticMesh->bAllowCPUAccess = true;
		UProceduralMeshComponent* ProcMeshComponent = NewObject<UProceduralMeshComponent>(Outer);

		//// MESH DATA

		TArray<FProcMeshSection> Sections;
		CreateProcMeshSectionFromStaticMesh(Sections, StaticMesh, LODIndex);
		for (int32 i = 0; i < Sections.Num(); ++i)
		{
			Sections[i].bEnableCollision = bCreateCollision;
			ProcMeshComponent->SetProcMeshSection(i, Sections[i]);
		}

		//// SIMPLE COLLISION
		if (bCreateCollision)
		{
			// Clear any existing collision hulls
			ProcMeshComponent->ClearCollisionConvexMeshes();

			if (StaticMesh->BodySetup != nullptr)
			{
				// Iterate over all convex hulls on static mesh..
				const int32 NumConvex = StaticMesh->BodySetup->AggGeom.ConvexElems.Num();
				for (int ConvexIndex = 0; ConvexIndex < NumConvex; ConvexIndex++)
				{
					// Copy convex verts to ProcMesh
					FKConvexElem& MeshConvex = StaticMesh->BodySetup->AggGeom.ConvexElems[ConvexIndex];
					ProcMeshComponent->AddCollisionConvexMesh(MeshConvex.VertexData);
				}
			}
		}

		//// MATERIALS

		for (int32 MatIndex = 0; MatIndex < StaticMesh->StaticMaterials.Num(); MatIndex++)
		{
			ProcMeshComponent->SetMaterial(MatIndex, StaticMesh->GetMaterial(MatIndex));
		}

		return ProcMeshComponent;
	}
	return nullptr;
}

void UArteriesMeshConversion::MergePoints(FProcMeshSection& OutSection, const FProcMeshSection& InSection, float InvSnapDist)
{
	OutSection.bEnableCollision = InSection.bEnableCollision;
	OutSection.bSectionVisible = InSection.bSectionVisible;

	TMap<FIntVector, int32> VertexToIndex;
	TMap<int32, int32> IndexConversion;

	for (int32 i = 0; i < InSection.ProcVertexBuffer.Num(); ++i) {
		const FProcMeshVertex& Vertex = InSection.ProcVertexBuffer[i];
		FIntVector IntPos = FIntVector(Vertex.Position * InvSnapDist);
		int32 IndexToAddIfNotExist = OutSection.ProcVertexBuffer.Num();
		int32 Index = VertexToIndex.FindOrAdd(IntPos, IndexToAddIfNotExist);
		//IndexConversion.Add(i, OutSection.ProcVertexBuffer.Num());
		IndexConversion.Add(i, Index);
		if (Index == IndexToAddIfNotExist) {
			const FProcMeshVertex& NewVertex = OutSection.ProcVertexBuffer.Add_GetRef(Vertex);
			//FProcMeshVertex& NewVertex = OutSection.ProcVertexBuffer.AddDefaulted_GetRef();
			//NewVertex.Position = FVector(IntPos) * SnapDist;
			//NewVertex.Position.Z = Vertex.Position.Z;
			OutSection.SectionLocalBox += NewVertex.Position;
		}
	}
	for (int32 i = 0; i < InSection.ProcIndexBuffer.Num(); i += 3) {
		int32 I0 = IndexConversion[InSection.ProcIndexBuffer[i]];
		int32 I1 = IndexConversion[InSection.ProcIndexBuffer[i + 1]];
		int32 I2 = IndexConversion[InSection.ProcIndexBuffer[i + 2]];
		if (I0 == I1 || I1 == I2 || I2 == I0) {
			continue;
		}
		OutSection.ProcIndexBuffer.Add(I0);
		OutSection.ProcIndexBuffer.Add(I1);
		OutSection.ProcIndexBuffer.Add(I2);
	}
	//for (int32 Index : InSection.ProcIndexBuffer) {
	//	OutSection.ProcIndexBuffer.Add(IndexConversion[Index]);
	//}
}
