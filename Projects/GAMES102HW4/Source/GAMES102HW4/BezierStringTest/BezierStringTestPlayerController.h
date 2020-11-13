// Fill out your copyright notice in the Description page of Project Settings.


#pragma once

#include "CoreMinimal.h"
#include "Splines/BezierString.h"
#include "CGDemoPlayerController2D.h"
#include "BezierStringTestPlayerController.generated.h"

using FSpatialBezierString3 = typename TBezierString3<3>;

//USTRUCT(BlueprintType)
//struct CURVEBUILDERTEST_API FCurveBuilderTestParamsInput
//{
//	GENERATED_BODY()
//public:
//	UPROPERTY(BlueprintReadWrite)
//		float CurrentWeight = 1.;
//
//};

//UENUM(BlueprintType)
//enum class ESplineConcatType : uint8
//{
//	ToPoint,
//	ToCurve,
//};

UENUM(BlueprintType)
enum class ESelectedNodeCtrlPointType : uint8
{
	Previous,
	Current,
	Next,
};

/**
 *
 */
UCLASS()
class GAMES102HW4_API ABezierStringTestPlayerController : public ACGDemoPlayerController2D
{
	GENERATED_BODY()
public:
	ABezierStringTestPlayerController(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void BeginPlay() override;

	virtual void Tick(float Delta) override;

	virtual void BindOnLeftMouseButtonPressed() override;
	virtual void BindOnLeftMouseButtonReleased() override;
	virtual void BindOnRightMouseButtonReleased() override;
	virtual void BindOnCtrlAndKey1Released() override;
	virtual void BindOnCtrlAndKey2Released() override;
	virtual void BindOnCtrlAndKey3Released() override;
	virtual void BindOnCtrlAndKey4Released() override;
	virtual void BindOnCtrlAndKey5Released() override;
	virtual void BindOnCtrlAndKey0Released() override;
	virtual void BindOnEnterReleased() override;

public:

	UFUNCTION(BlueprintCallable)
		void RemakeBezierC2();

	UFUNCTION(BlueprintCallable)
		void FlipDisplayControlPoint();

	UFUNCTION(BlueprintCallable)
		void FlipDisplaySmallTangent();

	UFUNCTION(BlueprintCallable)
		void FlipDisplaySmallCurvature();

	UFUNCTION(BlueprintCallable)
		void ClearCanvas();

	UFUNCTION(BlueprintCallable)
		void OnParamsInputChanged();

	UFUNCTION(BlueprintCallable)
		void SplitSplineAtCenter();

public:
	double SamplePointDT = 1. / 128.;//1. / 8.; //1. / 256.;

	int32 MaxSamplePointsNum = 0;

	TArray<FVector> ControlPoints;

	UPROPERTY(BlueprintReadWrite)
		bool bDisplayControlPoint = true;

	UPROPERTY(BlueprintReadWrite)
		bool bDisplaySmallTangent = false;

	UPROPERTY(BlueprintReadWrite)
		bool bDisplaySmallCurvature = false;

	//UPROPERTY(BlueprintReadWrite)
	//FCurveBuilderTestParamsInput ParamsInput;
public:

protected:
	virtual void AddControlPoint(const FVector& HitPoint);

	virtual void ClearCanvasImpl();

	void ResampleCurve();

private:
	
	int32 ResampleBezierString(int32 FirstLineLayer = 0);

	UFUNCTION()
		void PressLeftMouseButton(FKey Key, FVector2D MouseScreenPos, EInputEvent InputEvent, APlayerController* Ctrl);

	UFUNCTION()
		void ReleaseLeftMouseButton(FKey Key, FVector2D MouseScreenPos, EInputEvent InputEvent, APlayerController* Ctrl);

	UFUNCTION()
		void AddControlPointEvent(FKey Key, FVector2D MouseScreenPos, EInputEvent InputEvent, APlayerController* Ctrl);

	UFUNCTION()
		void AddNewSplineEvent(FKey Key, EInputEvent InputEvent, APlayerController* Ctrl);

	UFUNCTION()
		void ClearCanvasEvent(FKey Key, EInputEvent InputEvent, APlayerController* Ctrl);

	UFUNCTION()
		void RemakeBezierC2Event(FKey Key, EInputEvent Event, APlayerController* Ctrl);

	UFUNCTION()
		void FlipDisplayControlPointEvent(FKey Key, EInputEvent Event, APlayerController* Ctrl);

	UFUNCTION()
		void FlipDisplaySmallTangentEvent(FKey Key, EInputEvent Event, APlayerController* Ctrl);

	UFUNCTION()
		void FlipDisplaySmallCurvatureEvent(FKey Key, EInputEvent Event, APlayerController* Ctrl);

	UFUNCTION()
		void SplitSplineAtCenterEvent(FKey Key, EInputEvent Event, APlayerController* Ctrl);

public:
	TArray<FSpatialBezierString3> Splines;

	TOptional<FVector> NearestPoint;

	FSpatialBezierString3::FPointNode* NearestNode = nullptr;

	FSpatialBezierString3::FPointNode* SelectedNode = nullptr;

	TOptional<ESelectedNodeCtrlPointType> HoldingPointType;

	bool bPressedLeftMouseButton = false;

	EEndPointContinuity NewPointContinuityInit = EEndPointContinuity::G2;

	TOptional<FTransform> FixedTransform;

protected:
	FVector ControlPointToHitPoint(const FVector& P);

	FVector HitPointToControlPoint(const FVector& P);
};
