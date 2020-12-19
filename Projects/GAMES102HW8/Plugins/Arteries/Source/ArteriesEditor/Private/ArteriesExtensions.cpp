// Author: LiJiayu (JerryLi)
// Mail: lijiayu83@gmail.com (fullike@163.com)
// Copyright 2019. All Rights Reserved.

#include "ArteriesExtensions.h"
#include "ArteriesObject.h"
#include "ArteriesActor.h"
#include "ArteriesEditor.h"
#include "ArteriesEditorCommands.h"
#include "ArteriesEditorProperties.h"
#include "ArteriesEditorMode.h"
#include "Modules/ModuleManager.h"
#include "ObjectTools.h"
#include "ContentBrowserModule.h"
#include "GraphEditorModule.h"
#include "EditorModeManager.h"
#include "BlueprintEditor.h"
#include "Editor/EditorEngine.h"
#include "K2Node_CallFunction.h"
#define LOCTEXT_NAMESPACE "ArteriesEditor"

void FArteriesExtensions::InstallHooks()
{
	FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser"));
	FContentBrowserMenuExtender_SelectedAssets AssetDelegate = FContentBrowserMenuExtender_SelectedAssets::CreateSP(this, &FArteriesExtensions::OnContentBrowserContextMenu);
	ContentBrowserModule.GetAllAssetViewContextMenuExtenders().Add(AssetDelegate);
	AssetDelegateHandle = AssetDelegate.GetHandle();

	FGraphEditorModule& GraphEditorModule = FModuleManager::LoadModuleChecked<FGraphEditorModule>(TEXT("GraphEditor"));
	FGraphEditorModule::FGraphEditorMenuExtender_SelectedNode NodeDelegate = FGraphEditorModule::FGraphEditorMenuExtender_SelectedNode::CreateSP(this, &FArteriesExtensions::OnGraphEditorContextMenu);
	GraphEditorModule.GetAllGraphEditorContextMenuExtender().Add(NodeDelegate);
	GraphEditorDelegateHandle = NodeDelegate.GetHandle();

	FBlueprintEditorModule& BlueprintEditorModule = FModuleManager::LoadModuleChecked<FBlueprintEditorModule>("Kismet");
	BlueprintEditorModule.OnGatherBlueprintMenuExtensions().AddRaw(this, &FArteriesExtensions::OnGatherExtensions);
}

void FArteriesExtensions::RemoveHooks()
{
	FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser"));
	ContentBrowserModule.GetAllAssetViewContextMenuExtenders().RemoveAll([&](const FContentBrowserMenuExtender_SelectedAssets& Delegate)
	{
		return Delegate.GetHandle() == AssetDelegateHandle;
	});

	FGraphEditorModule& GraphEditorModule = FModuleManager::LoadModuleChecked<FGraphEditorModule>(TEXT("GraphEditor"));
	GraphEditorModule.GetAllGraphEditorContextMenuExtender().RemoveAll([&](const FGraphEditorModule::FGraphEditorMenuExtender_SelectedNode& Delegate)
	{
		return Delegate.GetHandle() == GraphEditorDelegateHandle;
	});
}
TSharedRef<FExtender> FArteriesExtensions::OnContentBrowserContextMenu(const TArray<FAssetData>& Selection)
{
	SelectedAssets = Selection;
	TSharedRef<FExtender> Extender(new FExtender());
	Extender->AddMenuExtension("CommonAssetActions", EExtensionHook::Before, nullptr, FMenuExtensionDelegate::CreateSP(this, &FArteriesExtensions::OnExtendContentBrowserMenu));
	return Extender;
}
TSharedRef<FExtender> FArteriesExtensions::OnGraphEditorContextMenu(const TSharedRef<FUICommandList>, const UEdGraph* InGraph, const UEdGraphNode* InNode, const UEdGraphPin* InPin, bool bConst)
{
	Graph = InGraph;
	Node = InNode; 

	FBlueprintEditor* BlueprintEditor = (FBlueprintEditor*)GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->FindEditorForAsset(InGraph->GetTypedOuter<UBlueprint>(), false);
	//FBlueprintEditor* BlueprintEditor = (FBlueprintEditor*)FAssetEditorManager::Get().FindEditorForAsset(InGraph->GetTypedOuter<UBlueprint>(), false);
	TSharedRef<FExtender> Extender(new FExtender());
	Extender->AddMenuExtension("EdGraphSchemaNodeActions", EExtensionHook::Before, BlueprintEditor->GetToolkitCommands(), FMenuExtensionDelegate::CreateSP(this, &FArteriesExtensions::OnExtendGraphEditorMenu));
	return Extender;
}
void FArteriesExtensions::ExtendToolBar(FToolBarBuilder& Builder)
{
	FSlateIcon IconBrush = FSlateIcon(
		FEditorStyle::GetStyleSetName(),
		"LevelEditor.Build",
		"LevelEditor.Build.Small");

	const FArteriesEditorCommands& Commands = FArteriesEditorCommands::Get();
	Builder.AddToolBarButton(
		Commands.Build,
		NAME_None,
		TAttribute<FText>(),
		TAttribute<FText>(),
		IconBrush,
		NAME_None);
}
void FArteriesExtensions::OnGatherExtensions(TSharedPtr<FExtender> Extender, UBlueprint* Blueprint)
{
	if (Blueprint->GeneratedClass->IsChildOf<AArteriesActor>())
		Extender->AddToolBarExtension("Asset", EExtensionHook::Before, TSharedPtr<FUICommandList>(), FToolBarExtensionDelegate::CreateRaw(this, &FArteriesExtensions::ExtendToolBar));
}
void FArteriesExtensions::OnExtendContentBrowserMenu(FMenuBuilder& MenuBuilder)
{
}
void FArteriesExtensions::OnExtendGraphEditorMenu(FMenuBuilder& MenuBuilder)
{
	if (const UK2Node_CallFunction* CallFunc = Cast<UK2Node_CallFunction>(Node))
	{
		if (CallFunc->FunctionReference.GetMemberParentClass() == UArteriesObject::StaticClass())
		{
			const FArteriesEditorCommands& Commands = FArteriesEditorCommands::Get();
			MenuBuilder.BeginSection("Arteries", LOCTEXT("Arteries", "Arteries"));
			MenuBuilder.AddMenuEntry(Commands.DisplayThisNode);
			MenuBuilder.AddMenuEntry(Commands.DisplayFinalResult);
			MenuBuilder.EndSection();
		}
	}
}
#undef LOCTEXT_NAMESPACE