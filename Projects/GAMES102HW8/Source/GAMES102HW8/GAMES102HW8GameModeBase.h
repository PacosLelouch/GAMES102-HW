// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "CGDemoGameModeBase.h"
#include "GAMES102HW8GameModeBase.generated.h"

/**
 * 
 */
UCLASS()
class GAMES102HW8_API AGAMES102HW8GameModeBase : public ACGDemoGameModeBase
{
	GENERATED_BODY()
public:
	AGAMES102HW8GameModeBase(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

public:
	virtual void BeginPlay() override;

public:
	UPROPERTY(EditAnywhere)
	TSubclassOf<class UControlWidget> WidgetClass;
};
