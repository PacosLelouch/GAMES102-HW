// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CGDemoPlayerController3D.h"
#include "GAMES102HW6PlayerController.generated.h"

class AMeshWrapper;

UENUM(BlueprintType)
enum class EDisplayType : uint8
{
	White,
	Normal,
	MeanCurvature,
	GaussianCurvature,
};

USTRUCT(BlueprintType)
struct GAMES102HW6_API FInputParams
{
	GENERATED_BODY()
public:
	UPROPERTY(BlueprintReadWrite)
	float Lambda = 0.25f;

	UPROPERTY(BlueprintReadWrite)
	int32 LoopCount = 10;
};

/**
 * 
 */
UCLASS()
class GAMES102HW6_API AGAMES102HW6PlayerController : public ACGDemoPlayerController3D
{
	GENERATED_BODY()
public:
	DECLARE_DELEGATE(FOnTimerHandleValidityChanged);

	AGAMES102HW6PlayerController(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void BeginPlay() override;

	//virtual void BindOnRightMouseButtonReleased() override;
	virtual void BindOnCtrlAndKey1Released() override;
	virtual void BindOnCtrlAndKey2Released() override;
	virtual void BindOnCtrlAndKey3Released() override;
	//virtual void BindOnCtrlAndKey4Released() override;
	//virtual void BindOnCtrlAndKey5Released() override;
	virtual void BindOnCtrlAndKey0Released() override;

public:
	UFUNCTION(BlueprintCallable)
	void LoadMesh(UStaticMesh* Mesh);

	UFUNCTION(BlueprintCallable)
	void ProcessMesh();

	UFUNCTION(BlueprintCallable)
	void DisplayMesh(EDisplayType InDisplayType);

	UFUNCTION(BlueprintCallable)
	void OnInputParamsChanged();


public:

	TArray<FVector> CurvePoints;

public:
	UPROPERTY(BlueprintReadWrite)
	EDisplayType DisplayType = EDisplayType::White;

	UPROPERTY(BlueprintReadWrite)
	FInputParams InputParams;

	UPROPERTY(BlueprintReadWrite)
	AMeshWrapper* MeshWrapper = nullptr;

public:
	FTimerHandle ProceduralHandler;

	bool bProcedural = false;

	int32 ProceduralCount = 0;

	FOnTimerHandleValidityChanged OnProceduralHandlerValidityChanged;

private:
	UFUNCTION()
	void DisplayWhiteEvent(FKey Key, EInputEvent InputEvent, APlayerController* Ctrl);

	UFUNCTION()
	void ProcessMeshEvent(FKey Key, EInputEvent InputEvent, APlayerController* Ctrl);

};
