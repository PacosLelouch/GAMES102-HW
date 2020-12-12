// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CGDemoPlayerController.h"
#include "CGDemoPlayerController3D.generated.h"

class UStaticMesh;

/**
 *
 */
UCLASS()
class CGDEMOSETTINGS_API ACGDemoPlayerController3D : public ACGDemoPlayerController
{
	GENERATED_BODY()
public:
	ACGDemoPlayerController3D(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void BeginPlay() override;
};
