// Fill out your copyright notice in the Description page of Project Settings.

#include "SubdivisionControlWidget.h"


void USubdivisionControlWidget::BindValues()
{
	Text_Chaikin->SetText(GetTextChaikin());
	Text_ThreeDegreeBSpline->SetText(GetTextThreeDegreeBSpline());
	Text_FourPointInterpolation->SetText(GetTextFourPointInterpolation());
	Text_All->SetText(GetTextAll());
	Text_CurrentMethod->SetText(GetTextCurrentMethod());
	Text_Closed->SetText(GetTextClosed());
	Text_Alpha->SetText(GetTextAlpha());
	Text_SubdivisionNum->SetText(GetTextSubdivisionNum());

	SpinBox_Alpha->SetMinValue(0.0);
	SpinBox_Alpha->SetMinSliderValue(0.0);
	SpinBox_Alpha->SetMaxSliderValue(1.0);
	SpinBox_Alpha->Delta = 0.125 / 16.;
	SpinBox_Alpha->SliderExponent = 1;

	SpinBox_SubdivisionNum->SetMinValue(0.0);
	SpinBox_SubdivisionNum->SetMinSliderValue(0.0);
	SpinBox_SubdivisionNum->SetMaxSliderValue(10.0);
	SpinBox_SubdivisionNum->Delta = 1;
	SpinBox_SubdivisionNum->SliderExponent = 1;
}

void USubdivisionControlWidget::BindEventsRuntime()
{
	SpinBox_Alpha->OnValueCommitted.AddDynamic(this, &USubdivisionControlWidget::CommitSpinBoxAlpha);
	SpinBox_SubdivisionNum->OnValueCommitted.AddDynamic(this, &USubdivisionControlWidget::CommitSpinBoxSubdivisionNum);

	Button_Chaikin->OnReleased.AddDynamic(this, &USubdivisionControlWidget::ReleaseButtonChaikin);
	Button_ThreeDegreeBSpline->OnReleased.AddDynamic(this, &USubdivisionControlWidget::ReleaseButtonThreeDegreeBSpline);
	Button_FourPointInterpolation->OnReleased.AddDynamic(this, &USubdivisionControlWidget::ReleaseButtonFourPointInterpolation);
	Button_All->OnReleased.AddDynamic(this, &USubdivisionControlWidget::ReleaseButtonAll);
	Button_Clear->OnReleased.AddDynamic(this, &USubdivisionControlWidget::ReleaseButtonClear);

	CheckBox_Closed->OnCheckStateChanged.AddDynamic(this, &USubdivisionControlWidget::ChangeCheckBoxClosedState);
}

void USubdivisionControlWidget::BindValuesRuntime()
{
	BindValues();
	SpinBox_Alpha->SetValue(GetSpinBoxAlpha());
	SpinBox_SubdivisionNum->SetValue(GetSpinBoxSubdivisionNum());
	//SpinBox_Alpha->ValueDelegate.BindDynamic(this, &USubdivisionControlWidget::GetSpinBoxAlpha);
	//SpinBox_SubdivisionNum->ValueDelegate.BindDynamic(this, &USubdivisionControlWidget::GetSpinBoxSubdivisionNum);
}

FText USubdivisionControlWidget::GetTextCurrentMethod()
{
	if (Controller) {
		switch (Controller->SubdivisionMethod) {
		case ESubdivisionMethod::Chaikin:
			return GetTextChaikin();
		case ESubdivisionMethod::ThreeDegreeBSpline:
			return GetTextThreeDegreeBSpline();
		case ESubdivisionMethod::FourPointInterpolation:
			return GetTextFourPointInterpolation();
		case ESubdivisionMethod::All:
			return GetTextAll();
		}
	}
	static FText NoneText = FText::FromString("None");
	return NoneText;
}

FText USubdivisionControlWidget::GetTextChaikin()
{
	static FText Text = FText::FromString("Chaikin");
	return Text;
}

FText USubdivisionControlWidget::GetTextThreeDegreeBSpline()
{
	static FText Text = FText::FromString("ThreeDegreeBSpline");
	return Text;
}

FText USubdivisionControlWidget::GetTextFourPointInterpolation()
{
	static FText Text = FText::FromString("FourPointInterpolation");
	return Text;
}

FText USubdivisionControlWidget::GetTextAll()
{
	static FText Text = FText::FromString("All");
	return Text;
}

FText USubdivisionControlWidget::GetTextClosed()
{
	static FText Text = FText::FromString("Closed");
	return Text;
}

FText USubdivisionControlWidget::GetTextAlpha()
{
	static FText Text = FText::FromString("Alpha");
	return Text;
}

FText USubdivisionControlWidget::GetTextSubdivisionNum()
{
	static FText Text = FText::FromString("Subdivision Num");
	return Text;
}

float USubdivisionControlWidget::GetSpinBoxAlpha()
{
	if (Controller) {
		return Controller->ParamsInput.Alpha;
	}
	return 0.;
}

float USubdivisionControlWidget::GetSpinBoxSubdivisionNum()
{
	if (Controller) {
		return static_cast<float>(Controller->ParamsInput.Num);
	}
	return 0;
}

void USubdivisionControlWidget::CommitSpinBoxAlpha(float InValue, ETextCommit::Type CommitMethod)
{
	if (Controller) {
		Controller->ParamsInput.Alpha = InValue;
		Controller->OnParamsInputChanged();
	}
}

void USubdivisionControlWidget::CommitSpinBoxSubdivisionNum(float InValue, ETextCommit::Type CommitMethod)
{
	if (CommitMethod != ETextCommit::Default)
	{
		SpinBox_SubdivisionNum->SetValue(FMath::RoundToFloat(InValue));
		if (Controller) 
		{
			Controller->ParamsInput.Num = FMath::RoundToInt(InValue);
			Controller->OnParamsInputChanged();
		}
	}
}

void USubdivisionControlWidget::ReleaseButtonChaikin()
{
	if (Controller) {
		Controller->ChangeSubdivisionMethod(ESubdivisionMethod::Chaikin);
	}
	Text_CurrentMethod->SetText(GetTextCurrentMethod());
}

void USubdivisionControlWidget::ReleaseButtonThreeDegreeBSpline()
{
	if (Controller) {
		Controller->ChangeSubdivisionMethod(ESubdivisionMethod::ThreeDegreeBSpline);
	}
	Text_CurrentMethod->SetText(GetTextCurrentMethod());
}

void USubdivisionControlWidget::ReleaseButtonFourPointInterpolation()
{
	if (Controller) {
		Controller->ChangeSubdivisionMethod(ESubdivisionMethod::FourPointInterpolation);
	}
	Text_CurrentMethod->SetText(GetTextCurrentMethod());
}

void USubdivisionControlWidget::ReleaseButtonAll()
{
	if (Controller) {
		Controller->ChangeSubdivisionMethod(ESubdivisionMethod::All);
	}
	Text_CurrentMethod->SetText(GetTextCurrentMethod());
}

void USubdivisionControlWidget::ReleaseButtonClear()
{
	if (Controller) {
		Controller->ClearCanvas();
	}
}

void USubdivisionControlWidget::ChangeCheckBoxClosedState(bool bIsChecked)
{
	if (Controller) {
		Controller->ParamsInput.bClosed = bIsChecked;
		Controller->OnParamsInputChanged();
	}
}
