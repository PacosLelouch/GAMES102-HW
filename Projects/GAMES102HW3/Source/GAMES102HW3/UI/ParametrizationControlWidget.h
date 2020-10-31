// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ControlWidget.h"
#include "ParametrizationControlWidget.generated.h"

UCLASS(BlueprintType, Blueprintable)
class UParametrizationControlWidget : public UControlWidget
{
	GENERATED_BODY()
public:

	virtual void BindValues() override;

	virtual void BindEvents() override;

public:
	UFUNCTION(BlueprintGetter)
	FText GetTextCurrentMethod();

	UFUNCTION(BlueprintGetter)
	FText GetTextUniform();

	UFUNCTION(BlueprintGetter)
	FText GetTextChordal();

	UFUNCTION(BlueprintGetter)
	FText GetTextCentripetal();

	UFUNCTION(BlueprintGetter)
	FText GetTextFoley();

	UFUNCTION(BlueprintGetter)
	FText GetTextAll();

public:

	UFUNCTION(BlueprintCallable)
	void ReleaseButtonUniform();

	UFUNCTION(BlueprintCallable)
	void ReleaseButtonChordal();

	UFUNCTION(BlueprintCallable)
	void ReleaseButtonCentripetal();

	UFUNCTION(BlueprintCallable)
	void ReleaseButtonFoley();

	UFUNCTION(BlueprintCallable)
	void ReleaseButtonAll();

public:
	UPROPERTY(BlueprintReadOnly, Category = "Widget|Text", Meta = (BindWidget))
	UTextBlock* TextCurrentMethod;

	UPROPERTY(BlueprintReadOnly, Category = "Widget|Text", Meta = (BindWidget))
	UTextBlock* TextUniform;

	UPROPERTY(BlueprintReadOnly, Category = "Widget|Text", Meta = (BindWidget))
	UTextBlock* TextChordal;

	UPROPERTY(BlueprintReadOnly, Category = "Widget|Text", Meta = (BindWidget))
	UTextBlock* TextCentripetal;

	UPROPERTY(BlueprintReadOnly, Category = "Widget|Text", Meta = (BindWidget))
	UTextBlock* TextFoley;

	UPROPERTY(BlueprintReadOnly, Category = "Widget|Text", Meta = (BindWidget))
	UTextBlock* TextAll;

	UPROPERTY(BlueprintReadOnly, Category = "Widget|Button", Meta = (BindWidget))
	UButton* ButtonUniform;

	UPROPERTY(BlueprintReadOnly, Category = "Widget|Button", Meta = (BindWidget))
	UButton* ButtonChordal;

	UPROPERTY(BlueprintReadOnly, Category = "Widget|Button", Meta = (BindWidget))
	UButton* ButtonCentripetal;

	UPROPERTY(BlueprintReadOnly, Category = "Widget|Button", Meta = (BindWidget))
	UButton* ButtonFoley;

	UPROPERTY(BlueprintReadOnly, Category = "Widget|Button", Meta = (BindWidget))
	UButton* ButtonAll;

};