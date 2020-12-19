// Fill out your copyright notice in the Description page of Project Settings.

#include "MeshControlWidget.h"
#include "SpecificReader.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInterface.h"

void UAssetEntryWidget::BindEventsRuntime()
{
	Border_Thumbnail->OnMouseDoubleClickEvent.BindDynamic(this, &UAssetEntryWidget::OnBorderDoubleClicked);
}

FEventReply UAssetEntryWidget::OnBorderDoubleClicked(FGeometry MyGeometry, const FPointerEvent& MouseEvent)
{
	LoadAsset();
	return FEventReply(true);
}

void UAssetEntryWidget::LoadAsset()
{
	//UStaticMesh* Mesh = Cast<UStaticMesh>(AssetObject);
	//if (Controller && Mesh->IsValidLowLevel())
	//{
	//	Controller->LoadMesh(Mesh);
	//}
}

void UMeshEntryWidget::LoadAsset()
{
	UStaticMesh* Mesh = Cast<UStaticMesh>(AssetObject);
	if (Controller && Mesh->IsValidLowLevel())
	{
		Controller->LoadMesh(Mesh);
	}
}

void UMaterialEntryWidget::LoadAsset()
{
	UMaterialInterface* Material = Cast<UMaterialInterface>(AssetObject);
	if (Controller && Material->IsValidLowLevel())
	{
		Controller->LoadMaterial(Material);
	}
}

void UMeshControlWidget::BindValues()
{
	Text_DisplayWhite->SetText(GetTextDisplayWhite());
	Text_DisplayNormal->SetText(GetTextDisplayNormal());
	Text_DisplayGaussianCurvature->SetText(GetTextGaussianCurvature());
	Text_DisplayMeanCurvature->SetText(GetTextMeanCurvature());
	Text_CurrentDisplayType->SetText(GetTextCurrentDisplayType());
	Text_OneStep->SetText(GetTextOneStep());
	Text_Density->SetText(GetTextDensity());
	Text_LoopCount->SetText(GetTextLoopCount());
	Text_Process->SetText(GetTextProcess());

	SpinBox_Density->SetMinValue(1.0);
	SpinBox_Density->SetMaxValue(100.0);
	SpinBox_Density->SetMinSliderValue(1.0);
	SpinBox_Density->SetMaxSliderValue(100.0);
	SpinBox_Density->Delta = 1. / 256.;
	SpinBox_Density->SliderExponent = 1;

	SpinBox_LoopCount->SetMinValue(0.0);
	SpinBox_LoopCount->SetMinSliderValue(0.0);
	SpinBox_LoopCount->SetMaxSliderValue(20.0);
	SpinBox_LoopCount->Delta = 1;
	SpinBox_LoopCount->SliderExponent = 1;

	UniformGridPanel_StaticMeshBrowser->ClearChildren();
}

void UMeshControlWidget::BindEventsRuntime()
{
	SpinBox_Density->OnValueCommitted.AddDynamic(this, &UMeshControlWidget::CommitSpinBoxDensity);
	SpinBox_LoopCount->OnValueCommitted.AddDynamic(this, &UMeshControlWidget::CommitSpinBoxSubdivisionNum);

	Button_DisplayWhite->OnReleased.AddDynamic(this, &UMeshControlWidget::ReleaseButtonDisplayWhite);
	Button_DisplayNormal->OnReleased.AddDynamic(this, &UMeshControlWidget::ReleaseButtonDisplayNormal);
	Button_DisplayGaussianCurvature->OnReleased.AddDynamic(this, &UMeshControlWidget::ReleaseButtonDisplayGaussianCurvature);
	Button_DisplayMeanCurvature->OnReleased.AddDynamic(this, &UMeshControlWidget::ReleaseButtonDisplayMeanCurvature);
	Button_Process->OnReleased.AddDynamic(this, &UMeshControlWidget::ReleaseButtonProcess);

	CheckBox_OneStep->OnCheckStateChanged.AddDynamic(this, &UMeshControlWidget::ChangeCheckBoxOneStepState);

	if (Controller)
	{
		Controller->OnProceduralHandlerValidityChanged.BindLambda([this]()
		{
			bool bEnabledOperation = !(Controller->PostOneStepHandle.IsValid());
			//Button_DisplayGaussianCurvature->SetIsEnabled(bEnabledOperation);
			//Button_DisplayMeanCurvature->SetIsEnabled(bEnabledOperation);
			//Button_DisplayNormal->SetIsEnabled(bEnabledOperation);
			//Button_DisplayWhite->SetIsEnabled(bEnabledOperation);
			Button_Process->SetIsEnabled(bEnabledOperation);
			SpinBox_LoopCount->SetIsEnabled(bEnabledOperation);
			SpinBox_Density->SetIsEnabled(bEnabledOperation);
		});
	}
}

void UMeshControlWidget::BindValuesRuntime()
{
	BindValues();
	SpinBox_Density->SetValue(GetSpinBoxDensity());
	SpinBox_LoopCount->SetValue(GetSpinBoxLoopCount());
	//SpinBox_Density->ValueDelegate.BindDynamic(this, &UMeshControlWidget::GetSpinBoxDensity);
	//SpinBox_LoopCount->ValueDelegate.BindDynamic(this, &UMeshControlWidget::GetSpinBoxLoopCount);

	if (Controller)
	{
		CheckBox_OneStep->SetIsChecked(Controller->bOneStep);
	}

	ReadStaticMeshesFromAssets();
	MakeAssetGridPanel(UniformGridPanel_StaticMeshBrowser, StaticMeshAssets, AssetEntryWidgetClass);
}

void UMeshControlWidget::ReadFromAssets(TArray<FAssetData>& OutAssets, TSubclassOf<UObject> ObjectType, FString Path)
{
	USpecificReader::Load(OutAssets, ObjectType, Path);
}

void UMeshControlWidget::ReadStaticMeshesFromAssets()
{
	USpecificReader::Load(StaticMeshAssets, UStaticMesh::StaticClass(), TEXT("/Game/Meshes"));
}

void UMeshControlWidget::ReadMaterialsFromAssets()
{
	USpecificReader::Load(MaterialAssets, UMaterialInterface::StaticClass(), TEXT("/Game/Materials"));
}

FText UMeshControlWidget::GetTextCurrentDisplayType()
{
	if (Controller) {
		//switch (Controller->DisplayType) {

		//}
		UEnum* Enum = StaticEnum<EDisplayType>();
		return Enum->GetDisplayNameTextByValue(static_cast<int64>(Controller->DisplayType));
	}
	static FText NoneText = FText::FromString("None");
	return NoneText;
}

FText UMeshControlWidget::GetTextDisplayWhite()
{
	static FText Text = FText::FromString("White");
	return Text;
}

FText UMeshControlWidget::GetTextDisplayNormal()
{
	static FText Text = FText::FromString("Normal");
	return Text;
}

FText UMeshControlWidget::GetTextGaussianCurvature()
{
	static FText Text = FText::FromString("Gaussian Curvature");
	return Text;
}

FText UMeshControlWidget::GetTextMeanCurvature()
{
	static FText Text = FText::FromString("Mean Curvature");
	return Text;
}

FText UMeshControlWidget::GetTextProcess()
{
	static FText Text = FText::FromString("Process");
	return Text;
}

FText UMeshControlWidget::GetTextOneStep()
{
	static FText Text = FText::FromString("OneStep");
	return Text;
}

FText UMeshControlWidget::GetTextDensity()
{
	static FText Text = FText::FromString("Density");
	return Text;
}

FText UMeshControlWidget::GetTextLoopCount()
{
	static FText Text = FText::FromString("Loop Count");
	return Text;
}

float UMeshControlWidget::GetSpinBoxDensity()
{
	if (Controller) {
		return Controller->InputParams.Density;
	}
	return 0.f;
}

float UMeshControlWidget::GetSpinBoxLoopCount()
{
	if (Controller) {
		return static_cast<float>(Controller->InputParams.LoopCount);
	}
	return 0.f;
}

void UMeshControlWidget::CommitSpinBoxDensity(float InValue, ETextCommit::Type CommitMethod)
{
	if (Controller) {
		Controller->InputParams.Density = InValue;
		//Controller->OnInputParamsChanged();
	}
}

void UMeshControlWidget::CommitSpinBoxSubdivisionNum(float InValue, ETextCommit::Type CommitMethod)
{
	if (CommitMethod != ETextCommit::Default)
	{
		int32 LoopCount = FMath::RoundToInt(InValue);
		SpinBox_LoopCount->SetValue(static_cast<float>(LoopCount));
		if (Controller) 
		{
			Controller->InputParams.LoopCount = LoopCount;
			//Controller->OnInputParamsChanged();
		}
	}
}

void UMeshControlWidget::ReleaseButtonDisplayWhite()
{
	if (Controller) {
		Controller->DisplayMesh(EDisplayType::White);
	}
	Text_CurrentDisplayType->SetText(GetTextCurrentDisplayType());
}

void UMeshControlWidget::ReleaseButtonDisplayNormal()
{
	if (Controller) {
		Controller->DisplayMesh(EDisplayType::Normal);
	}
	Text_CurrentDisplayType->SetText(GetTextCurrentDisplayType());
}

void UMeshControlWidget::ReleaseButtonDisplayGaussianCurvature()
{
	if (Controller) {
		Controller->DisplayMesh(EDisplayType::GaussianCurvature);
	}
	Text_CurrentDisplayType->SetText(GetTextCurrentDisplayType());
}

void UMeshControlWidget::ReleaseButtonDisplayMeanCurvature()
{
	if (Controller) {
		Controller->DisplayMesh(EDisplayType::MeanCurvature);
	}
	Text_CurrentDisplayType->SetText(GetTextCurrentDisplayType());
}

void UMeshControlWidget::ReleaseButtonProcess()
{
	if (Controller) {
		Controller->ProcessMesh();
	}
}

void UMeshControlWidget::ChangeCheckBoxOneStepState(bool bIsChecked)
{
	if (Controller) {
		Controller->bOneStep = bIsChecked;
	}
}

void UMeshControlWidget::MakeAssetGridPanel(UUniformGridPanel* UniformGridPanel, const TArray<FAssetData>& Assets, TSubclassOf<UAssetEntryWidget> EntryWidgetClass)
{
	ReadStaticMeshesFromAssets();

	int32 ColumnCount = 2;

	//int32 RowCount = (StaticMeshAssets.Num() + ColumnCount - 1) / ColumnCount;

	for (int32 i = 0; i < Assets.Num(); ++i)
	{
		const FAssetData& AssetData = Assets[i];
		UAssetEntryWidget* Entry = CreateWidget<UAssetEntryWidget>(UniformGridPanel, EntryWidgetClass ? EntryWidgetClass : UObject::StaticClass());
		Entry->Text_Name->SetText(FText::FromString(AssetData.GetAsset()->GetFName().ToString()));
#if WITH_EDITOR
		UTexture2D* Texture2D = USpecificReader::GetThumbnailFromAssetData(AssetData);
		Entry->Border_Thumbnail->SetBrushFromTexture(Texture2D);
#endif
		Entry->AssetObject = AssetData.GetAsset();
		int32 Row = i / ColumnCount;
		int32 Col = i - Row * ColumnCount;
		UniformGridPanel->AddChildToUniformGrid(Entry, Row, Col);
	}
}
