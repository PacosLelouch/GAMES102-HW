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
#include "AssetData.h"
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

	virtual void LoadAsset();
};

UCLASS(BlueprintType, Blueprintable)
class UMeshEntryWidget : public UAssetEntryWidget
{
	GENERATED_BODY()
protected:
	virtual void LoadAsset() override;
};

UCLASS(BlueprintType, Blueprintable)
class UMaterialEntryWidget : public UAssetEntryWidget
{
	GENERATED_BODY()
protected:
	virtual void LoadAsset() override;
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
	UFUNCTION(BlueprintCallable)
	void ReadFromAssets(TArray<FAssetData>& OutAssets, TSubclassOf<UObject> ObjectType, FString Path);

	void ReadStaticMeshesFromAssets();

	void ReadMaterialsFromAssets();

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
	FText GetTextOneStep();

	UFUNCTION(BlueprintGetter)
	FText GetTextDensity();

	UFUNCTION(BlueprintGetter)
	FText GetTextLoopCount();

	UFUNCTION(BlueprintGetter)
	float GetSpinBoxDensity();

	UFUNCTION(BlueprintGetter)
	float GetSpinBoxLoopCount();

public:

	UFUNCTION(BlueprintCallable)
	void CommitSpinBoxDensity(float InValue, ETextCommit::Type CommitMethod);

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
	void ChangeCheckBoxOneStepState(bool bIsChecked);

	UFUNCTION(BlueprintCallable)
	void MakeAssetGridPanel(UUniformGridPanel* UniformGridPanel, const TArray<FAssetData>& Assets, TSubclassOf<UAssetEntryWidget> EntryWidgetClass);

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
	UCheckBox* CheckBox_OneStep;

	UPROPERTY(BlueprintReadOnly, Category = "Widget|SpinBox", Meta = (BindWidget))
	USpinBox* SpinBox_Density;

	UPROPERTY(BlueprintReadOnly, Category = "Widget|SpinBox", Meta = (BindWidget))
	USpinBox* SpinBox_LoopCount;

	UPROPERTY(BlueprintReadOnly, Category = "Widget|Text", Meta = (BindWidget))
	UTextBlock* Text_CurrentDisplayType;

	UPROPERTY(BlueprintReadOnly, Category = "Widget|Text", Meta = (BindWidget))
	UTextBlock* Text_Process;

	UPROPERTY(BlueprintReadOnly, Category = "Widget|Text", Meta = (BindWidget))
	UTextBlock* Text_OneStep;

	UPROPERTY(BlueprintReadOnly, Category = "Widget|Text", Meta = (BindWidget))
	UTextBlock* Text_DisplayWhite;

	UPROPERTY(BlueprintReadOnly, Category = "Widget|Text", Meta = (BindWidget))
	UTextBlock* Text_DisplayNormal;

	UPROPERTY(BlueprintReadOnly, Category = "Widget|Text", Meta = (BindWidget))
	UTextBlock* Text_DisplayGaussianCurvature;

	UPROPERTY(BlueprintReadOnly, Category = "Widget|Text", Meta = (BindWidget))
	UTextBlock* Text_DisplayMeanCurvature;

	UPROPERTY(BlueprintReadOnly, Category = "Widget|Text", Meta = (BindWidget))
	UTextBlock* Text_Density;

	UPROPERTY(BlueprintReadOnly, Category = "Widget|Text", Meta = (BindWidget))
	UTextBlock* Text_LoopCount;

	UPROPERTY(BlueprintReadOnly, Category = "Widget|Grid", Meta = (BindWidget))
	UUniformGridPanel* UniformGridPanel_StaticMeshBrowser;

	UPROPERTY(BlueprintReadWrite, Category = "Widget|Settings")
	TSubclassOf<UAssetEntryWidget> AssetEntryWidgetClass;

public:
	UPROPERTY(BlueprintReadOnly, Category = "Widget|Assets")
	TArray<FAssetData> StaticMeshAssets;

	UPROPERTY(BlueprintReadOnly, Category = "Widget|Settings")
	TArray<FAssetData> MaterialAssets;
};