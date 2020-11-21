// Copyright Epic Games, Inc. All Rights Reserved.


#include "GAMES102HW5GameModeBase.h"
#include "Engine/World.h"
#include "Blueprint/UserWidget.h"
#include "UI/ControlWidget.h"
#include "GAMES102HW5PlayerController.h"

AGAMES102HW5GameModeBase::AGAMES102HW5GameModeBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PlayerControllerClass = AGAMES102HW5PlayerController::StaticClass();
}

void AGAMES102HW5GameModeBase::BeginPlay()
{
	Super::BeginPlay();
	UWorld* World = GetWorld();
	if (World) {
		APlayerController* Ctrl = World->GetFirstPlayerController();
		if (Ctrl) {
			CreateWidget(Ctrl, WidgetClass)->AddToViewport();
		}
	}
}
