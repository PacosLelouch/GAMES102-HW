// Fill out your copyright notice in the Description page of Project Settings.


#include "GAMES102HW6PlayerController.h"
#include "CGDemoCanvas2D.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "MeshWrapper.h"

AGAMES102HW6PlayerController::AGAMES102HW6PlayerController(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InputParams.Lambda = 0.125f;
	InputParams.LoopCount = 100;
	//MeshWrapper = CreateDefaultSubobject<AMeshWrapper>(TEXT("MeshWrapper"));
}

void AGAMES102HW6PlayerController::BeginPlay()
{
	Super::BeginPlay();
	MeshWrapper = GetWorld()->SpawnActor<AMeshWrapper>(FVector(-200.f, 0.f, 0.f), FRotator(0.f));
}

void AGAMES102HW6PlayerController::BindOnCtrlAndKey1Released()
{
	OnCtrlAndKey1Released.AddDynamic(this, &AGAMES102HW6PlayerController::DisplayWhiteEvent);
}

void AGAMES102HW6PlayerController::BindOnCtrlAndKey2Released()
{

}

void AGAMES102HW6PlayerController::BindOnCtrlAndKey3Released()
{

}

void AGAMES102HW6PlayerController::BindOnCtrlAndKey0Released()
{
	OnCtrlAndKey0Released.AddDynamic(this, &AGAMES102HW6PlayerController::ProcessMeshEvent);
}

void AGAMES102HW6PlayerController::LoadMesh(UStaticMesh* Mesh)
{
	MeshWrapper->LoadStaticMesh(Mesh, 0, false);
	DisplayMesh(DisplayType);
}

void AGAMES102HW6PlayerController::ProcessMesh()
{
	if (InputParams.LoopCount == 0 || InputParams.Lambda == 0. || !MeshWrapper || !MeshWrapper->ComputeMesh)
	{
		return;
	}

	if (!bProcedural)
	{
		MeshWrapper->ConvertToMinimalSurface(InputParams.Lambda, InputParams.LoopCount);
		DisplayMesh(DisplayType);
	}
	else if (!ProceduralHandler.IsValid())
	{
		ProceduralCount = 0;
		float Interval = 0.0625f;//FMath::Max(0.02f, InputParams.Lambda * 0.5f);
		GetWorld()->GetTimerManager().SetTimer(ProceduralHandler, [this]()
			{
				MeshWrapper->ConvertToMinimalSurface(InputParams.Lambda, 1);
				DisplayMesh(DisplayType);
				++ProceduralCount;
				if (ProceduralCount >= InputParams.LoopCount)
				{
					GetWorld()->GetTimerManager().ClearTimer(ProceduralHandler);
				}
				OnProceduralHandlerValidityChanged.ExecuteIfBound();
			}, 
			Interval, true, -1.f);
	}
}

void AGAMES102HW6PlayerController::DisplayMesh(EDisplayType InDisplayType)
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

void AGAMES102HW6PlayerController::OnInputParamsChanged()
{
	ProcessMesh();
}

void AGAMES102HW6PlayerController::DisplayWhiteEvent(FKey Key, EInputEvent InputEvent, APlayerController* Ctrl)
{
	DisplayMesh(EDisplayType::White);
}

void AGAMES102HW6PlayerController::ProcessMeshEvent(FKey Key, EInputEvent InputEvent, APlayerController* Ctrl)
{
	ProcessMesh();
}
