// Fill out your copyright notice in the Description page of Project Settings.


#include "GAMES102HW8PlayerController.h"
#include "CGDemoCanvas2D.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "MeshWrapper.h"

AGAMES102HW8PlayerController::AGAMES102HW8PlayerController(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InputParams.Density = 50.f;
	InputParams.LoopCount = 10;
	//MeshWrapper = CreateDefaultSubobject<AMeshWrapper>(TEXT("MeshWrapper"));
}

void AGAMES102HW8PlayerController::BeginPlay()
{
	Super::BeginPlay();
	MeshWrapper = GetWorld()->SpawnActor<AMeshWrapper>(FVector(-200.f, 0.f, 0.f), FRotator(0.f));
}

void AGAMES102HW8PlayerController::BindOnCtrlAndKey1Released()
{
	OnCtrlAndKey1Released.AddDynamic(this, &AGAMES102HW8PlayerController::DisplayWhiteEvent);
}

void AGAMES102HW8PlayerController::BindOnCtrlAndKey2Released()
{

}

void AGAMES102HW8PlayerController::BindOnCtrlAndKey3Released()
{

}

void AGAMES102HW8PlayerController::BindOnCtrlAndKey0Released()
{
	OnCtrlAndKey0Released.AddDynamic(this, &AGAMES102HW8PlayerController::ProcessMeshEvent);
}

static UMaterialInterface* LastMaterial = nullptr;

void AGAMES102HW8PlayerController::LoadMesh(UStaticMesh* Mesh)
{
	MeshWrapper->LoadStaticMesh(Mesh, 0, false);
	if (LastMaterial)
	{
		MeshWrapper->LoadMaterial(LastMaterial);
	}
	DisplayMesh(DisplayType);
}

void AGAMES102HW8PlayerController::LoadMaterial(UMaterialInterface* Material)
{
	LastMaterial = Material;
	MeshWrapper->LoadMaterial(Material);
	DisplayMesh(DisplayType);
}

void AGAMES102HW8PlayerController::ProcessMesh()
{
	if (InputParams.LoopCount == 0 || !MeshWrapper || !MeshWrapper->ComputeMesh)
	{
		return;
	}

	MeshWrapper->CentroidalVoronoiTessellation(
		static_cast<int32>(GetWorld()->TimeSeconds),
		0,
		InputParams.Density,
		InputParams.LoopCount);
	DisplayMesh(DisplayType);

	//MeshWrapper->ConvertToMinimalSurfaceGlobal(InputParams.Density, bSoft, false, true);
	//DisplayMesh(DisplayType);
	//if (!bSoft)
	//{
	//	MeshWrapper->ConvertToMinimalSurface(InputParams.Density, InputParams.LoopCount);
	//	DisplayMesh(DisplayType);
	//}
	//else if (!PostOneStepHandle.IsValid())
	//{
	//	ProceduralCount = 0;
	//	float Interval = 0.0625f;//FMath::Max(0.02f, InputParams.Density * 0.5f);
	//	GetWorld()->GetTimerManager().SetTimer(PostOneStepHandle, [this]()
	//		{
	//			MeshWrapper->ConvertToMinimalSurface(InputParams.Density, 1);
	//			DisplayMesh(DisplayType);
	//			++ProceduralCount;
	//			if (ProceduralCount >= InputParams.LoopCount)
	//			{
	//				GetWorld()->GetTimerManager().ClearTimer(PostOneStepHandle);
	//			}
	//			OnProceduralHandlerValidityChanged.ExecuteIfBound();
	//		}, 
	//		Interval, true, -1.f);
	//}
}

void AGAMES102HW8PlayerController::DisplayMesh(EDisplayType InDisplayType)
{
	DisplayType = InDisplayType;
	if (MeshWrapper->RenderMesh)
	{
		switch (DisplayType)
		{
		case EDisplayType::White:
			MeshWrapper->SetVertexColorWhite();
			MeshWrapper->UpdateRender();
			break;
		case EDisplayType::Normal:
			MeshWrapper->SetVertexColorByNormal();
			MeshWrapper->UpdateRender();
			break;
		case EDisplayType::MeanCurvature:
			MeshWrapper->SetVertexColorByMeanCurvature();
			MeshWrapper->UpdateRender();
			break;
		case EDisplayType::GaussianCurvature:
			MeshWrapper->SetVertexColorByGaussianCurvature();
			MeshWrapper->UpdateRender();
			break;
		}
	}
}

void AGAMES102HW8PlayerController::OnInputParamsChanged()
{
	ProcessMesh();
}

void AGAMES102HW8PlayerController::DisplayWhiteEvent(FKey Key, EInputEvent InputEvent, APlayerController* Ctrl)
{
	DisplayMesh(EDisplayType::White);
}

void AGAMES102HW8PlayerController::ProcessMeshEvent(FKey Key, EInputEvent InputEvent, APlayerController* Ctrl)
{
	ProcessMesh();
}
