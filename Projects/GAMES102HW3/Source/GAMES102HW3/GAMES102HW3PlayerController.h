// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CGDemoPlayerController2D.h"
#include "FunctionFitting/FunctionFitting.h"
#include "Parametrization/Parametrization.h"
#include "GAMES102HW3PlayerController.generated.h"

USTRUCT(BlueprintType)
struct GAMES102HW3_API FFittingParamsInput
{
	GENERATED_BODY()
public:
	UPROPERTY(BlueprintReadWrite)
	float GaussianSigma = 5;

	UPROPERTY(BlueprintReadWrite)
	int32 RegressionSpan = 5;

	UPROPERTY(BlueprintReadWrite)
	float RidgeLambda = 1.;
};

/**
 * 
 */
UCLASS()
class GAMES102HW3_API AGAMES102HW3PlayerController : public ACGDemoPlayerController2D
{
	GENERATED_BODY()
public:
	AGAMES102HW3PlayerController(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void BeginPlay() override;

	virtual void BindOnRightMouseButtonReleased() override;
	virtual void BindOnCtrlAndKey1Released() override;
	virtual void BindOnCtrlAndKey2Released() override;
	virtual void BindOnCtrlAndKey3Released() override;
	virtual void BindOnCtrlAndKey4Released() override;
	virtual void BindOnCtrlAndKey5Released() override;
	virtual void BindOnCtrlAndKey0Released() override;

public:
	UFUNCTION(BlueprintCallable)
	void ChangeFittingMethod(EFunctionFittingMethod Method);

	UFUNCTION(BlueprintCallable)
	void ChangeParametrizationMethod(EParametrizationMethod Method);


	UFUNCTION(BlueprintCallable)
	void ClearCanvas();

	UFUNCTION(BlueprintCallable)
	void OnParamsInputChanged();

	virtual void AddCurvePoint(const FVector& HitPoint);

public:
	double SamplePointDT = 0.01;

	int32 MaxSamplePointsNum = 0;

	TArray<FVector> FunctionPoints;

	UPROPERTY(BlueprintReadOnly)
	EFunctionFittingMethod FittingMethod;

	UPROPERTY(BlueprintReadOnly)
	EParametrizationMethod ParametrizationMethod;

	TMap<EFunctionFittingMethod, TPair<TSharedPtr<FFunctionFittingBase>, TSharedPtr<FFunctionFittingBase> > > Fittings;

	TMap<EParametrizationMethod, TSharedPtr<FParametrizationBase> > Parametrizations;

public:
	UPROPERTY(BlueprintReadWrite)
	FFittingParamsInput ParamsInput;

private:
	UFUNCTION()
	void AddCurvePointEvent(FKey Key, FVector2D MouseScreenPos, EInputEvent InputEvent, APlayerController* Ctrl);

	UFUNCTION()
	void ChangeFittingMethodToPolynomialFitting(FKey Key, EInputEvent Event, APlayerController* Ctrl);

	UFUNCTION()
	void ChangeFittingMethodToGaussianFitting(FKey Key, EInputEvent Event, APlayerController* Ctrl);

	UFUNCTION()
	void ChangeFittingMethodToPolynomialRegression(FKey Key, EInputEvent Event, APlayerController* Ctrl);

	UFUNCTION()
	void ChangeFittingMethodToRidgeFitting(FKey Key, EInputEvent Event, APlayerController* Ctrl);

	UFUNCTION()
	void ChangeFittingMethodToAllFitting(FKey Key, EInputEvent Event, APlayerController* Ctrl);

	UFUNCTION()
	void ClearCanvasEvent(FKey Key, EInputEvent Event, APlayerController* Ctrl);

	void ResampleFunction();
};
