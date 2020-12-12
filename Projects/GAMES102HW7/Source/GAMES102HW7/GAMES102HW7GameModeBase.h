// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "CGDemoGameModeBase.h"
#include "GAMES102HW7GameModeBase.generated.h"

/**
 * 
 */
UCLASS()
class GAMES102HW7_API AGAMES102HW7GameModeBase : public ACGDemoGameModeBase
{
	GENERATED_BODY()
public:
	AGAMES102HW7GameModeBase(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

public:
	virtual void BeginPlay() override;

public:
	UPROPERTY(EditAnywhere)
	TSubclassOf<class UControlWidget> WidgetClass;
};
