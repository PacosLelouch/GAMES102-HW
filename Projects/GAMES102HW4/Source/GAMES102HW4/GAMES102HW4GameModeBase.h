// Fill out your copyright notice in the Description page of Project Settings.


#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "CGDemoGameModeBase.h"
#include "GAMES102HW4GameModeBase.generated.h"

/**
 * 
 */
UCLASS()
class GAMES102HW4_API AGAMES102HW4GameModeBase : public ACGDemoGameModeBase
{
	GENERATED_BODY()
public:
	AGAMES102HW4GameModeBase(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
};
