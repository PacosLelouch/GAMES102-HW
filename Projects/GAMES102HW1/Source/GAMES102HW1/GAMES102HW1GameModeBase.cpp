// Copyright Epic Games, Inc. All Rights Reserved.


#include "GAMES102HW1GameModeBase.h"
#include "GAMES102HW1PlayerController.h"

AGAMES102HW1GameModeBase::AGAMES102HW1GameModeBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PlayerControllerClass = AGAMES102HW1PlayerController::StaticClass();
}
