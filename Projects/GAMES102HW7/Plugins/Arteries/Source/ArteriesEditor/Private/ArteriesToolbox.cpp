// Author: LiJiayu (JerryLi)
// Mail: lijiayu83@gmail.com (fullike@163.com)
// Copyright 2019. All Rights Reserved.

#include "ArteriesToolbox.h"
#include "ArteriesActor.h"
#include "ArteriesEditorCommands.h"
#include "SEditorViewport.h"
#include "SlateOptMacros.h"
#include "EditorModeManager.h"
#include "ProceduralMeshComponent.h"
#include "PropertyPath.h"
#include "IPropertyTable.h"
#include "IPropertyTableColumn.h"
#include "IPropertyTableWidgetHandle.h"
#include "Framework/Commands/GenericCommands.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Widgets/Text/SInlineEditableTextBlock.h"
#include "Widgets/Layout/SHeader.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Layout/SWrapBox.h"
#include "Widgets/Layout/SUniformGridPanel.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Input/SNumericEntryBox.h"
#include "HAL/PlatformApplicationMisc.h"

UArteriesStats::UArteriesStats(const FObjectInitializer& ObjectInitializer) :UObject(ObjectInitializer), Count(0)
{
}
#define LOCTEXT_NAMESPACE "ArteriesEditor"
BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void SArteriesToolbox::Construct(const FArguments& InArgs)
{
	SelectionClass = NULL;
	bDisplayPoints = true;
	bDisplayPrimitives = true;
	bDisplayPointIds = false;
	bDisplayPrimitiveIds = false;

	Client = InArgs._Client;
	EditorMode = InArgs._EditorMode;
	EditorMode->Toolbox = this;

	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	FDetailsViewArgs Args;
	Args.bHideSelectionTip = true;
	Args.bAllowSearch = false;
	PropertiesWidget = PropertyModule.CreateDetailView(Args);
	PropertiesWidget->SetIsPropertyVisibleDelegate(FIsPropertyVisible::CreateSP(this, &SArteriesToolbox::IsPropertyVisible));
	PropertiesWidget->SetObject(EditorMode->Properties);

	SelectionTable = PropertyModule.CreatePropertyTable();
	SelectionTable->SetIsUserAllowedToChangeRoot(false);
	SelectionTable->SetOrientation(EPropertyTableOrientation::AlignPropertiesInColumns);
	SelectionTable->SetShowRowHeader(false);
	SelectionTable->SetShowObjectName(false);
	SelectionTable->OnSelectionChanged()->AddSP(this, &SArteriesToolbox::OnSelectionChanged);

	StatsTable = PropertyModule.CreatePropertyTable();
	StatsTable->SetIsUserAllowedToChangeRoot(false);
	StatsTable->SetOrientation(EPropertyTableOrientation::AlignPropertiesInColumns);
	StatsTable->SetShowRowHeader(false);
	StatsTable->SetShowObjectName(false);

	ModeNames.Add("Select");
	ModeNames.Add("Create");
	ModeNames.Add("Settings");
	ModeNames.Add("Stats");

	StandardFont = FEditorStyle::GetFontStyle(TEXT("PropertyWindow.NormalFont"));
	StandardPadding = FMargin(6.f, 3.f);

	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
	//	.AutoHeight()
		.FillHeight(0.65f)
		.Padding(2)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(1.f, 5.f, 0.f, 5.f)
			[
				BuildToolBar()
			]
			+ SHorizontalBox::Slot()
			.Padding(0.f, 2.f, 2.f, 0.f)
			[
				SNew(SBorder)
				.BorderImage(FEditorStyle::GetBrush("ToolPanel.DarkGroupBorder"))
				.Padding(StandardPadding)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(StandardPadding)
					[
						SNew(STextBlock)
						.Text_Lambda([this] { return FText::FromName(ModeNames[EditorMode->Mode]); })
						.TextStyle(FEditorStyle::Get(), "FoliageEditMode.ActiveToolName.Text")
					]
					+ SVerticalBox::Slot()
						.FillHeight(1.f)
					[
						BuildSelectMode(PropertyModule)
					]
					+ SVerticalBox::Slot()
						.FillHeight(1.f)
					[
						BuildCreateMode()
					]
					+ SVerticalBox::Slot()
						.FillHeight(1.f)
					[
						BuildSettingsMode()
					]
					+ SVerticalBox::Slot()
						.FillHeight(1.f)
					[
						BuildStatsMode(PropertyModule)
					]
				]
			]
		]
		/*
		+ SVerticalBox::Slot()
		.MaxHeight(500.0f)
		.Padding(2)
		[
			SAssignNew(ResultsList, SListView<TSharedPtr<FString>>)
			.ItemHeight(20.0f)
			.SelectionMode(ESelectionMode::Single)
			.ListItemsSource(&Results)
			.OnGenerateRow_Lambda([](TSharedPtr<FString> InItem, const TSharedRef<STableViewBase>& Owner)
			{
				return SNew(STableRow<TSharedPtr<FText>>, Owner)
					.Padding(FMargin(16, 4, 16, 4))
					[
						SNew(STextBlock).Text(FText::FromString(*InItem))
					];
			})
			.OnMouseButtonDoubleClick_Lambda([this](TSharedPtr<FString> InItem)
			{
				Properties->CurrentLabel = *InItem;
				Properties->UpdateDisplay();
			})
		]*/
		+SVerticalBox::Slot()
	//	.AutoHeight()
	//	.MaxHeight(500)
		.FillHeight(0.35f)
		.Padding(2)
		[
			PropertiesWidget.ToSharedRef()
		]
	];
}
void SArteriesToolbox::RefreshProperties()
{
	PropertiesWidget->ForceRefresh();
}
void SArteriesToolbox::RefreshSelection()
{
	UArteriesObject* Object = EditorMode->Object;
	if (SelectionClass)
	{
		SelectionClass->ClearFlags(RF_Public | RF_Standalone);
		SelectionClass = nullptr;
		CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS);
	}
	UPackage* Pkg = EditorMode->BlueprintEditor->GetBlueprintObj()->GetOutermost();
	SelectionClass = NewObject<UClass>(Pkg, "SelectionClass", RF_Public | RF_Standalone | RF_Transient);
	SelectionClass->SetSuperStruct(UObject::StaticClass());
	auto AddProperty = [&](const FName& Name, UClass* PropClass, UScriptStruct* Struct = NULL)
	{
		if (UProperty* Prop = FindField<UProperty>(SelectionClass, Name))
			return;
		UProperty* Prop = Cast<UProperty>(StaticConstructObject_Internal(PropClass, SelectionClass, Name, RF_Public | RF_Transient));		
		Prop->SetPropertyFlags(CPF_Edit | CPF_EditConst | CPF_AssetRegistrySearchable | CPF_NativeAccessSpecifierPublic | CPF_ComputedFlags);
		if (UStructProperty* StructProp = Cast<UStructProperty>(Prop))
			StructProp->Struct = Struct;
		SelectionClass->AddCppProperty(Prop);
	};
	auto CollectProperties = [&](FArteriesElement* Element)
	{
		for (auto It = Element->IntValues.CreateIterator(); It; ++It)
			AddProperty(It->Key, UIntProperty::StaticClass());
		for (auto It = Element->FloatValues.CreateIterator(); It; ++It)
			AddProperty(It->Key, UFloatProperty::StaticClass());
		for (auto It = Element->Vec2Values.CreateIterator(); It; ++It)
			AddProperty(It->Key, UStructProperty::StaticClass(), TBaseStructure<FVector2D>::Get());
		for (auto It = Element->Vec3Values.CreateIterator(); It; ++It)
			AddProperty(It->Key, UStructProperty::StaticClass(), TBaseStructure<FVector>::Get());
	//	for (auto It = Element->StrValues.CreateIterator(); It; ++It)
	//		AddProperty(It->Key, UStrProperty::StaticClass());
	};
	switch (EditorMode->SubMode)
	{
	case 0:
		for (FArteriesPoint& Point : Object->Points)
			CollectProperties(&Point);
		AddProperty("Position", UStructProperty::StaticClass(), TBaseStructure<FVector>::Get());
		AddProperty("ID", UIntProperty::StaticClass());
		break;
	case 1:
		for (FArteriesPrimitive& Primitive : Object->Primitives)
			CollectProperties(&Primitive);
		AddProperty("NumPoints", UIntProperty::StaticClass());
		AddProperty("ID", UIntProperty::StaticClass());
		break;
	case 2:
		AddProperty("NumPoints", UIntProperty::StaticClass());
		AddProperty("Name", UNameProperty::StaticClass());
		break;
	case 3:
		AddProperty("NumPrimitives", UIntProperty::StaticClass());
		AddProperty("Name", UNameProperty::StaticClass());
		break;
	}
	// Finalize the class
	SelectionClass->Bind();
	SelectionClass->StaticLink(true);
	SelectionClass->AssembleReferenceTokenStream();
	// Ensure the CDO exists
	SelectionClass->GetDefaultObject(true);

	for (TWeakObjectPtr<UObject> Obj : SelectionObjects)
		Obj->RemoveFromRoot();
	SelectionObjects.Empty();
	auto SetProperty = [&](UObject* Obj, const FName& Name, void* SourceData, uint32 Size)
	{
		if (UProperty* Prop = FindField<UProperty>(SelectionClass, Name))
			FMemory::Memcpy((uint8*)Obj + Prop->GetOffset_ForInternal(), SourceData, Size);
	};
	auto SetProperties = [&](UObject* Obj, FArteriesElement* Element)
	{
		for (auto It = Element->IntValues.CreateIterator(); It; ++It)
			SetProperty(Obj, It->Key, &It->Value, sizeof(int));
		for (auto It = Element->FloatValues.CreateIterator(); It; ++It)
			SetProperty(Obj, It->Key, &It->Value, sizeof(float));
		for (auto It = Element->Vec2Values.CreateIterator(); It; ++It)
			SetProperty(Obj, It->Key, &It->Value, sizeof(FVector2D));
		for (auto It = Element->Vec3Values.CreateIterator(); It; ++It)
			SetProperty(Obj, It->Key, &It->Value, sizeof(FVector));
	};
	auto SetPointProperties = [&](UObject* Obj, FArteriesPoint* Point, int Index)
	{
		SetProperty(Obj, "ID", &Index, sizeof(int));
		SetProperty(Obj, "Position", &Point->Position, sizeof(FVector));
		SetProperties(Obj, Point);
	};
	auto SetPrimitiveProperties = [&](UObject* Obj, FArteriesPrimitive* Primitive, int Index)
	{
		SetProperty(Obj, "ID", &Index, sizeof(int));
		int NumPoints = Primitive->Points.Num();
		SetProperty(Obj, "NumPoints", &NumPoints, sizeof(int));
		SetProperties(Obj, Primitive);
	};
	auto CreateSelectionObj = [&]()->UObject*
	{
		UObject* Obj = StaticConstructObject_Internal(SelectionClass, GetTransientPackage());
		Obj->AddToRoot();
		SelectionObjects.Add(Obj);
		return Obj;
	};
	switch (EditorMode->SubMode)
	{
	case 0:
		for (int i = 0; i < Object->Points.Num(); i++)
			SetPointProperties(CreateSelectionObj(), &Object->Points[i], i);
		break;
	case 1:
		for (int i = 0; i < Object->Primitives.Num(); i++)
			SetPrimitiveProperties(CreateSelectionObj(), &Object->Primitives[i], i);
		break;
	case 2:
		for (auto It = Object->PointGroups.CreateIterator(); It; ++It)
		{
			int NumPoints = It->Value.Points.Num();
			UObject* Obj = CreateSelectionObj();
			SetProperty(Obj, "Name", &It->Key, sizeof(FName));
			SetProperty(Obj, "NumPoints", &NumPoints, sizeof(int));
		}
		break;
	case 3:
		for (auto It = Object->PrimitiveGroups.CreateIterator(); It; ++It)
		{
			int NumPrimitives = It->Value.Primitives.Num();
			UObject* Obj = CreateSelectionObj();
			SetProperty(Obj, "Name", &It->Key, sizeof(FName));
			SetProperty(Obj, "NumPrimitives", &NumPrimitives, sizeof(int));
		}
		break;
	}
	SelectionTable->SetObjects(SelectionObjects);
	const TArray< TSharedRef< class IPropertyTableColumn > >& Columns = SelectionTable->GetColumns();
	for (int i = 0; i < Columns.Num(); i++)
	{
		TSharedPtr< FPropertyPath > PropertyPath = Columns[i]->GetDataSource()->AsPropertyPath();
		const FPropertyInfo& PropertyInfo = PropertyPath->GetRootProperty();
		const FString& ColumnWidthString = PropertyInfo.Property->GetMetaData("ColumnWidth");
		const float ColumnWidth = ColumnWidthString.Len() > 0 ? FCString::Atof(*ColumnWidthString) : 100.0f;
		Columns[i]->SetWidth(ColumnWidth);
		Columns[i]->SetSizeMode(EPropertyTableColumnSizeMode::Fixed);
	}
}
void SArteriesToolbox::RefreshStats()
{
	AArteriesActor* Actor = EditorMode->Actor;
	UArteriesObject* Object = EditorMode->Object;
	for (TWeakObjectPtr<UObject> Obj : StatsObjects)
		Obj->RemoveFromRoot();
	StatsObjects.Empty();

	AddStats(TEXT("Points"), Object->Points.Num());
	AddStats(TEXT("Primitives"), Object->Primitives.Num());

	TArray<UActorComponent*> ProceduralMeshComponents = Actor->K2_GetComponentsByClass(UProceduralMeshComponent::StaticClass());
	for (UActorComponent* Component : ProceduralMeshComponents)
	{
		if (UProceduralMeshComponent* ProcComponent = Cast<UProceduralMeshComponent>(Component))
		{
			for (int i = 0; i < ProcComponent->GetNumSections(); i++)
			{
				UMaterialInterface* Material = ProcComponent->GetMaterial(i);
				FProcMeshSection* Section = ProcComponent->GetProcMeshSection(i);
				FString MaterialName = Material ? Material->GetName() : TEXT("Default");
				AddStats(FString::Printf(TEXT("Vertices(%s)"), *MaterialName), Section->ProcVertexBuffer.Num());
				AddStats(FString::Printf(TEXT("Triangles(%s)"), *MaterialName), Section->ProcIndexBuffer.Num() / 3);
			}
		}
	}
	TMap<UClass*, int> Actors;
	for (USceneComponent* Component : Actor->GetRootComponent()->GetAttachChildren())
	{
		AActor* Owner = Component->GetOwner();
		if (Owner != Actor)
			Actors.FindOrAdd(Owner->GetClass())++;
	}
	for (auto It = Actors.CreateIterator(); It; ++It)
		AddStats(FString::Printf(TEXT("Actors(%s)"), *It->Key->GetName()), It->Value);

	TArray<UActorComponent*> InstancedStaticMeshComponents = Actor->K2_GetComponentsByClass(UHierarchicalInstancedStaticMeshComponent::StaticClass());
	for (UActorComponent* Component : InstancedStaticMeshComponents)
	{
		if (UHierarchicalInstancedStaticMeshComponent* InstComponent = Cast<UHierarchicalInstancedStaticMeshComponent>(Component))
		{
			UStaticMesh* Mesh = InstComponent->GetStaticMesh();
			AddStats(FString::Printf(TEXT("Instances(%s)"), *Mesh->GetName()), InstComponent->GetInstanceCount());
		}
	}

	StatsTable->SetObjects(StatsObjects);
	/*
	for (TFieldIterator<UProperty> PropertyIter(UArteriesStats::StaticClass(), EFieldIteratorFlags::IncludeSuper); PropertyIter; ++PropertyIter)
	{
		const TWeakObjectPtr< UProperty >& Property = *PropertyIter;
		StatsTable->AddColumn(Property);
	}*/
//	StatsTable->ForceRefresh();
}
FReply SArteriesToolbox::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	if (EditorMode->UICommandList->ProcessCommandBindings(InKeyEvent))
		return FReply::Handled();
	if (EditorMode->InputKey(Client, Client->Viewport, InKeyEvent.GetKey(), InKeyEvent.IsRepeat() ? IE_Repeat : IE_Pressed))
		return FReply::Handled();
	return FReply::Unhandled();
}
void SArteriesToolbox::OnSelectionChanged()
{
	TArray<TWeakObjectPtr<UObject>> SelectedObjects;
	SelectionTable->GetSelectedTableObjects(SelectedObjects);
	UArteriesObject* Object = EditorMode->Object;
	EditorMode->ClearSelection();
	switch (EditorMode->SubMode)
	{
	case 0:
	{
		UIntProperty* Prop = FindField<UIntProperty>(SelectionClass, "ID");
		for (TWeakObjectPtr<UObject>& Obj : SelectedObjects)
		{
			int ID = Prop->GetPropertyValue_InContainer(Obj.Get());
			EditorMode->AddToSelection(&Object->Points[ID]);
		}
		break;
	}
	case 1:
	{
		UIntProperty* Prop = FindField<UIntProperty>(SelectionClass, "ID");
		for (TWeakObjectPtr<UObject>& Obj : SelectedObjects)
		{
			int ID = Prop->GetPropertyValue_InContainer(Obj.Get());
			EditorMode->AddToSelection(&Object->Primitives[ID]);
		}
		break;
	}
	case 2:
	{
		UNameProperty* Prop = FindField<UNameProperty>(SelectionClass, "Name");
		for (TWeakObjectPtr<UObject>& Obj : SelectedObjects)
		{
			FName Name = Prop->GetPropertyValue_InContainer(Obj.Get());
			FArteriesPointGroup& Group = Object->PointGroups[Name];
			for (FArteriesPoint* Point : Group.Points)
				EditorMode->AddToSelection(Point);
		}
		break;
	}
	case 3:
	{
		UNameProperty* Prop = FindField<UNameProperty>(SelectionClass, "Name");
		for (TWeakObjectPtr<UObject>& Obj : SelectedObjects)
		{
			FName Name = Prop->GetPropertyValue_InContainer(Obj.Get());
			FArteriesPrimitiveGroup& Group = Object->PrimitiveGroups[Name];
			for (FArteriesPrimitive* Primitive : Group.Primitives)
				EditorMode->AddToSelection(Primitive);
		}
		break;
	}
	}
	EditorMode->OnSelectionChanged();
}
bool SArteriesToolbox::IsListSelected() const
{
	return ListView->GetSelectedItems().Num() > 0;
}
bool SArteriesToolbox::CouldPasteHere() const
{
	return GetPasteObjects().Num() > 0;
}
TArray<UArteriesObject*> SArteriesToolbox::GetPasteObjects() const
{
	TArray<UArteriesObject*> PasteObjects;
	FString Paste;
	FPlatformApplicationMisc::ClipboardPaste(Paste);
	TArray<FString> Strs;
	Paste.ParseIntoArray(Strs, TEXT("\r\n"));
	for (FString& Str : Strs)
	{
		if (UArteriesObject* Object = FindObject<UArteriesObject>(nullptr, *Str))
			PasteObjects.AddUnique(Object);
	}
	return MoveTemp(PasteObjects);
}
void SArteriesToolbox::AddStats(const FString& Name, int Count)
{
	UArteriesStats* Stats = NewObject<UArteriesStats>(GetTransientPackage());
	Stats->Name = Name;
	Stats->Count = Count;
	Stats->AddToRoot();
	StatsObjects.Add(Stats);
}
bool SArteriesToolbox::IsPropertyVisible(const FPropertyAndParent& PropertyAndParent) const
{
	const FName PropertyName = PropertyAndParent.Property.GetFName();
	if (PropertyName == GET_MEMBER_NAME_CHECKED(UArteriesEditorProperties, Position))
		return EditorMode->GetSelectionType() == FArteriesPoint::StaticStruct();
	if (PropertyName == GET_MEMBER_NAME_CHECKED(UArteriesEditorProperties, Material))
		return EditorMode->GetSelectionType() == FArteriesPrimitive::StaticStruct();
	return true;
}
TSharedRef<SWidget> SArteriesToolbox::BuildToolBar()
{
	FToolBarBuilder Toolbar(EditorMode->UICommandList, FMultiBoxCustomization::None, nullptr, Orient_Vertical);
	Toolbar.SetLabelVisibility(EVisibility::Collapsed);
	Toolbar.SetStyle(&FEditorStyle::Get(), "FoliageEditToolbar");
	{
		Toolbar.AddToolBarButton(FArteriesEditorCommands::Get().Select, NAME_None, TAttribute<FText>(), TAttribute<FText>(), FSlateIcon(FEditorStyle::GetStyleSetName(), "LandscapeEditor.SelectComponentTool.Small"));
		Toolbar.AddToolBarButton(FArteriesEditorCommands::Get().Create, NAME_None, TAttribute<FText>(), TAttribute<FText>(), FSlateIcon(FEditorStyle::GetStyleSetName(), "Matinee.ToggleCurveEditor.Small"));
		Toolbar.AddToolBarButton(FArteriesEditorCommands::Get().Settings, NAME_None, TAttribute<FText>(), TAttribute<FText>(), FSlateIcon(FEditorStyle::GetStyleSetName(), "LevelEditor.GameSettings.Small"));
		Toolbar.AddToolBarButton(FArteriesEditorCommands::Get().Stats, NAME_None, TAttribute<FText>(), TAttribute<FText>(), FSlateIcon(FEditorStyle::GetStyleSetName(), "MaterialEditor.ToggleBuiltinStats.Small"));
	}
	return SNew(SHorizontalBox)
	+ SHorizontalBox::Slot()
	[
		SNew(SOverlay)
		+ SOverlay::Slot()
		[
			SNew(SBorder)
			.HAlign(HAlign_Center)
			.Padding(0)
			.BorderImage(FEditorStyle::GetBrush("NoBorder"))
			.IsEnabled(FSlateApplication::Get().GetNormalExecutionAttribute())
			[
				Toolbar.MakeWidget()
			]
		]
	];
}
TSharedRef<SWidget> SArteriesToolbox::BuildSelectMode(FPropertyEditorModule& PropertyModule)
{
	SelectModeNames.Empty();
	SelectModes.Empty();
	SelectModeNames.Add("Point");
	SelectModeNames.Add("Primitive");
	SelectModeNames.Add("PointGroup");
	SelectModeNames.Add("PrimitiveGroup");
	for (int i = 0; i < SelectModeNames.Num(); i++)
		SelectModes.Add(MakeShareable(new int(i)));
	return SNew(SVerticalBox)
		.Visibility(this, &SArteriesToolbox::GetModeVisibility, 0)
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(StandardPadding)
		[
			SNew(SHeader)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("SelectionType", "Selection Type"))
				.Font(StandardFont)
			]
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(StandardPadding)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.Padding(StandardPadding)
			.FillWidth(0.5f)
			[
				SNew(SComboBox<TSharedPtr<int32>>)
				.OptionsSource(&SelectModes)
				.InitiallySelectedItem(SelectModes[0])
				.OnSelectionChanged_Lambda([this](TSharedPtr<int32> Item, ESelectInfo::Type)
				{
					EditorMode->OnSetSubMode(*Item);
				})
				.OnGenerateWidget_Lambda([this](TSharedPtr<int32> Item)
				{
					return SNew(STextBlock).Font(StandardFont).Text(FText::FromName(SelectModeNames[*Item]));
				})
				[
					SNew(STextBlock).Text_Lambda([this] { return FText::FromName(SelectModeNames[EditorMode->SubMode]); })
				]
			]
			+ SHorizontalBox::Slot()
			.Padding(StandardPadding)
			.AutoWidth()
			[
				SNew(SButton)
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				.OnClicked_Lambda([this]()->FReply {EditorMode->OnSave(); return FReply::Handled(); })
				.Text(LOCTEXT("Save", "Save"))
			]
			+ SHorizontalBox::Slot()
			.Padding(StandardPadding)
			.AutoWidth()
			[
				SNew(SButton)
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				.OnClicked_Lambda([this]()->FReply {EditorMode->OnSaveSelection(); return FReply::Handled(); })
				.Text(LOCTEXT("Save Selection", "Save Selection"))
			]
			+ SHorizontalBox::Slot()
			.Padding(StandardPadding)
			.AutoWidth()
			[
				SNew(SButton)
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				.OnClicked_Lambda([this]()->FReply {EditorMode->OnExportSelection(); return FReply::Handled(); })
				.Text(LOCTEXT("Export", "Export"))
			]
		]
		+ SVerticalBox::Slot()
		.FillHeight(1.f)
		.Padding(StandardPadding)
		[
			PropertyModule.CreatePropertyTableWidgetHandle(SelectionTable.ToSharedRef())->GetWidget()
		];
}
TSharedRef<SWidget> SArteriesToolbox::BuildCreateMode()
{
	return SNew(SVerticalBox).Visibility(this, &SArteriesToolbox::GetModeVisibility, 1);
}
TSharedRef<SWidget> SArteriesToolbox::BuildSettingsMode()
{
	return SNew(SVerticalBox)
		.Visibility(this, &SArteriesToolbox::GetModeVisibility, 2)
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(StandardPadding)
		[
			SNew(SCheckBox)
			.IsChecked_Lambda([this](){return bDisplayPoints ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;})
			.OnCheckStateChanged_Lambda([this](ECheckBoxState State){bDisplayPoints = State == ECheckBoxState::Checked;})
			[
				SNew(STextBlock).Text(LOCTEXT("DisplayPoints", "DisplayPoints"))
			]
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(StandardPadding)
		[
			SNew(SCheckBox)
			.IsChecked_Lambda([this]() {return bDisplayPrimitives ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
			.OnCheckStateChanged_Lambda([this](ECheckBoxState State) {bDisplayPrimitives = State == ECheckBoxState::Checked; })
			[
				SNew(STextBlock).Text(LOCTEXT("DisplayPrimitives", "DisplayPrimitives"))
			]
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(StandardPadding)
		[
			SNew(SCheckBox)
			.IsChecked_Lambda([this]() {return bDisplayPointIds ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
			.OnCheckStateChanged_Lambda([this](ECheckBoxState State) {bDisplayPointIds = State == ECheckBoxState::Checked; })
			[
				SNew(STextBlock).Text(LOCTEXT("DisplayPointIds", "DisplayPointIds"))
			]
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(StandardPadding)
		[
			SNew(SCheckBox)
			.IsChecked_Lambda([this]() {return bDisplayPrimitiveIds ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
			.OnCheckStateChanged_Lambda([this](ECheckBoxState State) {bDisplayPrimitiveIds = State == ECheckBoxState::Checked; })
			[
				SNew(STextBlock).Text(LOCTEXT("DisplayPrimitiveIds", "DisplayPrimitiveIds"))
			]
		];
}
TSharedRef<SWidget> SArteriesToolbox::BuildStatsMode(FPropertyEditorModule& PropertyModule)
{
	return SNew(SVerticalBox)
		.Visibility(this, &SArteriesToolbox::GetModeVisibility, 3)
		+SVerticalBox::Slot()
		.FillHeight(1.f)
		.Padding(2)
		[
			PropertyModule.CreatePropertyTableWidgetHandle(StatsTable.ToSharedRef())->GetWidget()
		];
}
EVisibility SArteriesToolbox::GetModeVisibility(int Mode) const
{
	return EditorMode->Mode == Mode ? EVisibility::Visible : EVisibility::Collapsed;
}
EVisibility SArteriesToolbox::GetSelectPointVisibility() const
{
	return EditorMode->Mode == 0 && EditorMode->GetSelectionType() == FArteriesPoint::StaticStruct() ? EVisibility::Visible : EVisibility::Collapsed;
}
EVisibility SArteriesToolbox::GetSelectPrimitiveVisibility() const
{
	return EditorMode->Mode == 0 && EditorMode->GetSelectionType() == FArteriesPrimitive::StaticStruct() ? EVisibility::Visible : EVisibility::Collapsed;
}

END_SLATE_FUNCTION_BUILD_OPTIMIZATION
#undef LOCTEXT_NAMESPACE
