// Fill out your copyright notice in the Description page of Project Settings.


#include "GAMES102HW3PlayerController.h"
#include "CGDemoCanvas2D.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/StaticMeshActor.h"

AGAMES102HW3PlayerController::AGAMES102HW3PlayerController(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	FittingMethod = EFunctionFittingMethod::PolynomialRegression;
	Fittings = {
		{
			EFunctionFittingMethod::PolynomialFitting, 
			MakeTuple(TSharedPtr<FFunctionFittingBase>(new FPolynomialFitting), TSharedPtr<FFunctionFittingBase>(new FPolynomialFitting))
		},
		{
			EFunctionFittingMethod::GaussianFitting,
			MakeTuple(TSharedPtr<FFunctionFittingBase>(new FGaussianFitting), TSharedPtr<FFunctionFittingBase>(new FGaussianFitting)),
		},
		{
			EFunctionFittingMethod::PolynomialRegression,
			MakeTuple(TSharedPtr<FFunctionFittingBase>(new FPolynomialRegression), TSharedPtr<FFunctionFittingBase>(new FPolynomialRegression)),
		},
		{
			EFunctionFittingMethod::RidgeRegression,
			MakeTuple(TSharedPtr<FFunctionFittingBase>(new FRidgeRegression), TSharedPtr<FFunctionFittingBase>(new FRidgeRegression)),
		},
	};

	ParametrizationMethod = EParametrizationMethod::Uniform;
	Parametrizations = {
		{
			EParametrizationMethod::Uniform,
			TSharedPtr<FParametrizationBase>(new FUniformParametrization),
		},
		{
			EParametrizationMethod::Chordal,
			TSharedPtr<FParametrizationBase>(new FChordalParametrization),
		},
		{
			EParametrizationMethod::Centripetal,
			TSharedPtr<FParametrizationBase>(new FCentripetalParametrization),
		},
		{
			EParametrizationMethod::Foley,
			TSharedPtr<FParametrizationBase>(new FFoleyParametrization),
		},
	};
}

void AGAMES102HW3PlayerController::BeginPlay()
{
	Super::BeginPlay();
	MaxSamplePointsNum = FMath::CeilToInt(1. / SamplePointDT) + 1;
}

void AGAMES102HW3PlayerController::BindOnRightMouseButtonReleased()
{
	OnRightMouseButtonReleased.AddDynamic(this, &AGAMES102HW3PlayerController::AddCurvePointEvent);
}

void AGAMES102HW3PlayerController::BindOnCtrlAndKey1Released()
{
	OnCtrlAndKey1Released.AddDynamic(this, &AGAMES102HW3PlayerController::ChangeFittingMethodToPolynomialFitting);
}

void AGAMES102HW3PlayerController::BindOnCtrlAndKey2Released()
{
	OnCtrlAndKey2Released.AddDynamic(this, &AGAMES102HW3PlayerController::ChangeFittingMethodToGaussianFitting);
}

void AGAMES102HW3PlayerController::BindOnCtrlAndKey3Released()
{
	OnCtrlAndKey3Released.AddDynamic(this, &AGAMES102HW3PlayerController::ChangeFittingMethodToPolynomialRegression);
}

void AGAMES102HW3PlayerController::BindOnCtrlAndKey4Released()
{
	OnCtrlAndKey4Released.AddDynamic(this, &AGAMES102HW3PlayerController::ChangeFittingMethodToRidgeFitting);
}

void AGAMES102HW3PlayerController::BindOnCtrlAndKey5Released()
{
	OnCtrlAndKey5Released.AddDynamic(this, &AGAMES102HW3PlayerController::ChangeFittingMethodToAllFitting);
}

void AGAMES102HW3PlayerController::BindOnCtrlAndKey0Released()
{
	OnCtrlAndKey0Released.AddDynamic(this, &AGAMES102HW3PlayerController::ClearCanvasEvent);
}

void AGAMES102HW3PlayerController::ChangeFittingMethod(EFunctionFittingMethod Method)
{
	FittingMethod = Method;
	ResampleFunction();
}

void AGAMES102HW3PlayerController::ChangeParametrizationMethod(EParametrizationMethod Method)
{
	ParametrizationMethod = Method;
	ResampleFunction();
}

void AGAMES102HW3PlayerController::ClearCanvas()
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

void AGAMES102HW3PlayerController::OnParamsInputChanged()
{
	ResampleFunction();
}

void AGAMES102HW3PlayerController::AddCurvePoint(const FVector& HitPoint)
{
	Canvas2D->DisplayPoints[0].Array.Add(HitPoint);
	ResampleFunction();
}

void AGAMES102HW3PlayerController::ChangeFittingMethodToPolynomialFitting(FKey Key, EInputEvent Event, APlayerController* Ctrl)
{
	UE_LOG(LogTemp, Warning, TEXT("Polynomial Fitting"));
	ChangeFittingMethod(EFunctionFittingMethod::PolynomialFitting);
}

void AGAMES102HW3PlayerController::ChangeFittingMethodToGaussianFitting(FKey Key, EInputEvent Event, APlayerController* Ctrl)
{
	UE_LOG(LogTemp, Warning, TEXT("Gaussian Fitting"));
	ChangeFittingMethod(EFunctionFittingMethod::GaussianFitting);
}

void AGAMES102HW3PlayerController::ChangeFittingMethodToPolynomialRegression(FKey Key, EInputEvent Event, APlayerController* Ctrl)
{
	UE_LOG(LogTemp, Warning, TEXT("Polynomial Regression"));
	ChangeFittingMethod(EFunctionFittingMethod::PolynomialRegression);
}

void AGAMES102HW3PlayerController::ChangeFittingMethodToRidgeFitting(FKey Key, EInputEvent Event, APlayerController* Ctrl)
{
	UE_LOG(LogTemp, Warning, TEXT("Ridge Fitting"));
	ChangeFittingMethod(EFunctionFittingMethod::RidgeRegression);
}

void AGAMES102HW3PlayerController::ChangeFittingMethodToAllFitting(FKey Key, EInputEvent Event, APlayerController* Ctrl)
{
	UE_LOG(LogTemp, Warning, TEXT("All Fitting"));
	ChangeFittingMethod(EFunctionFittingMethod::All);
}

void AGAMES102HW3PlayerController::ClearCanvasEvent(FKey Key, EInputEvent Event, APlayerController* Ctrl)
{
	ClearCanvas();
}

void AGAMES102HW3PlayerController::ResampleFunction()
{
	Canvas2D->ClearDrawing();
	Canvas2D->DrawPoints();

	TArray<FParametrizationBase*> EnabledParametrizations;
	TArray<EParametrizationMethod> EnabledPMethods;
	TArray<TArray<TTuple<double, double> >*> PointsTXs;
	TArray<TArray<TTuple<double, double> >*> PointsTYs;
	switch (ParametrizationMethod) {
	case EParametrizationMethod::All:
	case EParametrizationMethod::Uniform:
	{
		EnabledPMethods.Add(EParametrizationMethod::Uniform);
		EnabledParametrizations.Add(Parametrizations[EParametrizationMethod::Uniform].Get());
		Canvas3DTo2D(static_cast<FUniformParametrization*>(EnabledParametrizations.Last())->Params.InPointsXY,
			Canvas2D->DisplayPoints[0].Array, false);
		PointsTXs.Add(&static_cast<FUniformParametrization*>(EnabledParametrizations.Last())->Params.OutPointsParamX);
		PointsTYs.Add(&static_cast<FUniformParametrization*>(EnabledParametrizations.Last())->Params.OutPointsParamY);
		if (ParametrizationMethod != EParametrizationMethod::All) {
			break;
		}
	}
	case EParametrizationMethod::Chordal:
	{
		EnabledPMethods.Add(EParametrizationMethod::Chordal);
		EnabledParametrizations.Add(Parametrizations[EParametrizationMethod::Chordal].Get());
		Canvas3DTo2D(static_cast<FChordalParametrization*>(EnabledParametrizations.Last())->Params.InPointsXY,
			Canvas2D->DisplayPoints[0].Array, false);
		PointsTXs.Add(&static_cast<FChordalParametrization*>(EnabledParametrizations.Last())->Params.OutPointsParamX);
		PointsTYs.Add(&static_cast<FChordalParametrization*>(EnabledParametrizations.Last())->Params.OutPointsParamY);
		if (ParametrizationMethod != EParametrizationMethod::All) {
			break;
		}
	}
	case EParametrizationMethod::Centripetal:
	{
		EnabledPMethods.Add(EParametrizationMethod::Centripetal);
		EnabledParametrizations.Add(Parametrizations[EParametrizationMethod::Centripetal].Get());
		Canvas3DTo2D(static_cast<FCentripetalParametrization*>(EnabledParametrizations.Last())->Params.InPointsXY,
			Canvas2D->DisplayPoints[0].Array, false);
		PointsTXs.Add(&static_cast<FCentripetalParametrization*>(EnabledParametrizations.Last())->Params.OutPointsParamX);
		PointsTYs.Add(&static_cast<FCentripetalParametrization*>(EnabledParametrizations.Last())->Params.OutPointsParamY);
		if (ParametrizationMethod != EParametrizationMethod::All) {
			break;
		}
	}
	case EParametrizationMethod::Foley:
	{
		EnabledPMethods.Add(EParametrizationMethod::Foley);
		EnabledParametrizations.Add(Parametrizations[EParametrizationMethod::Foley].Get());
		Canvas3DTo2D(static_cast<FFoleyParametrization*>(EnabledParametrizations.Last())->Params.InPointsXY,
			Canvas2D->DisplayPoints[0].Array, false);
		PointsTXs.Add(&static_cast<FFoleyParametrization*>(EnabledParametrizations.Last())->Params.OutPointsParamX);
		PointsTYs.Add(&static_cast<FFoleyParametrization*>(EnabledParametrizations.Last())->Params.OutPointsParamY);
		if (ParametrizationMethod != EParametrizationMethod::All) {
			break;
		}
	}
	}
	if (EnabledParametrizations.Num() == 0 || PointsTXs.Num() == 0 || PointsTYs.Num() == 0) {
		return;
	}
	for (int32 PIndex = 0; PIndex < EnabledParametrizations.Num(); ++PIndex) {
		PointsTXs[PIndex]->Empty(PointsTXs[PIndex]->Max());
		PointsTYs[PIndex]->Empty(PointsTYs[PIndex]->Max());
		EnabledParametrizations[PIndex]->Parametrize(0., 1.);

		TArray<EFunctionFittingMethod> EnabledMethods;
		switch (FittingMethod) {
		case EFunctionFittingMethod::All:
		case EFunctionFittingMethod::PolynomialFitting:
		{
			EnabledMethods.Add(EFunctionFittingMethod::PolynomialFitting);
			FPolynomialFittingParams& Params0 =
				StaticCastSharedPtr<FPolynomialFitting>(Fittings[EFunctionFittingMethod::PolynomialFitting].Get<0>())->Params;
			FPolynomialFittingParams& Params1 =
				StaticCastSharedPtr<FPolynomialFitting>(Fittings[EFunctionFittingMethod::PolynomialFitting].Get<1>())->Params;
			//Canvas3DTo2D(Params0.InPoints, Canvas2D->DisplayPoints[0].Array, false);
			//Canvas3DTo2D(Params1.InPoints, Canvas2D->DisplayPoints[0].Array, false);
			Params0.InPoints = *PointsTXs[PIndex];
			Params1.InPoints = *PointsTYs[PIndex];
			if (FittingMethod != EFunctionFittingMethod::All) {
				break;
			}
		}
		case EFunctionFittingMethod::GaussianFitting:
		{
			EnabledMethods.Add(EFunctionFittingMethod::GaussianFitting);
			FGaussianFittingParams& Params0 =
				StaticCastSharedPtr<FGaussianFitting>(Fittings[EFunctionFittingMethod::GaussianFitting].Get<0>())->Params;
			FGaussianFittingParams& Params1 =
				StaticCastSharedPtr<FGaussianFitting>(Fittings[EFunctionFittingMethod::GaussianFitting].Get<1>())->Params;
			Params0.InSigma = ParamsInput.GaussianSigma;
			Params1.InSigma = ParamsInput.GaussianSigma;
			//Canvas3DTo2D(Params0.InPoints, Canvas2D->DisplayPoints[0].Array, false);
			//Canvas3DTo2D(Params1.InPoints, Canvas2D->DisplayPoints[0].Array, false);
			Params0.InPoints = *PointsTXs[PIndex];
			Params1.InPoints = *PointsTYs[PIndex];
			if (FittingMethod != EFunctionFittingMethod::All) {
				break;
			}
		}
		case EFunctionFittingMethod::PolynomialRegression:
		{
			EnabledMethods.Add(EFunctionFittingMethod::PolynomialRegression);
			FPolynomialRegressionParams& Params0 =
				StaticCastSharedPtr<FPolynomialRegression>(Fittings[EFunctionFittingMethod::PolynomialRegression].Get<0>())->Params;
			FPolynomialRegressionParams& Params1 =
				StaticCastSharedPtr<FPolynomialRegression>(Fittings[EFunctionFittingMethod::PolynomialRegression].Get<1>())->Params;
			Params0.InSpanNum = ParamsInput.RegressionSpan;
			Params1.InSpanNum = ParamsInput.RegressionSpan;
			//Canvas3DTo2D(Params0.InPoints, Canvas2D->DisplayPoints[0].Array, false);
			//Canvas3DTo2D(Params1.InPoints, Canvas2D->DisplayPoints[0].Array, false);
			Params0.InPoints = *PointsTXs[PIndex];
			Params1.InPoints = *PointsTYs[PIndex];
			if (FittingMethod != EFunctionFittingMethod::All) {
				break;
			}
		}
		case EFunctionFittingMethod::RidgeRegression:
		{
			EnabledMethods.Add(EFunctionFittingMethod::RidgeRegression);
			FRidgeRegressionParams& Params0 =
				StaticCastSharedPtr<FRidgeRegression>(Fittings[EFunctionFittingMethod::RidgeRegression].Get<0>())->Params;
			FRidgeRegressionParams& Params1 =
				StaticCastSharedPtr<FRidgeRegression>(Fittings[EFunctionFittingMethod::RidgeRegression].Get<1>())->Params;
			Params0.InSpanNum = ParamsInput.RegressionSpan;
			Params1.InSpanNum = ParamsInput.RegressionSpan;
			Params0.InLambda = ParamsInput.RidgeLambda;
			Params1.InLambda = ParamsInput.RidgeLambda;
			//Canvas3DTo2D(Params0.InPoints, Canvas2D->DisplayPoints[0].Array, false);
			//Canvas3DTo2D(Params1.InPoints, Canvas2D->DisplayPoints[0].Array, false);
			Params0.InPoints = *PointsTXs[PIndex];
			Params1.InPoints = *PointsTYs[PIndex];
			if (FittingMethod != EFunctionFittingMethod::All) {
				break;
			}
		}
		}

		for (EFunctionFittingMethod MLayer : EnabledMethods) {
			TSharedPtr<FFunctionFittingBase>& Fitting0 = Fittings[MLayer].Get<0>();
			TSharedPtr<FFunctionFittingBase>& Fitting1 = Fittings[MLayer].Get<1>();
			Fitting0->Fit();
			Fitting1->Fit();
			TArray<TTuple<double, double> > SampleResultsX;
			Fitting0->Sample(SampleResultsX, 0., 1., SamplePointDT);
			TArray<TTuple<double, double> > SampleResultsY;
			Fitting1->Sample(SampleResultsY, 0., 1., SamplePointDT);
			TArray<TTuple<double, double> > SampleResults(SampleResultsY);
			for (int32 i = 0; i < SampleResultsX.Num(); ++i) {
				SampleResults[i].Get<0>() = SampleResultsX[i].Get<1>();
			}
			int32 CurLayer = (int32)EnabledPMethods[PIndex];//PIndex * EnabledMethods.Num() + (int32)MLayer;
			Canvas2D->DisplayLines[CurLayer].Array.Empty(MaxSamplePointsNum);
			Canvas2DTo3D(Canvas2D->DisplayLines[CurLayer].Array, SampleResults, true);
			Canvas2D->DrawLines(CurLayer);
		}
	}
}

void AGAMES102HW3PlayerController::AddCurvePointEvent(FKey Key, FVector2D MouseScreenPos, EInputEvent InputEvent, APlayerController* Ctrl)
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
