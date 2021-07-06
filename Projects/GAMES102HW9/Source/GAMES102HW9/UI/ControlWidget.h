// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "../GAMES102HW9PlayerController.h"
#include "ControlWidget.generated.h"

UCLASS(BlueprintType)
class UControlWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	virtual bool Initialize() override;
	
	virtual void NativePreConstruct() override;

	virtual void NativeConstruct() override;

	virtual void BindValues();

	virtual void BindEventsRuntime();

	virtual void BindValuesRuntime();

public:
	UPROPERTY(BlueprintReadWrite, Category = "Controller")
	AGAMES102HW9PlayerController* Controller;
};
