// Copyright 2020 PacosLelouch, Inc. All Rights Reserved.
// https://github.com/PacosLelouch/


#include "MeshWrapper.h"
#include "ArteriesObject.h"
#include "ProceduralMeshComponent.h"
//#include "KismetProceduralMeshLibrary.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "../Conversion/MeshConversion.h"
#include "../Processing/MeshDenoising.h"
#include "../Processing/MeshParametrization.h"

AMeshWrapper::AMeshWrapper(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));

}

void AMeshWrapper::LoadStaticMesh(UStaticMesh* InStaticMesh, int32 LODIndex, bool bCreateCollision)
{
	if (RenderMesh)
	{
		RenderMesh->DestroyComponent();
	}

	UObject* Outer = Cast<UObject>(this);
	RenderMesh = UArteriesMeshConversion::CreateProcMeshComponentFromStaticMesh(InStaticMesh, LODIndex, bCreateCollision, Outer);

	for (int32 i = 0; i < RenderMesh->GetNumSections(); ++i)
	{
		FProcMeshSection& Section = *RenderMesh->GetProcMeshSection(i);
		FProcMeshSection CopySection = Section;
		Section.Reset();
		UArteriesMeshConversion::MergePoints(Section, CopySection);
	}

	ComputeMesh = UArteriesMeshConversion::ConvertMeshFromRenderToCompute(RenderMesh, Outer);
	UArteriesMeshDenoising::UpdateVertexInfos(ComputeMesh);
}

void AMeshWrapper::LoadMaterial(UMaterialInterface* Material)
{
	if (RenderMesh)
	{
		for (int32 i = 0; i < RenderMesh->GetNumMaterials(); ++i)
		{
			RenderMesh->SetMaterial(i, Material);
		}
	}
	if (ComputeMesh)
	{
		for (FArteriesPrimitive& Primitive : ComputeMesh->Primitives)
		{
			Primitive.Material = Material;
		}
		if (!RenderMesh)
		{
			RenderMesh = UArteriesMeshConversion::ConvertMeshFromComputeToRender(ComputeMesh, Cast<UObject>(this));
		}
	}
	Parametrization();
}

void AMeshWrapper::ConvertToMinimalSurfaceGlobal(float MoveRatio, bool bSoft, bool bReduce, bool bUpdateImmediately)
{
	if (bReduce)
	{
		for (int32 i = 0; i < RenderMesh->GetNumSections(); ++i)
		{
			FProcMeshSection& Section = *RenderMesh->GetProcMeshSection(i);
			FProcMeshSection CopySection = Section;
			Section.Reset();
			UArteriesMeshConversion::MergePoints(Section, CopySection);
		}
		if (!ComputeMesh)
		{
			UObject* Outer = Cast<UObject>(this);
			ComputeMesh = NewObject<UArteriesObject>(Outer);
		}
		UArteriesMeshConversion::ConvertMeshFromRenderToComputeInPlace(ComputeMesh, RenderMesh, Cast<UObject>(this));
	}

	if (bSoft)
	{
		UArteriesMeshDenoising::MinimalSurfaceGlobalSoftInPlace(ComputeMesh, MoveRatio);
	}
	else
	{
		UArteriesMeshDenoising::MinimalSurfaceGlobalHardInPlace(ComputeMesh, MoveRatio);
	}

	if (bUpdateImmediately)
	{
		UArteriesMeshConversion::ConvertMeshFromComputeToRenderInPlace(RenderMesh, ComputeMesh, Cast<UObject>(this));
	}
}

void AMeshWrapper::ConvertToMinimalSurfaceLocal(float MoveRatio, int32 MaxLoopCount, bool bReduce, bool bUpdateImmediately)
{
	if (bReduce)
	{
		for (int32 i = 0; i < RenderMesh->GetNumSections(); ++i)
		{
			FProcMeshSection& Section = *RenderMesh->GetProcMeshSection(i);
			FProcMeshSection CopySection = Section;
			Section.Reset();
			UArteriesMeshConversion::MergePoints(Section, CopySection);
		}
		if (!ComputeMesh)
		{
			UObject* Outer = Cast<UObject>(this);
			ComputeMesh = NewObject<UArteriesObject>(Outer);
		}
		UArteriesMeshConversion::ConvertMeshFromRenderToComputeInPlace(ComputeMesh, RenderMesh, Cast<UObject>(this));
	}

	UArteriesMeshDenoising::MinimalSurfaceLocalInPlace(ComputeMesh, MoveRatio, MaxLoopCount);
	if (bUpdateImmediately)
	{
		UArteriesMeshConversion::ConvertMeshFromComputeToRenderInPlace(RenderMesh, ComputeMesh, Cast<UObject>(this));
	}
}

void AMeshWrapper::Parametrization()
{
	if (!ComputeMesh)
	{
		return;
	}
	UArteriesMeshParametrization::FloaterParametrizationInPlace(ComputeMesh);
	if (!RenderMesh)
	{
		RenderMesh = UArteriesMeshConversion::ConvertMeshFromComputeToRender(ComputeMesh, Cast<UObject>(this));
	}
	else
	{
		UArteriesMeshConversion::ConvertMeshFromComputeToRenderInPlace(RenderMesh, ComputeMesh, Cast<UObject>(this));
	}
}

void AMeshWrapper::SetVertexColorWhite()
{
	if (!ComputeMesh)
	{
		return;
	}
	UObject* Outer = Cast<UObject>(this);

	for (FArteriesPoint& Point : ComputeMesh->Points)
	{
		Point.SetVec3(AVN_Color, FVector::OneVector);
	}
	if (!RenderMesh)
	{
		RenderMesh = NewObject<UProceduralMeshComponent>(Outer);
	}
	UArteriesMeshConversion::ConvertMeshFromComputeToRenderInPlace(RenderMesh, ComputeMesh, Outer);
}

void AMeshWrapper::SetVertexColorByNormal()
{
	if (!ComputeMesh)
	{
		return;
	}
	UObject* Outer = Cast<UObject>(this);

	for (FArteriesPoint& Point : ComputeMesh->Points)
	{
		if(Point.HasVec3(AVN_TangentZ))
		{
			Point.SetVec3(AVN_Color, Point.GetVec3(AVN_TangentZ) * 0.5f + 0.5f);
		}
		else
		{
			Point.SetVec3(AVN_Color, FVector::ZeroVector);
		}
	}
	if (!RenderMesh)
	{
		RenderMesh = NewObject<UProceduralMeshComponent>(Outer);
	}
	UArteriesMeshConversion::ConvertMeshFromComputeToRenderInPlace(RenderMesh, ComputeMesh, Outer);
}

void AMeshWrapper::SetVertexColorByGaussianCurvature()
{
	if (!ComputeMesh)
	{
		return;
	}
	UObject* Outer = Cast<UObject>(this);

	for (FArteriesPoint& Point : ComputeMesh->Points)
	{
		if (Point.HasFloat(UArteriesMeshDenoising::GaussianCurvatureTag))
		{
			float Value = Point.GetFloat(UArteriesMeshDenoising::GaussianCurvatureTag);
			Point.SetVec3(AVN_Color, FVector(GetColorFromColorBar(Value, 0.02f/*0.0625f*/)));
		}
		else
		{
			Point.SetVec3(AVN_Color, FVector::ZeroVector);
		}
	}
	if (!RenderMesh)
	{
		RenderMesh = NewObject<UProceduralMeshComponent>(Outer);
	}
	UArteriesMeshConversion::ConvertMeshFromComputeToRenderInPlace(RenderMesh, ComputeMesh, Outer);
}

void AMeshWrapper::SetVertexColorByMeanCurvature()
{
	if (!ComputeMesh)
	{
		return;
	}
	UObject* Outer = Cast<UObject>(this);

	for (FArteriesPoint& Point : ComputeMesh->Points)
	{
		if (Point.HasFloat(UArteriesMeshDenoising::MeanCurvatureTag))
		{
			float Value = Point.GetFloat(UArteriesMeshDenoising::MeanCurvatureTag);
			Point.SetVec3(AVN_Color, FVector(GetColorFromColorBar(Value, 0.25f/*1.0f*/)));
		}
		else
		{
			Point.SetVec3(AVN_Color, FVector::ZeroVector);
		}
	}
	if (!RenderMesh)
	{
		RenderMesh = NewObject<UProceduralMeshComponent>(Outer);
	}
	UArteriesMeshConversion::ConvertMeshFromComputeToRenderInPlace(RenderMesh, ComputeMesh, Outer);
}

void AMeshWrapper::UpdateRender()
{
	if (!RenderMesh->IsRegistered())
	{
		RenderMesh->Mobility = RootComponent->Mobility;
		RenderMesh->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);
		AddInstanceComponent(RenderMesh);
		RenderMesh->RegisterComponent();
	}
	else 
	{
		RenderMesh->MarkRenderTransformDirty();
	}
}

FLinearColor AMeshWrapper::GetColorFromColorBar(float Value, float AbsMax)
{
	FLinearColor HSV(240.f * FMath::Max(1.f - FMath::Abs(Value) / AbsMax, 0.f), 1.f, 1.f);
	return HSV.HSVToLinearRGB();
}
