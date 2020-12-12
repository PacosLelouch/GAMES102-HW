// Author: LiJiayu (JerryLi)
// Mail: lijiayu83@gmail.com (fullike@163.com)
// Copyright 2019. All Rights Reserved.

#include "DetailCustomizations.h"
#include "Widgets/Input/SButton.h"
#define LOCTEXT_NAMESPACE "ArteriesEditor"

TSharedRef<IDetailCustomization> FArteriesActorDetailCustomization::MakeInstance()
{
	return MakeShareable(new FArteriesActorDetailCustomization);
}

void FArteriesActorDetailCustomization::CustomizeDetails(class IDetailLayoutBuilder& DetailBuilder)
{
	TArray<TWeakObjectPtr<UObject>> ObjectsBeingCustomized;
	DetailBuilder.GetObjectsBeingCustomized(ObjectsBeingCustomized);
	if (ObjectsBeingCustomized.Num() > 0)
	{
		if (AArteriesActor* Actor = Cast<AArteriesActor>(ObjectsBeingCustomized[0].Get()))
		{
			IDetailCategoryBuilder& BuildingCategoryBuilder = DetailBuilder.EditCategory("Arteries", FText::GetEmpty(), ECategoryPriority::Important);
			BuildingCategoryBuilder.AddCustomRow(LOCTEXT("Arteries", "Arteries"))
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(6, 0, 0, 0)
				[
					SNew(SButton)
					.ContentPadding(5)
					.IsEnabled(true)
					.HAlign(HAlign_Center)
					.Text(LOCTEXT("Build", "Build"))
					.OnClicked(this, &FArteriesActorDetailCustomization::OnBuildClicked, Actor)
				]
			/*
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(6, 0, 0, 0)
				[
					SNew(SButton)
					.ContentPadding(5)
					.IsEnabled(true)
					.HAlign(HAlign_Center)
					.Text(LOCTEXT("Check", "Check"))
					.OnClicked(this, &FArteriesActorDetailCustomization::OnCheckClicked)
				]*/
			];
		}
	}
}
FReply FArteriesActorDetailCustomization::OnBuildClicked(AArteriesActor* Actor)
{
	Actor->Build(true);
	return FReply::Handled();
}
FReply FArteriesActorDetailCustomization::OnCheckClicked()
{
	int PendingKillCount = 0;
	int UnReachCount = 0;
	int Count = 0;
	for (TObjectIterator<AArteriesActor> It; It; ++It)
	{
		AArteriesActor* Actor = *It;
		Count++;
		FUObjectItem* ObjectItem = GUObjectArray.ObjectToObjectItem(Actor);
		if (ObjectItem->IsUnreachable())
			UnReachCount++;
		if (ObjectItem->IsPendingKill())
			PendingKillCount++;
	}
	return FReply::Handled();
}
#undef LOCTEXT_NAMESPACE