// Fill out your copyright notice in the Description page of Project Settings.


#include "GAMES102HW9PlayerController.h"
#include "CGDemoCanvas2D.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "MeshWrapper.h"

AGAMES102HW9PlayerController::AGAMES102HW9PlayerController(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InputParams.SimpRatio = 0.25f;
	InputParams.LoopCount = 10;
	//MeshWrapper = CreateDefaultSubobject<AMeshWrapper>(TEXT("MeshWrapper"));
}

void AGAMES102HW9PlayerController::BeginPlay()
{
	Super::BeginPlay();
	MeshWrapper = GetWorld()->SpawnActor<AMeshWrapper>(FVector(-200.f, 0.f, 0.f), FRotator(0.f));
}

void AGAMES102HW9PlayerController::BindOnCtrlAndKey1Released()
{
	OnCtrlAndKey1Released.AddDynamic(this, &AGAMES102HW9PlayerController::DisplayWhiteEvent);
}

void AGAMES102HW9PlayerController::BindOnCtrlAndKey2Released()
{

}

void AGAMES102HW9PlayerController::BindOnCtrlAndKey3Released()
{

}

void AGAMES102HW9PlayerController::BindOnCtrlAndKey0Released()
{
	OnCtrlAndKey0Released.AddDynamic(this, &AGAMES102HW9PlayerController::ProcessMeshEvent);
}

static UMaterialInterface* LastMaterial = nullptr;

void AGAMES102HW9PlayerController::LoadMesh(UStaticMesh* Mesh)
{
	MeshWrapper->LoadStaticMesh(Mesh, 0, false);
	if (LastMaterial)
	{
		MeshWrapper->LoadMaterial(LastMaterial);
	}
	DisplayMesh(DisplayType);
}

void AGAMES102HW9PlayerController::LoadMaterial(UMaterialInterface* Material)
{
	LastMaterial = Material;
	MeshWrapper->LoadMaterial(Material);
	DisplayMesh(DisplayType);
}

void AGAMES102HW9PlayerController::ProcessMesh()
{
	if (!MeshWrapper || !MeshWrapper->ComputeMesh)
	{
		return;
	}

	MeshWrapper->Simplification(InputParams.SimpRatio);
	DisplayMesh(DisplayType);
}

void AGAMES102HW9PlayerController::DisplayMesh(EDisplayType InDisplayType)
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

void AGAMES102HW9PlayerController::OnInputParamsChanged()
{
	ProcessMesh();
}

void AGAMES102HW9PlayerController::DisplayWhiteEvent(FKey Key, EInputEvent InputEvent, APlayerController* Ctrl)
{
	DisplayMesh(EDisplayType::White);
}

void AGAMES102HW9PlayerController::ProcessMeshEvent(FKey Key, EInputEvent InputEvent, APlayerController* Ctrl)
{
	ProcessMesh();
}
