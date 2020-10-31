// Copyright Epic Games, Inc. All Rights Reserved.


#include "GAMES102HW3GameModeBase.h"
#include "GAMES102HW3PlayerController.h"

AGAMES102HW3GameModeBase::AGAMES102HW3GameModeBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PlayerControllerClass = AGAMES102HW3PlayerController::StaticClass();
}

void AGAMES102HW3GameModeBase::BeginPlay()
{
	Super::BeginPlay();
	//UWorld* World = GetWorld();
	//if (World) {
	//	APlayerController* Ctrl = World->GetFirstPlayerController();
	//	if (Ctrl) {
	//		CreateWidget(Ctrl, UControlWidget::StaticClass())->AddToViewport();
	//	}
	//}
}
