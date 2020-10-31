// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "CGDemoGameModeBase.h"
#include "GAMES102HW3GameModeBase.generated.h"

/**
 * 
 */
UCLASS()
class GAMES102HW3_API AGAMES102HW3GameModeBase : public ACGDemoGameModeBase
{
	GENERATED_BODY()
public:
	AGAMES102HW3GameModeBase(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

public:
	virtual void BeginPlay() override;
};
