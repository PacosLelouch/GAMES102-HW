// Author: LiJiayu (JerryLi)
// Mail: lijiayu83@gmail.com (fullike@163.com)
// Copyright 2019. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"
#include "Modules/ModuleManager.h"
#include "WorkflowOrientedApp/WorkflowTabManager.h"
#include "ArteriesToolbox.h"
#include "ArteriesViewport.h"
#include "ArteriesEditorProperties.h"
#include "ArteriesExtensions.h"

/**
 * The public interface to this module.  In most cases, this interface is only public to sibling modules 
 * within this plugin.
 */
class FArteriesEditorModule : public IModuleInterface
{

public:

	/**
	 * Singleton-like access to this module's interface.  This is just for convenience!
	 * Beware of calling this during the shutdown phase, though.  Your module might have been unloaded already.
	 *
	 * @return Returns singleton instance, loading the module on demand if needed
	 */
	static inline FArteriesEditorModule& Get()
	{
		return FModuleManager::LoadModuleChecked< FArteriesEditorModule >( "ArteriesEditor" );
	}

	/**
	 * Checks to see if this module is loaded and ready.  It is only valid to call Get() if IsAvailable() returns true.
	 *
	 * @return True if the module is loaded and ready to use
	 */
	static inline bool IsAvailable()
	{
		return FModuleManager::Get().IsModuleLoaded( "ArteriesEditor" );
	}

	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	void OnPostEngineInit();
	void OnPreExit();
	void OnMapOpened(const FString& Filename, bool bAsTemplate);
	void RegisterBlueprintEditorTab(FWorkflowAllowedTabSet& TabFactories, FName InModeName, TSharedPtr<FBlueprintEditor> BlueprintEditor);
	TSharedPtr<FArteriesExtensions> Extensions;
	FDelegateHandle BlueprintEditorTabSpawnerHandle;
	FDelegateHandle CoreDelegateHandle;
	FDelegateHandle OnMapOpenedHandle;
};

class FArteriesToolboxWorkflowTabFactory : public FWorkflowTabFactory
{
public:
	FArteriesToolboxWorkflowTabFactory(TSharedPtr<FBlueprintEditor> BlueprintEditor, TSharedRef<SArteriesToolbox> InToolbox);
	virtual TSharedRef<SWidget> CreateTabBody(const FWorkflowTabSpawnInfo& Info) const override;
protected:
	TSharedRef<SArteriesToolbox> Toolbox;
};

class FArteriesViewportWorkflowTabFactory : public FWorkflowTabFactory
{
public:
	FArteriesViewportWorkflowTabFactory(TSharedPtr<FBlueprintEditor> BlueprintEditor, TSharedRef<SArteriesViewport> InViewport);
	virtual TSharedRef<SWidget> CreateTabBody(const FWorkflowTabSpawnInfo& Info) const override;
protected:
	TSharedRef<SArteriesViewport> Viewport;
};
DECLARE_LOG_CATEGORY_EXTERN(LogArteriesEditor, Verbose, All);