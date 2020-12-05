// Fill out your copyright notice in the Description page of Project Settings.

#include "MeshControlWidget.h"
#include "SpecificReader.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"

void UAssetEntryWidget::BindEventsRuntime()
{
	Border_Thumbnail->OnMouseDoubleClickEvent.BindDynamic(this, &UAssetEntryWidget::OnBorderDoubleClicked);
}


FEventReply UAssetEntryWidget::OnBorderDoubleClicked(FGeometry MyGeometry, const FPointerEvent& MouseEvent)
{
	UStaticMesh* Mesh = Cast<UStaticMesh>(AssetObject);
	if (Controller && Mesh->IsValidLowLevel())
	{
		Controller->LoadMesh(Mesh);
	}
	return FEventReply(true);
}

void UMeshControlWidget::BindValues()
{
	Text_DisplayWhite->SetText(GetTextDisplayWhite());
	Text_DisplayNormal->SetText(GetTextDisplayNormal());
	Text_DisplayGaussianCurvature->SetText(GetTextGaussianCurvature());
	Text_DisplayMeanCurvature->SetText(GetTextMeanCurvature());
	Text_CurrentDisplayType->SetText(GetTextCurrentDisplayType());
	Text_Procedural->SetText(GetTextProcedural());
	Text_Lambda->SetText(GetTextLambda());
	Text_LoopCount->SetText(GetTextLoopCount());
	Text_Process->SetText(GetTextProcess());

	SpinBox_Lambda->SetMinValue(0.0);
	SpinBox_Lambda->SetMaxValue(1.0);
	SpinBox_Lambda->SetMinSliderValue(0.0);
	SpinBox_Lambda->SetMaxSliderValue(1.0);
	SpinBox_Lambda->Delta = 1. / 256.;
	SpinBox_Lambda->SliderExponent = 1;

	SpinBox_LoopCount->SetMinValue(0.0);
	SpinBox_LoopCount->SetMinSliderValue(0.0);
	SpinBox_LoopCount->SetMaxSliderValue(20.0);
	SpinBox_LoopCount->Delta = 1;
	SpinBox_LoopCount->SliderExponent = 1;

	UniformGridPanel_StaticMeshBrowser->ClearChildren();
}

void UMeshControlWidget::BindEventsRuntime()
{
	SpinBox_Lambda->OnValueCommitted.AddDynamic(this, &UMeshControlWidget::CommitSpinBoxLambda);
	SpinBox_LoopCount->OnValueCommitted.AddDynamic(this, &UMeshControlWidget::CommitSpinBoxSubdivisionNum);

	Button_DisplayWhite->OnReleased.AddDynamic(this, &UMeshControlWidget::ReleaseButtonDisplayWhite);
	Button_DisplayNormal->OnReleased.AddDynamic(this, &UMeshControlWidget::ReleaseButtonDisplayNormal);
	Button_DisplayGaussianCurvature->OnReleased.AddDynamic(this, &UMeshControlWidget::ReleaseButtonDisplayGaussianCurvature);
	Button_DisplayMeanCurvature->OnReleased.AddDynamic(this, &UMeshControlWidget::ReleaseButtonDisplayMeanCurvature);
	Button_Process->OnReleased.AddDynamic(this, &UMeshControlWidget::ReleaseButtonProcess);

	CheckBox_Procedural->OnCheckStateChanged.AddDynamic(this, &UMeshControlWidget::ChangeCheckBoxProceduralState);

	if (Controller)
	{
		Controller->OnProceduralHandlerValidityChanged.BindLambda([this]()
		{
			bool bEnabledOperation = !(Controller->ProceduralHandler.IsValid());
			//Button_DisplayGaussianCurvature->SetIsEnabled(bEnabledOperation);
			//Button_DisplayMeanCurvature->SetIsEnabled(bEnabledOperation);
			//Button_DisplayNormal->SetIsEnabled(bEnabledOperation);
			//Button_DisplayWhite->SetIsEnabled(bEnabledOperation);
			Button_Process->SetIsEnabled(bEnabledOperation);
			SpinBox_LoopCount->SetIsEnabled(bEnabledOperation);
			SpinBox_Lambda->SetIsEnabled(bEnabledOperation);
		});
	}
}

void UMeshControlWidget::BindValuesRuntime()
{
	BindValues();
	SpinBox_Lambda->SetValue(GetSpinBoxLambda());
	SpinBox_LoopCount->SetValue(GetSpinBoxLoopCount());
	//SpinBox_Lambda->ValueDelegate.BindDynamic(this, &UMeshControlWidget::GetSpinBoxLambda);
	//SpinBox_LoopCount->ValueDelegate.BindDynamic(this, &UMeshControlWidget::GetSpinBoxLoopCount);

	if (Controller)
	{
		CheckBox_Procedural->SetIsChecked(Controller->bProcedural);
	}

	ReadStaticMeshesFromAssets();

	int32 ColumnCount = 2;

	//int32 RowCount = (StaticMeshAssets.Num() + ColumnCount - 1) / ColumnCount;

	for (int32 i = 0; i < StaticMeshAssets.Num(); ++i)
	{
		const FAssetData& AssetData = StaticMeshAssets[i];
		UAssetEntryWidget* Entry = CreateWidget<UAssetEntryWidget>(UniformGridPanel_StaticMeshBrowser, AssetEntryWidgetClass);
		Entry->Text_Name->SetText(FText::FromString(AssetData.GetAsset()->GetFName().ToString()));
#if WITH_EDITOR
		UTexture2D* Texture2D = USpecificReader::GetThumbnailFromAssetData(AssetData);
		Entry->Border_Thumbnail->SetBrushFromTexture(Texture2D);
#endif
		Entry->AssetObject = AssetData.GetAsset();
		int32 Row = i / ColumnCount;
		int32 Col = i - Row * ColumnCount;
		UniformGridPanel_StaticMeshBrowser->AddChildToUniformGrid(Entry, Row, Col);
	}
}

void UMeshControlWidget::ReadStaticMeshesFromAssets()
{
	USpecificReader::Load(StaticMeshAssets, UStaticMesh::StaticClass(), TEXT("/Game/Meshes"));
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

FText UMeshControlWidget::GetTextProcedural()
{
	static FText Text = FText::FromString("Procedural");
	return Text;
}

FText UMeshControlWidget::GetTextLambda()
{
	static FText Text = FText::FromString("Lambda");
	return Text;
}

FText UMeshControlWidget::GetTextLoopCount()
{
	static FText Text = FText::FromString("Loop Count");
	return Text;
}

float UMeshControlWidget::GetSpinBoxLambda()
{
	if (Controller) {
		return Controller->InputParams.Lambda;
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

void UMeshControlWidget::CommitSpinBoxLambda(float InValue, ETextCommit::Type CommitMethod)
{
	if (Controller) {
		Controller->InputParams.Lambda = InValue;
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

void UMeshControlWidget::ChangeCheckBoxProceduralState(bool bIsChecked)
{
	if (Controller) {
		Controller->bProcedural = bIsChecked;
	}
}
