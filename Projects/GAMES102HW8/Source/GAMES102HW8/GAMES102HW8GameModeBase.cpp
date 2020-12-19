// Copyright Epic Games, Inc. All Rights Reserved.


#include "GAMES102HW8GameModeBase.h"
#include "Engine/World.h"
#include "Blueprint/UserWidget.h"
#include "UI/ControlWidget.h"
#include "GAMES102HW8PlayerController.h"

AGAMES102HW8GameModeBase::AGAMES102HW8GameModeBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PlayerControllerClass = AGAMES102HW8PlayerController::StaticClass();
}

void AGAMES102HW8GameModeBase::BeginPlay()
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
