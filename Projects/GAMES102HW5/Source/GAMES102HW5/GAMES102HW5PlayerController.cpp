// Fill out your copyright notice in the Description page of Project Settings.


#include "GAMES102HW5PlayerController.h"
#include "CGDemoCanvas2D.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/StaticMeshActor.h"

AGAMES102HW5PlayerController::AGAMES102HW5PlayerController(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SubdivisionMethod = ESubdivisionMethod::Chaikin;
	Subdivisions = {
		{
			ESubdivisionMethod::Chaikin,
			MakeShareable(new FChaikin),
		},
		{
			ESubdivisionMethod::ThreeDegreeBSpline,
			MakeShareable(new FThreeDegreeBSpline),
		},
		{
			ESubdivisionMethod::FourPointInterpolation,
			MakeShareable(new FFourPointInterpolation),
		},
	};
}

void AGAMES102HW5PlayerController::BeginPlay()
{
	Super::BeginPlay();
	//MaxSamplePointsNum = FMath::CeilToInt(1. / SamplePointDT) + 1;
}

void AGAMES102HW5PlayerController::BindOnRightMouseButtonReleased()
{
	OnRightMouseButtonReleased.AddDynamic(this, &AGAMES102HW5PlayerController::AddCurvePointEvent);
}

void AGAMES102HW5PlayerController::BindOnCtrlAndKey1Released()
{
	OnCtrlAndKey1Released.AddDynamic(this, &AGAMES102HW5PlayerController::ChangeSubdivisionMethodToChaikin);
}

void AGAMES102HW5PlayerController::BindOnCtrlAndKey2Released()
{
	OnCtrlAndKey2Released.AddDynamic(this, &AGAMES102HW5PlayerController::ChangeSubdivisionMethodToThreeDegreeBSpline);
}

void AGAMES102HW5PlayerController::BindOnCtrlAndKey3Released()
{
	OnCtrlAndKey3Released.AddDynamic(this, &AGAMES102HW5PlayerController::ChangeSubdivisionMethodToFourPointInterpolation);
}

void AGAMES102HW5PlayerController::BindOnCtrlAndKey4Released()
{
}

void AGAMES102HW5PlayerController::BindOnCtrlAndKey5Released()
{
}

void AGAMES102HW5PlayerController::BindOnCtrlAndKey0Released()
{
	OnCtrlAndKey0Released.AddDynamic(this, &AGAMES102HW5PlayerController::ClearCanvasEvent);
}

void AGAMES102HW5PlayerController::ChangeSubdivisionMethod(ESubdivisionMethod Method)
{
	SubdivisionMethod = Method;
	ResampleCurve();
}

void AGAMES102HW5PlayerController::ClearCanvas()
{
	UE_LOG(LogTemp, Warning, TEXT("Clear Canvas"));
	for (int32 Layer = 0; Layer < Canvas2D->DisplayPoints.Num(); ++Layer) {
		Canvas2D->DisplayPoints[Layer].Array.Empty(0);
	}
	for (int32 Layer = 0; Layer < Canvas2D->DisplayLines.Num(); ++Layer) {
		Canvas2D->DisplayLines[Layer].Array.Empty(Canvas2D->DisplayLines[Layer].Array.Max());
	}
	for (int32 Layer = 0; Layer < Canvas2D->DisplayPolygons.Num(); ++Layer) {
		Canvas2D->DisplayPolygons[Layer].Array.Empty(0);
	}
	Canvas2D->ClearDrawing();
}

void AGAMES102HW5PlayerController::OnParamsInputChanged()
{
	ResampleCurve();
}

void AGAMES102HW5PlayerController::AddCurvePoint(const FVector& HitPoint)
{
	Canvas2D->DisplayPoints[0].Array.Add(HitPoint);
	ResampleCurve();
}

void AGAMES102HW5PlayerController::ChangeSubdivisionMethodToChaikin(FKey Key, EInputEvent Event, APlayerController* Ctrl)
{
	UE_LOG(LogTemp, Warning, TEXT("Chaikin"));
	ChangeSubdivisionMethod(ESubdivisionMethod::Chaikin);
}

void AGAMES102HW5PlayerController::ChangeSubdivisionMethodToThreeDegreeBSpline(FKey Key, EInputEvent Event, APlayerController* Ctrl)
{
	UE_LOG(LogTemp, Warning, TEXT("ThreeDegreeBSpline"));
	ChangeSubdivisionMethod(ESubdivisionMethod::ThreeDegreeBSpline);
}

void AGAMES102HW5PlayerController::ChangeSubdivisionMethodToFourPointInterpolation(FKey Key, EInputEvent Event, APlayerController* Ctrl)
{
	UE_LOG(LogTemp, Warning, TEXT("FourPointInterpolation"));
	ChangeSubdivisionMethod(ESubdivisionMethod::FourPointInterpolation);
}

void AGAMES102HW5PlayerController::ClearCanvasEvent(FKey Key, EInputEvent Event, APlayerController* Ctrl)
{
	ClearCanvas();
}

void AGAMES102HW5PlayerController::ResampleCurve()
{
	Canvas2D->ClearDrawing();
	Canvas2D->DrawPoints();

	if (Canvas2D->DisplayPoints[0].Array.Num() == 0)
	{
		return;
	}

	TArray<FSubdivisionBase*> EnabledSubdivisions;
	TArray<ESubdivisionMethod> EnabledSMethods;
	switch (SubdivisionMethod) {
	case ESubdivisionMethod::All:
	case ESubdivisionMethod::Chaikin:
	{
		EnabledSMethods.Add(ESubdivisionMethod::Chaikin);
		EnabledSubdivisions.Add(Subdivisions[ESubdivisionMethod::Chaikin].Get());
		EnabledSubdivisions.Last()->Params->bInClosed = ParamsInput.bClosed;
		Canvas3DTo2D(EnabledSubdivisions.Last()->Params->InPoints,
			Canvas2D->DisplayPoints[0].Array, false);
		if (SubdivisionMethod != ESubdivisionMethod::All) {
			break;
		}
	}
	case ESubdivisionMethod::ThreeDegreeBSpline:
	{
		EnabledSMethods.Add(ESubdivisionMethod::ThreeDegreeBSpline);
		EnabledSubdivisions.Add(Subdivisions[ESubdivisionMethod::ThreeDegreeBSpline].Get());
		EnabledSubdivisions.Last()->Params->bInClosed = ParamsInput.bClosed;
		Canvas3DTo2D(EnabledSubdivisions.Last()->Params->InPoints,
			Canvas2D->DisplayPoints[0].Array, false);
		if (SubdivisionMethod != ESubdivisionMethod::All) {
			break;
		}
	}
	case ESubdivisionMethod::FourPointInterpolation:
	{
		EnabledSMethods.Add(ESubdivisionMethod::FourPointInterpolation);
		EnabledSubdivisions.Add(Subdivisions[ESubdivisionMethod::FourPointInterpolation].Get());
		EnabledSubdivisions.Last()->Params->bInClosed = ParamsInput.bClosed;
		static_cast<FFourPointInterpolationParams*>(EnabledSubdivisions.Last()->Params.Get())->InAlpha = ParamsInput.Alpha;
		Canvas3DTo2D(EnabledSubdivisions.Last()->Params->InPoints,
			Canvas2D->DisplayPoints[0].Array, false);
		if (SubdivisionMethod != ESubdivisionMethod::All) {
			break;
		}
	}
	}
	if (EnabledSubdivisions.Num() == 0) {
		return;
	}
	for (int32 SIndex = 0; SIndex < EnabledSubdivisions.Num(); ++SIndex) {
		EnabledSubdivisions[SIndex]->Subdivide(ParamsInput.Num);
		if (ParamsInput.bClosed && EnabledSubdivisions[SIndex]->Params->OutPoints.Num() > 0)
		{
			auto NewPoint = EnabledSubdivisions[SIndex]->Params->OutPoints[0];
			EnabledSubdivisions[SIndex]->Params->OutPoints.Add(NewPoint);
		}
		int32 CurLayer = (int32)EnabledSMethods[SIndex];
		Canvas2DTo3D(Canvas2D->DisplayLines[CurLayer].Array, EnabledSubdivisions[SIndex]->Params->OutPoints, false);
		Canvas2D->DrawLines(CurLayer);
	}
}

void AGAMES102HW5PlayerController::AddCurvePointEvent(FKey Key, FVector2D MouseScreenPos, EInputEvent InputEvent, APlayerController* Ctrl)
{
	FVector WorldPos, WorldDir;
	Ctrl->DeprojectScreenPositionToWorld(MouseScreenPos.X, MouseScreenPos.Y, WorldPos, WorldDir);
	float Distance = 0;
	FVector HitPoint(0, 0, 0);
	bool bHit = TraceToCanvas(Distance, HitPoint, WorldPos, WorldDir);
	UE_LOG(LogTemp, Warning, TEXT("Right Mouse Button Released: %s, %s. %s"),
		*WorldPos.ToCompactString(), *WorldDir.ToCompactString(), (bHit ? TEXT("true") : TEXT("false")));
	if (!bHit) {
		return;
	}
	AddCurvePoint(HitPoint);
}
