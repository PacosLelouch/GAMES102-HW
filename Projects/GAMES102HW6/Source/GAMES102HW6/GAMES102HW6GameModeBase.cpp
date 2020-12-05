// Copyright Epic Games, Inc. All Rights Reserved.


#include "GAMES102HW6GameModeBase.h"
#include "Engine/World.h"
#include "Blueprint/UserWidget.h"
#include "UI/ControlWidget.h"
#include "GAMES102HW6PlayerController.h"

AGAMES102HW6GameModeBase::AGAMES102HW6GameModeBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PlayerControllerClass = AGAMES102HW6PlayerController::StaticClass();
}

void AGAMES102HW6GameModeBase::BeginPlay()
{
	Super::BeginPlay();
	UWorld* World = GetWorld();
	if (World->IsValidLowLevel() && WidgetClass->IsValidLowLevel()) {
		APlayerController* Ctrl = World->GetFirstPlayerController();
		if (Ctrl) {
			CreateWidget(Ctrl, WidgetClass)->AddToViewport();
		}
	}
}
