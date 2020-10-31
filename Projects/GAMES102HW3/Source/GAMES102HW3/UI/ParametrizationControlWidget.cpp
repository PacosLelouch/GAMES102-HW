// Fill out your copyright notice in the Description page of Project Settings.

#include "ParametrizationControlWidget.h"

void UParametrizationControlWidget::BindValues()
{
	TextUniform->SetText(GetTextUniform());
	TextChordal->SetText(GetTextChordal());
	TextCentripetal->SetText(GetTextCentripetal());
	TextFoley->SetText(GetTextFoley());
	TextAll->SetText(GetTextAll());
	TextCurrentMethod->SetText(GetTextCurrentMethod());
}

void UParametrizationControlWidget::BindEvents()
{
	ButtonUniform->OnReleased.AddDynamic(this, &UParametrizationControlWidget::ReleaseButtonUniform);
	ButtonChordal->OnReleased.AddDynamic(this, &UParametrizationControlWidget::ReleaseButtonChordal);
	ButtonCentripetal->OnReleased.AddDynamic(this, &UParametrizationControlWidget::ReleaseButtonCentripetal);
	ButtonFoley->OnReleased.AddDynamic(this, &UParametrizationControlWidget::ReleaseButtonFoley);
	ButtonAll->OnReleased.AddDynamic(this, &UParametrizationControlWidget::ReleaseButtonAll);
}

FText UParametrizationControlWidget::GetTextCurrentMethod()
{
	if (Controller) {
		switch (Controller->ParametrizationMethod) {
		case EParametrizationMethod::Uniform:
			return GetTextUniform();
		case EParametrizationMethod::Chordal:
			return GetTextChordal();
		case EParametrizationMethod::Centripetal:
			return GetTextCentripetal();
		case EParametrizationMethod::Foley:
			return GetTextFoley();
		case EParametrizationMethod::All:
			return GetTextAll();
		}
	}
	static FText NoneText = FText::FromString("None");
	return NoneText;
}

FText UParametrizationControlWidget::GetTextUniform()
{
	static FText Text = FText::FromString("Uniform");
	return Text;
}

FText UParametrizationControlWidget::GetTextChordal()
{
	static FText Text = FText::FromString("Chordal");
	return Text;
}

FText UParametrizationControlWidget::GetTextCentripetal()
{
	static FText Text = FText::FromString("Centripetal");
	return Text;
}

FText UParametrizationControlWidget::GetTextFoley()
{
	static FText Text = FText::FromString("Foley");
	return Text;
}

FText UParametrizationControlWidget::GetTextAll()
{
	static FText Text = FText::FromString("All");
	return Text;
}

void UParametrizationControlWidget::ReleaseButtonUniform()
{
	if (Controller) {
		Controller->ChangeParametrizationMethod(EParametrizationMethod::Uniform);
	}
	TextCurrentMethod->SetText(GetTextCurrentMethod());
}

void UParametrizationControlWidget::ReleaseButtonChordal()
{
	if (Controller) {
		Controller->ChangeParametrizationMethod(EParametrizationMethod::Chordal);
	}
	TextCurrentMethod->SetText(GetTextCurrentMethod());
}

void UParametrizationControlWidget::ReleaseButtonCentripetal()
{
	if (Controller) {
		Controller->ChangeParametrizationMethod(EParametrizationMethod::Centripetal);
	}
	TextCurrentMethod->SetText(GetTextCurrentMethod());
}

void UParametrizationControlWidget::ReleaseButtonFoley()
{
	if (Controller) {
		Controller->ChangeParametrizationMethod(EParametrizationMethod::Foley);
	}
	TextCurrentMethod->SetText(GetTextCurrentMethod());
}

void UParametrizationControlWidget::ReleaseButtonAll()
{
	if (Controller) {
		Controller->ChangeParametrizationMethod(EParametrizationMethod::All);
	}
	TextCurrentMethod->SetText(GetTextCurrentMethod());
}
