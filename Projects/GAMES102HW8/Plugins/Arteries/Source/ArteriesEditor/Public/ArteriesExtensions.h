// Author: LiJiayu (JerryLi)
// Mail: lijiayu83@gmail.com (fullike@163.com)
// Copyright 2019. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Framework/MultiBox/MultiBoxExtender.h"
#include "AssetRegistryModule.h"

class UEdGraph;
class UEdGraphNode;
class UEdGraphPin;

class FArteriesExtensions : public TSharedFromThis<FArteriesExtensions>
{
public:
	void InstallHooks();
	void RemoveHooks();
private:
	TSharedRef<FExtender> OnContentBrowserContextMenu(const TArray<FAssetData>& Selection);
	TSharedRef<FExtender> OnGraphEditorContextMenu(const TSharedRef<FUICommandList> UICommandList, const UEdGraph* InGraph, const UEdGraphNode* InNode, const UEdGraphPin* InPin, bool bConst);
	void ExtendToolBar(FToolBarBuilder& Builder);
	void OnGatherExtensions(TSharedPtr<FExtender> Extender, UBlueprint* Blueprint);
	void OnExtendContentBrowserMenu(FMenuBuilder& MenuBuilder);
	void OnExtendGraphEditorMenu(FMenuBuilder& MenuBuilder);
	TArray<FAssetData> SelectedAssets;
	const UEdGraph* Graph;
	const UEdGraphNode* Node;
	FDelegateHandle AssetDelegateHandle;
	FDelegateHandle GraphEditorDelegateHandle;
};