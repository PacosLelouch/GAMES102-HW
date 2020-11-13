// Fill out your copyright notice in the Description page of Project Settings.



#include "GAMES102HW4GameModeBase.h"
#include "BezierStringTest/BezierStringTestPlayerController.h"

AGAMES102HW4GameModeBase::AGAMES102HW4GameModeBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PlayerControllerClass = ABezierStringTestPlayerController::StaticClass();
}