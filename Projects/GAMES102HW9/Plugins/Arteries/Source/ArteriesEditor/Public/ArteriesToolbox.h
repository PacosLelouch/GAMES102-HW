// Author: LiJiayu (JerryLi)
// Mail: lijiayu83@gmail.com (fullike@163.com)
// Copyright 2019. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Layout/Visibility.h"
#include "Input/Reply.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SWidget.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/SListView.h"
#include "BlueprintEditor.h"
#include "PropertyEditorModule.h"
#include "ArteriesObject.h"
#include "ArteriesViewport.h"
#include "ArteriesEditorMode.h"
#include "ArteriesToolbox.generated.h"

UCLASS()
class UArteriesStats : public UObject
{
	GENERATED_UCLASS_BODY()
public:
	UPROPERTY(VisibleAnywhere, AssetRegistrySearchable, Category = "Stats")
	FString Name;
	UPROPERTY(VisibleAnywhere, AssetRegistrySearchable, Category = "Stats")
	int Count;
};
struct FPropertyAndParent;
class IDetailsView;
class IPropertyTable;
class IPropertyTableWidgetHandle;
class SArteriesToolbox : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SArteriesToolbox) {}
		SLATE_ARGUMENT(FArteriesViewportClient*, Client)
		SLATE_ARGUMENT(FArteriesEditorMode*, EditorMode)
	SLATE_END_ARGS()
	/** SCompoundWidget functions */
	void Construct(const FArguments& InArgs);
	void RefreshProperties();
	void RefreshSelection();
	void RefreshStats();
	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent);
private:
	void OnSelectionChanged();
	bool IsListSelected() const;
	bool CouldPasteHere() const;
	TArray<UArteriesObject*> GetPasteObjects() const;
	void AddStats(const FString& Name, int Count);
	bool IsPropertyVisible(const FPropertyAndParent& PropertyAndParent) const;
	EVisibility GetModeVisibility(int Mode) const;
	EVisibility GetSelectPointVisibility() const;
	EVisibility GetSelectPrimitiveVisibility() const;
	/** Creates the toolbar. */
	TSharedRef<SWidget> BuildToolBar();
	TSharedRef<SWidget> BuildSelectMode(FPropertyEditorModule& PropertyModule);
	TSharedRef<SWidget> BuildCreateMode();
	TSharedRef<SWidget> BuildSettingsMode();
	TSharedRef<SWidget> BuildStatsMode(FPropertyEditorModule& PropertyModule);
public:
	bool bDisplayPoints;
	bool bDisplayPrimitives;
	bool bDisplayPointIds;
	bool bDisplayPrimitiveIds;
private:
	FSlateFontInfo StandardFont;
	FMargin StandardPadding;

	TArray<FName> ModeNames;
	TArray<FName> SelectModeNames;
	TArray<TSharedPtr<int32>> SelectModes;

	FArteriesViewportClient* Client;
	FArteriesEditorMode* EditorMode;
	TSharedPtr<IDetailsView> PropertiesWidget;
	TSharedPtr<IPropertyTable> SelectionTable;
	TSharedPtr<IPropertyTable> StatsTable;
	UClass* SelectionClass;
	TArray<TWeakObjectPtr<UObject>> SelectionObjects;
	TArray<TWeakObjectPtr<UObject>> StatsObjects;
	TSharedPtr<SListView<UArteriesObject*>> ListView;
};
