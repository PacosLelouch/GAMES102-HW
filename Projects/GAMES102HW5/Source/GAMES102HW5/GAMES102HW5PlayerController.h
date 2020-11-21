// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CGDemoPlayerController2D.h"
#include "Subdivision/SubdivisionCollection.h"
#include "GAMES102HW5PlayerController.generated.h"

USTRUCT(BlueprintType)
struct GAMES102HW5_API FSubdivisionParamsInput
{
	GENERATED_BODY()
public:
	UPROPERTY(BlueprintReadWrite)
	float Alpha = 0.0625;

	UPROPERTY(BlueprintReadWrite)
	int32 Num = 2;

	UPROPERTY(BlueprintReadWrite)
	bool bClosed = false;
};

/**
 * 
 */
UCLASS()
class GAMES102HW5_API AGAMES102HW5PlayerController : public ACGDemoPlayerController2D
{
	GENERATED_BODY()
public:
	AGAMES102HW5PlayerController(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

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
	void ChangeSubdivisionMethod(ESubdivisionMethod Method);


	UFUNCTION(BlueprintCallable)
	void ClearCanvas();

	UFUNCTION(BlueprintCallable)
	void OnParamsInputChanged();

	virtual void AddCurvePoint(const FVector& HitPoint);

public:
	//double SamplePointDT = 0.01;

	//int32 MaxSamplePointsNum = 0;

	TArray<FVector> CurvePoints;

	UPROPERTY(BlueprintReadOnly)
	ESubdivisionMethod SubdivisionMethod;

	TMap<ESubdivisionMethod, TSharedPtr<FSubdivisionBase> > Subdivisions;

public:
	UPROPERTY(BlueprintReadWrite)
	FSubdivisionParamsInput ParamsInput;

private:
	UFUNCTION()
	void AddCurvePointEvent(FKey Key, FVector2D MouseScreenPos, EInputEvent InputEvent, APlayerController* Ctrl);

	UFUNCTION()
	void ChangeSubdivisionMethodToChaikin(FKey Key, EInputEvent Event, APlayerController* Ctrl);

	UFUNCTION()
	void ChangeSubdivisionMethodToThreeDegreeBSpline(FKey Key, EInputEvent Event, APlayerController* Ctrl);

	UFUNCTION()
	void ChangeSubdivisionMethodToFourPointInterpolation(FKey Key, EInputEvent Event, APlayerController* Ctrl);

	UFUNCTION()
	void ClearCanvasEvent(FKey Key, EInputEvent Event, APlayerController* Ctrl);

	void ResampleCurve();
};
