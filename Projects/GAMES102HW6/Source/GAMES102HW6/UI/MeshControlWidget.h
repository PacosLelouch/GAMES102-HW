// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ControlWidget.h"
#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/SpinBox.h"
#include "Components/CheckBox.h"
#include "Components/UniformGridPanel.h"
#include "MeshControlWidget.generated.h"

UCLASS(BlueprintType, Blueprintable)
class UAssetEntryWidget : public UControlWidget
{
	GENERATED_BODY()
public:
	virtual void BindEventsRuntime() override;

	UPROPERTY(BlueprintReadOnly, Category = "Widget|Border", Meta = (BindWidget))
	UBorder* Border_Thumbnail;

	UPROPERTY(BlueprintReadOnly, Category = "Widget|Text", Meta = (BindWidget))
	UTextBlock* Text_Name;

	UPROPERTY(BlueprintReadOnly, Category = "Data")
	UObject* AssetObject = nullptr;

protected:
	UFUNCTION()
	FEventReply OnBorderDoubleClicked(FGeometry MyGeometry, const FPointerEvent& MouseEvent);
};

UCLASS(BlueprintType, Blueprintable)
class UMeshControlWidget : public UControlWidget
{
	GENERATED_BODY()
public:

	virtual void BindValues() override;

	virtual void BindEventsRuntime() override;

	virtual void BindValuesRuntime() override;

public:
	void ReadStaticMeshesFromAssets();

public:
	UFUNCTION(BlueprintGetter)
	FText GetTextCurrentDisplayType();

	UFUNCTION(BlueprintGetter)
	FText GetTextDisplayWhite();

	UFUNCTION(BlueprintGetter)
	FText GetTextDisplayNormal();

	UFUNCTION(BlueprintGetter)
	FText GetTextGaussianCurvature();

	UFUNCTION(BlueprintGetter)
	FText GetTextMeanCurvature();

	UFUNCTION(BlueprintGetter)
	FText GetTextProcess();

	UFUNCTION(BlueprintGetter)
	FText GetTextProcedural();

	UFUNCTION(BlueprintGetter)
	FText GetTextLambda();

	UFUNCTION(BlueprintGetter)
	FText GetTextLoopCount();

	UFUNCTION(BlueprintGetter)
	float GetSpinBoxLambda();

	UFUNCTION(BlueprintGetter)
	float GetSpinBoxLoopCount();

public:

	UFUNCTION(BlueprintCallable)
	void CommitSpinBoxLambda(float InValue, ETextCommit::Type CommitMethod);

	UFUNCTION(BlueprintCallable)
	void CommitSpinBoxSubdivisionNum(float InValue, ETextCommit::Type CommitMethod);

	UFUNCTION(BlueprintCallable)
	void ReleaseButtonDisplayWhite();

	UFUNCTION(BlueprintCallable)
	void ReleaseButtonDisplayNormal();

	UFUNCTION(BlueprintCallable)
	void ReleaseButtonDisplayGaussianCurvature();

	UFUNCTION(BlueprintCallable)
	void ReleaseButtonDisplayMeanCurvature();

	UFUNCTION(BlueprintCallable)
	void ReleaseButtonProcess();

	UFUNCTION(BlueprintCallable)
	void ChangeCheckBoxProceduralState(bool bIsChecked);

public:

	UPROPERTY(BlueprintReadOnly, Category = "Widget|Button", Meta = (BindWidget))
	UButton* Button_Process;

	UPROPERTY(BlueprintReadOnly, Category = "Widget|Button", Meta = (BindWidget))
	UButton* Button_DisplayWhite;

	UPROPERTY(BlueprintReadOnly, Category = "Widget|Button", Meta = (BindWidget))
	UButton* Button_DisplayNormal;

	UPROPERTY(BlueprintReadOnly, Category = "Widget|Button", Meta = (BindWidget))
	UButton* Button_DisplayGaussianCurvature;

	UPROPERTY(BlueprintReadOnly, Category = "Widget|Button", Meta = (BindWidget))
	UButton* Button_DisplayMeanCurvature;

	UPROPERTY(BlueprintReadOnly, Category = "Widget|CheckBox", Meta = (BindWidget))
	UCheckBox* CheckBox_Procedural;

	UPROPERTY(BlueprintReadOnly, Category = "Widget|SpinBox", Meta = (BindWidget))
	USpinBox* SpinBox_Lambda;

	UPROPERTY(BlueprintReadOnly, Category = "Widget|SpinBox", Meta = (BindWidget))
	USpinBox* SpinBox_LoopCount;

	UPROPERTY(BlueprintReadOnly, Category = "Widget|Text", Meta = (BindWidget))
	UTextBlock* Text_CurrentDisplayType;

	UPROPERTY(BlueprintReadOnly, Category = "Widget|Text", Meta = (BindWidget))
	UTextBlock* Text_Process;

	UPROPERTY(BlueprintReadOnly, Category = "Widget|Text", Meta = (BindWidget))
	UTextBlock* Text_Procedural;

	UPROPERTY(BlueprintReadOnly, Category = "Widget|Text", Meta = (BindWidget))
	UTextBlock* Text_DisplayWhite;

	UPROPERTY(BlueprintReadOnly, Category = "Widget|Text", Meta = (BindWidget))
	UTextBlock* Text_DisplayNormal;

	UPROPERTY(BlueprintReadOnly, Category = "Widget|Text", Meta = (BindWidget))
	UTextBlock* Text_DisplayGaussianCurvature;

	UPROPERTY(BlueprintReadOnly, Category = "Widget|Text", Meta = (BindWidget))
	UTextBlock* Text_DisplayMeanCurvature;

	UPROPERTY(BlueprintReadOnly, Category = "Widget|Text", Meta = (BindWidget))
	UTextBlock* Text_Lambda;

	UPROPERTY(BlueprintReadOnly, Category = "Widget|Text", Meta = (BindWidget))
	UTextBlock* Text_LoopCount;

	UPROPERTY(BlueprintReadOnly, Category = "Widget|Grid", Meta = (BindWidget))
	UUniformGridPanel* UniformGridPanel_StaticMeshBrowser;

	UPROPERTY(BlueprintReadWrite, Category = "Widget|Settings")
	TSubclassOf<UAssetEntryWidget> AssetEntryWidgetClass;

public:
	TArray<FAssetData> StaticMeshAssets;
};