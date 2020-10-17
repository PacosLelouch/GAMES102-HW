// Fill out your copyright notice in the Description page of Project Settings.


#include "GAMES102HW1PlayerController.h"
#include "CGDemoCanvas2D.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/StaticMeshActor.h"

AGAMES102HW1PlayerController::AGAMES102HW1PlayerController(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	FittingMethod = EFunctionFittingMethod::PolynomialFitting;
	Fittings = {
		{ EFunctionFittingMethod::PolynomialFitting, MakeShared<FPolynomialFitting>() },
		{ EFunctionFittingMethod::GaussianFitting, MakeShared<FGaussianFitting>() },
		{ EFunctionFittingMethod::PolynomialRegression, MakeShared<FPolynomialRegression>() },
		{ EFunctionFittingMethod::RidgeRegression, MakeShared<FRidgeRegression>() },
	};
}

void AGAMES102HW1PlayerController::BeginPlay()
{
	Super::BeginPlay();
	MaxSamplePointsNum = FMath::CeilToInt((Canvas2D->CanvasBoxYZ.Max.Y - Canvas2D->CanvasBoxYZ.Min.Y) / SamplePointDY) + 1;
	//for (FOccluderVertexArray& VArray : Canvas2D->DisplayPoints) {
	//	VArray.Reserve(MaxSamplePointsNum);
	//}
	//for (FOccluderVertexArray& VArray : Canvas2D->DisplayLines) {
	//	VArray.Reserve(MaxSamplePointsNum);
	//}
}

void AGAMES102HW1PlayerController::BindOnRightMouseButtonReleased()
{
	OnRightMouseButtonReleased.AddDynamic(this, &AGAMES102HW1PlayerController::AddFunctionPoint);
}

void AGAMES102HW1PlayerController::BindOnKey1Released()
{
	OnCtrlAndKey1Released.AddDynamic(this, &AGAMES102HW1PlayerController::ChangeFittingMethodToPolynomialFitting);
}

void AGAMES102HW1PlayerController::BindOnKey2Released()
{
	OnCtrlAndKey2Released.AddDynamic(this, &AGAMES102HW1PlayerController::ChangeFittingMethodToGaussianFitting);
}

void AGAMES102HW1PlayerController::BindOnKey3Released()
{
	OnCtrlAndKey3Released.AddDynamic(this, &AGAMES102HW1PlayerController::ChangeFittingMethodToPolynomialRegression);
}

void AGAMES102HW1PlayerController::BindOnKey4Released()
{
	OnCtrlAndKey4Released.AddDynamic(this, &AGAMES102HW1PlayerController::ChangeFittingMethodToRidgeFitting);
}

void AGAMES102HW1PlayerController::BindOnKey5Released()
{
	OnCtrlAndKey5Released.AddDynamic(this, &AGAMES102HW1PlayerController::ChangeFittingMethodToAllFitting);
}

void AGAMES102HW1PlayerController::BindOnKey0Released()
{
	OnCtrlAndKey0Released.AddDynamic(this, &AGAMES102HW1PlayerController::ClearCanvasEvent);
}

void AGAMES102HW1PlayerController::ChangeFittingMethod(EFunctionFittingMethod Method)
{
	//Canvas2D->DisplayPoints.Empty(MaxSamplePointsNum);
	//Canvas2D->DisplayLines.Empty(MaxSamplePointsNum);
	FittingMethod = Method;
	ResampleFunction();
}

void AGAMES102HW1PlayerController::ClearCanvas()
{
	UE_LOG(LogTemp, Warning, TEXT("Clear Canvas"));
	Canvas2D->DisplayPoints[0].Array.Empty(MaxSamplePointsNum);
	for (const TPair<EFunctionFittingMethod, TSharedPtr<FFunctionFittingBase> >& FitPair : Fittings) {
		Canvas2D->DisplayLines[(int32)FitPair.Key].Array.Empty(MaxSamplePointsNum);
	}
	Canvas2D->ClearDrawing();
}

void AGAMES102HW1PlayerController::OnParamsInputChanged()
{
	ResampleFunction();
}

void AGAMES102HW1PlayerController::ChangeFittingMethodToPolynomialFitting(FKey Key, EInputEvent Event, APlayerController* Ctrl)
{
	UE_LOG(LogTemp, Warning, TEXT("Polynomial Fitting"));
	ChangeFittingMethod(EFunctionFittingMethod::PolynomialFitting);
}

void AGAMES102HW1PlayerController::ChangeFittingMethodToGaussianFitting(FKey Key, EInputEvent Event, APlayerController* Ctrl)
{
	UE_LOG(LogTemp, Warning, TEXT("Gaussian Fitting"));
	ChangeFittingMethod(EFunctionFittingMethod::GaussianFitting);
}

void AGAMES102HW1PlayerController::ChangeFittingMethodToPolynomialRegression(FKey Key, EInputEvent Event, APlayerController* Ctrl)
{
	UE_LOG(LogTemp, Warning, TEXT("Polynomial Regression"));
	ChangeFittingMethod(EFunctionFittingMethod::PolynomialRegression);
}

void AGAMES102HW1PlayerController::ChangeFittingMethodToRidgeFitting(FKey Key, EInputEvent Event, APlayerController* Ctrl)
{
	UE_LOG(LogTemp, Warning, TEXT("Ridge Fitting"));
	ChangeFittingMethod(EFunctionFittingMethod::RidgeRegression);
}

void AGAMES102HW1PlayerController::ChangeFittingMethodToAllFitting(FKey Key, EInputEvent Event, APlayerController* Ctrl)
{
	UE_LOG(LogTemp, Warning, TEXT("All Fitting"));
	ChangeFittingMethod(EFunctionFittingMethod::All);
}

void AGAMES102HW1PlayerController::ClearCanvasEvent(FKey Key, EInputEvent Event, APlayerController* Ctrl)
{
	ClearCanvas();
}

void AGAMES102HW1PlayerController::ResampleFunction()
{
	Canvas2D->ClearDrawing();
	Canvas2D->DrawPoints();
	TArray<EFunctionFittingMethod> EnabledMethods;
	switch (FittingMethod) {
	case EFunctionFittingMethod::All:
	case EFunctionFittingMethod::PolynomialFitting:
	{
		EnabledMethods.Add(EFunctionFittingMethod::PolynomialFitting);
		FPolynomialFittingParams& Params = 
			StaticCastSharedPtr<FPolynomialFitting>(Fittings[EFunctionFittingMethod::PolynomialFitting])->Params;
		Canvas3DTo2D(Params.InPoints, Canvas2D->DisplayPoints[0].Array, false);
		if (FittingMethod != EFunctionFittingMethod::All) {
			break;
		}
	}
	case EFunctionFittingMethod::GaussianFitting:
	{
		EnabledMethods.Add(EFunctionFittingMethod::GaussianFitting);
		FGaussianFittingParams& Params = 
			StaticCastSharedPtr<FGaussianFitting>(Fittings[EFunctionFittingMethod::GaussianFitting])->Params;
		Params.InSigma = ParamsInput.GaussianSigma;
		Canvas3DTo2D(Params.InPoints, Canvas2D->DisplayPoints[0].Array, false);
		if (FittingMethod != EFunctionFittingMethod::All) {
			break;
		}
	}
	case EFunctionFittingMethod::PolynomialRegression:
	{
		EnabledMethods.Add(EFunctionFittingMethod::PolynomialRegression);
		FPolynomialRegressionParams& Params = 
			StaticCastSharedPtr<FPolynomialRegression>(Fittings[EFunctionFittingMethod::PolynomialRegression])->Params;
		Params.InSpanNum = ParamsInput.RegressionSpan;
		Canvas3DTo2D(Params.InPoints, Canvas2D->DisplayPoints[0].Array, false);
		if (FittingMethod != EFunctionFittingMethod::All) {
			break;
		}
	}
	case EFunctionFittingMethod::RidgeRegression:
	{
		EnabledMethods.Add(EFunctionFittingMethod::RidgeRegression);
		FRidgeRegressionParams& Params = 
			StaticCastSharedPtr<FRidgeRegression>(Fittings[EFunctionFittingMethod::RidgeRegression ])->Params;
		Params.InSpanNum = ParamsInput.RegressionSpan;
		Params.InLambda = ParamsInput.RidgeLambda;
		Canvas3DTo2D(Params.InPoints, Canvas2D->DisplayPoints[0].Array, false);
		if (FittingMethod != EFunctionFittingMethod::All) {
			break;
		}
	}
	}

	for (EFunctionFittingMethod Layer : EnabledMethods) {
		TSharedPtr<FFunctionFittingBase>& Fitting = Fittings[Layer];
		Fitting->Fit();
		TArray<TTuple<double, double> > SampleResults;
		Fitting->Sample(SampleResults, Canvas2D->CanvasBoxYZ.Min.Y, Canvas2D->CanvasBoxYZ.Max.Y, SamplePointDY);
		Canvas2D->DisplayLines[(int32)Layer].Array.Empty(MaxSamplePointsNum);
		Canvas2DTo3D(Canvas2D->DisplayLines[(int32)Layer].Array, SampleResults, true);
		Canvas2D->DrawLines((int32)Layer);
	}
}

void AGAMES102HW1PlayerController::AddFunctionPoint(FKey Key, FVector2D MouseScreenPos, EInputEvent InputEvent, APlayerController* Ctrl)
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
	Canvas2D->DisplayPoints[0].Array.Add(HitPoint);
	ResampleFunction();
}
