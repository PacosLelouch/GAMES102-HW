// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CGDemoPlayerController2D.h"
#include "FunctionFitting/FunctionFitting.h"
#include "GAMES102HW1PlayerController.generated.h"

USTRUCT(BlueprintType)
struct GAMES102HW1_API FFittingParamsInput
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
class GAMES102HW1_API AGAMES102HW1PlayerController : public ACGDemoPlayerController2D
{
	GENERATED_BODY()
public:
	AGAMES102HW1PlayerController(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void BeginPlay() override;

	virtual void BindOnRightMouseButtonReleased() override;
	virtual void BindOnKey1Released() override;
	virtual void BindOnKey2Released() override;
	virtual void BindOnKey3Released() override;
	virtual void BindOnKey4Released() override;
	virtual void BindOnKey5Released() override;
	virtual void BindOnKey0Released() override;

public:
	UFUNCTION(BlueprintCallable)
	void ChangeFittingMethod(EFunctionFittingMethod Method);


	UFUNCTION(BlueprintCallable)
	void ClearCanvas();

	UFUNCTION(BlueprintCallable)
	void OnParamsInputChanged();

public:
	double SamplePointDY = 1;

	int32 MaxSamplePointsNum = 0;

	TArray<FVector> FunctionPoints;

	UPROPERTY(BlueprintReadOnly)
	EFunctionFittingMethod FittingMethod;

	TMap<EFunctionFittingMethod, TSharedPtr<FFunctionFittingBase> > Fittings;

public:
	UPROPERTY(BlueprintReadWrite)
	FFittingParamsInput ParamsInput;

private:
	UFUNCTION()
	void AddFunctionPoint(FKey Key, FVector2D MouseScreenPos, EInputEvent InputEvent, APlayerController* Ctrl);

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
