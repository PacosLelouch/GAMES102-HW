// Copyright Epic Games, Inc. All Rights Reserved.


#include "GAMES102HW7GameModeBase.h"
#include "Engine/World.h"
#include "Blueprint/UserWidget.h"
#include "UI/ControlWidget.h"
#include "GAMES102HW7PlayerController.h"

AGAMES102HW7GameModeBase::AGAMES102HW7GameModeBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PlayerControllerClass = AGAMES102HW7PlayerController::StaticClass();
}

void AGAMES102HW7GameModeBase::BeginPlay()
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
