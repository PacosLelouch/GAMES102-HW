// Copyright Epic Games, Inc. All Rights Reserved.


#include "GAMES102HW9GameModeBase.h"
#include "Engine/World.h"
#include "Blueprint/UserWidget.h"
#include "UI/ControlWidget.h"
#include "GAMES102HW9PlayerController.h"

AGAMES102HW9GameModeBase::AGAMES102HW9GameModeBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PlayerControllerClass = AGAMES102HW9PlayerController::StaticClass();
}

void AGAMES102HW9GameModeBase::BeginPlay()
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
