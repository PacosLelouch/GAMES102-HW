// Author: LiJiayu (JerryLi)
// Mail: lijiayu83@gmail.com (fullike@163.com)
// Copyright 2019. All Rights Reserved.

#include "ArteriesEditor.h"
#include "ArteriesEditorMode.h"
#include "ArteriesEditorCommands.h"
#include "ArteriesObjectFactory.h"
#include "ArteriesActor.h"
#include "EditorModeRegistry.h"
#include "AssetToolsModule.h"
#include "Modules/ModuleManager.h"
#include "EditorModeManager.h"
#include "DetailCustomizations.h"
#include "BlueprintEditor.h"

IMPLEMENT_MODULE( FArteriesEditorModule, ArteriesEditor )

#define LOCTEXT_NAMESPACE "ArteriesEditor"
void FArteriesEditorModule::StartupModule()
{
	FArteriesEditorCommands::Register();
	FEditorModeRegistry::Get().RegisterMode<FArteriesEditorMode>(FArteriesEditorMode::ID,
		LOCTEXT("ModeName", "Mesh Editor"),
		FSlateIcon(FEditorStyle::GetStyleSetName(), "LevelEditor.MeshPaintMode", "LevelEditor.MeshPaintMode.Small"));
	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
	EAssetTypeCategories::Type Category = AssetTools.RegisterAdvancedAssetCategory(FName(TEXT("Arteries")), LOCTEXT("Arteries", "Arteries"));
	AssetTools.RegisterAssetTypeActions(MakeShareable(new FArteriesObjectTypeActions(Category)));
	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	PropertyModule.RegisterCustomClassLayout("ArteriesActor", FOnGetDetailCustomizationInstance::CreateStatic(&FArteriesActorDetailCustomization::MakeInstance));
	// Register for assets being opened
	CoreDelegateHandle = FCoreDelegates::OnPostEngineInit.AddRaw(this, &FArteriesEditorModule::OnPostEngineInit);
	OnMapOpenedHandle = FEditorDelegates::OnMapOpened.AddRaw(this, &FArteriesEditorModule::OnMapOpened);
}
void FArteriesEditorModule::OnMapOpened(const FString& Filename, bool bAsTemplate)
{
	GWorld->InitializeActorsForPlay(FURL());
}
void FArteriesEditorModule::ShutdownModule()
{
	FEditorDelegates::OnMapOpened.Remove(OnMapOpenedHandle);
	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	PropertyModule.UnregisterCustomClassLayout("ArteriesActor");
	FEditorModeRegistry::Get().UnregisterMode(FArteriesEditorMode::ID);
}
void FArteriesEditorModule::OnPostEngineInit()
{
	Extensions = MakeShareable(new FArteriesExtensions);
	Extensions->InstallHooks();

	FBlueprintEditorModule& BlueprintEditorModule = FModuleManager::LoadModuleChecked<FBlueprintEditorModule>("Kismet");
	BlueprintEditorTabSpawnerHandle = BlueprintEditorModule.OnRegisterTabsForEditor().AddRaw(this, &FArteriesEditorModule::RegisterBlueprintEditorTab);

	FCoreDelegates::OnPostEngineInit.Remove(CoreDelegateHandle);
	CoreDelegateHandle = FCoreDelegates::OnPreExit.AddRaw(this, &FArteriesEditorModule::OnPreExit);
}
void FArteriesEditorModule::OnPreExit()
{
	FBlueprintEditorModule* BlueprintEditorModule = FModuleManager::GetModulePtr<FBlueprintEditorModule>("Kismet");
	BlueprintEditorModule->OnRegisterTabsForEditor().Remove(BlueprintEditorTabSpawnerHandle);

	FCoreDelegates::OnPreExit.Remove(CoreDelegateHandle);
	Extensions->RemoveHooks();
}
void FArteriesEditorModule::RegisterBlueprintEditorTab(FWorkflowAllowedTabSet& TabFactories, FName InModeName, TSharedPtr<FBlueprintEditor> BlueprintEditor)
{
	if (UBlueprint* BP = BlueprintEditor->GetBlueprintObj())
	{
		if (BP->ParentClass->IsChildOf(AArteriesActor::StaticClass()))
		{
			TSharedRef<SArteriesViewport> Viewport = SNew(SArteriesViewport).BlueprintEditor(BlueprintEditor);
			TSharedRef<SArteriesToolbox> Toolbox = SNew(SArteriesToolbox).Client((FArteriesViewportClient*)Viewport->GetViewportClient().Get())
				.EditorMode((FArteriesEditorMode*)Viewport->GetViewportClient()->GetModeTools()->GetActiveMode(FArteriesEditorMode::ID));
			TabFactories.RegisterFactory(MakeShared<FArteriesToolboxWorkflowTabFactory>(BlueprintEditor, Toolbox));
			TabFactories.RegisterFactory(MakeShared<FArteriesViewportWorkflowTabFactory>(BlueprintEditor, Viewport));
		}
	}
}
#undef LOCTEXT_NAMESPACE
FArteriesToolboxWorkflowTabFactory::FArteriesToolboxWorkflowTabFactory(TSharedPtr<FBlueprintEditor> BlueprintEditor, TSharedRef<SArteriesToolbox> InToolbox)
	: FWorkflowTabFactory("Arteries Toolbox", BlueprintEditor)
	, Toolbox(InToolbox)
{
	TabIcon = FSlateIcon(FEditorStyle::GetStyleSetName(), "Toolbar.Icon");
}
TSharedRef<SWidget> FArteriesToolboxWorkflowTabFactory::CreateTabBody(const FWorkflowTabSpawnInfo& Info) const
{
	return Toolbox;
}
FArteriesViewportWorkflowTabFactory::FArteriesViewportWorkflowTabFactory(TSharedPtr<FBlueprintEditor> BlueprintEditor, TSharedRef<SArteriesViewport> InViewport)
	: FWorkflowTabFactory("Arteries Viewport", BlueprintEditor)
	, Viewport(InViewport)
{
	TabIcon = FSlateIcon(FEditorStyle::GetStyleSetName(), "LevelEditor.Tabs.Viewports");
}
TSharedRef<SWidget> FArteriesViewportWorkflowTabFactory::CreateTabBody(const FWorkflowTabSpawnInfo& Info) const
{
	return Viewport;
}
DEFINE_LOG_CATEGORY(LogArteriesEditor);