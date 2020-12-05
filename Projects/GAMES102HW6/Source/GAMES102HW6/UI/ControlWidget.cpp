// Fill out your copyright notice in the Description page of Project Settings.

#include "ControlWidget.h"

bool UControlWidget::Initialize()
{
	bool ReturnValue = Super::Initialize();
	return ReturnValue;
}

void UControlWidget::NativePreConstruct()
{
	Super::NativePreConstruct();
	BindValues();
}

void UControlWidget::NativeConstruct()
{
	Super::NativeConstruct();
	UWorld* World = GetWorld();
	if (World) {
		APlayerController* Ctrl = World->GetFirstPlayerController();
		if (Ctrl) {
			Controller = Cast<AGAMES102HW6PlayerController>(Ctrl);
		}
	}
	BindValuesRuntime();
	BindEventsRuntime();
}

void UControlWidget::BindValues()
{
}

void UControlWidget::BindEventsRuntime()
{
}

void UControlWidget::BindValuesRuntime()
{
}
