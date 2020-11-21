// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ControlWidget.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/SpinBox.h"
#include "Components/CheckBox.h"
#include "SubdivisionControlWidget.generated.h"

UCLASS(BlueprintType, Blueprintable)
class USubdivisionControlWidget : public UControlWidget
{
	GENERATED_BODY()
public:

	virtual void BindValues() override;

	virtual void BindEventsRuntime() override;

	virtual void BindValuesRuntime() override;

public:
	UFUNCTION(BlueprintGetter)
	FText GetTextCurrentMethod();

	UFUNCTION(BlueprintGetter)
	FText GetTextChaikin();

	UFUNCTION(BlueprintGetter)
	FText GetTextThreeDegreeBSpline();

	UFUNCTION(BlueprintGetter)
	FText GetTextFourPointInterpolation();

	UFUNCTION(BlueprintGetter)
	FText GetTextAll();

	UFUNCTION(BlueprintGetter)
	FText GetTextClosed();

	UFUNCTION(BlueprintGetter)
	FText GetTextAlpha();

	UFUNCTION(BlueprintGetter)
	FText GetTextSubdivisionNum();

	UFUNCTION(BlueprintGetter)
	float GetSpinBoxAlpha();

	UFUNCTION(BlueprintGetter)
	float GetSpinBoxSubdivisionNum();

public:
	UFUNCTION(BlueprintCallable)
	void CommitSpinBoxAlpha(float InValue, ETextCommit::Type CommitMethod);

	UFUNCTION(BlueprintCallable)
	void CommitSpinBoxSubdivisionNum(float InValue, ETextCommit::Type CommitMethod);

	UFUNCTION(BlueprintCallable)
	void ReleaseButtonChaikin();

	UFUNCTION(BlueprintCallable)
	void ReleaseButtonThreeDegreeBSpline();

	UFUNCTION(BlueprintCallable)
	void ReleaseButtonFourPointInterpolation();

	UFUNCTION(BlueprintCallable)
	void ReleaseButtonAll();

	UFUNCTION(BlueprintCallable)
	void ReleaseButtonClear();

	UFUNCTION(BlueprintCallable)
	void ChangeCheckBoxClosedState(bool bIsChecked);

public:

	UPROPERTY(BlueprintReadOnly, Category = "Widget|Button", Meta = (BindWidget))
	UButton* Button_Clear;

	UPROPERTY(BlueprintReadOnly, Category = "Widget|Button", Meta = (BindWidget))
	UButton* Button_Chaikin;

	UPROPERTY(BlueprintReadOnly, Category = "Widget|Button", Meta = (BindWidget))
	UButton* Button_ThreeDegreeBSpline;

	UPROPERTY(BlueprintReadOnly, Category = "Widget|Button", Meta = (BindWidget))
	UButton* Button_FourPointInterpolation;

	UPROPERTY(BlueprintReadOnly, Category = "Widget|Button", Meta = (BindWidget))
	UButton* Button_All;

	UPROPERTY(BlueprintReadOnly, Category = "Widget|CheckBox", Meta = (BindWidget))
	UCheckBox* CheckBox_Closed;

	UPROPERTY(BlueprintReadOnly, Category = "Widget|SpinBox", Meta = (BindWidget))
	USpinBox* SpinBox_Alpha;

	UPROPERTY(BlueprintReadOnly, Category = "Widget|SpinBox", Meta = (BindWidget))
	USpinBox* SpinBox_SubdivisionNum;

	UPROPERTY(BlueprintReadOnly, Category = "Widget|Text", Meta = (BindWidget))
	UTextBlock* Text_CurrentMethod;

	UPROPERTY(BlueprintReadOnly, Category = "Widget|Text", Meta = (BindWidget))
	UTextBlock* Text_Clear;

	UPROPERTY(BlueprintReadOnly, Category = "Widget|Text", Meta = (BindWidget))
	UTextBlock* Text_Closed;

	UPROPERTY(BlueprintReadOnly, Category = "Widget|Text", Meta = (BindWidget))
	UTextBlock* Text_Chaikin;

	UPROPERTY(BlueprintReadOnly, Category = "Widget|Text", Meta = (BindWidget))
	UTextBlock* Text_ThreeDegreeBSpline;

	UPROPERTY(BlueprintReadOnly, Category = "Widget|Text", Meta = (BindWidget))
	UTextBlock* Text_FourPointInterpolation;

	UPROPERTY(BlueprintReadOnly, Category = "Widget|Text", Meta = (BindWidget))
	UTextBlock* Text_All;

	UPROPERTY(BlueprintReadOnly, Category = "Widget|Text", Meta = (BindWidget))
	UTextBlock* Text_Alpha;

	UPROPERTY(BlueprintReadOnly, Category = "Widget|Text", Meta = (BindWidget))
	UTextBlock* Text_SubdivisionNum;
};