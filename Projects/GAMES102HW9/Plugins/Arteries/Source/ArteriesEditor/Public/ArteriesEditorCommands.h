// Author: LiJiayu (JerryLi)
// Mail: lijiayu83@gmail.com (fullike@163.com)
// Copyright 2019. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "EditorStyleSet.h"
#include "Framework/Commands/Commands.h"

/**
 * Unreal Foliage Edit mode actions
 */
class FArteriesEditorCommands : public TCommands<FArteriesEditorCommands>
{

public:
	FArteriesEditorCommands() : TCommands<FArteriesEditorCommands>
	(
		"ArteriesEditorMode", // Context name for fast lookup
		NSLOCTEXT("Contexts", "ArteriesEditorMode", "Arteries Editor Mode"), // Localized context name for displaying
		NAME_None, // Parent
		FEditorStyle::GetStyleSetName() // Icon Style Set
	)
	{
	}
	/** Commands for the tools toolbar. */
	TSharedPtr< FUICommandInfo > Select;
	TSharedPtr< FUICommandInfo > Create;
	TSharedPtr< FUICommandInfo > Settings;
	TSharedPtr< FUICommandInfo > Stats;
	TSharedPtr< FUICommandInfo > InternalObjects;

	TSharedPtr< FUICommandInfo > Delete;
	TSharedPtr< FUICommandInfo > Reverse;
	TSharedPtr< FUICommandInfo > FlipX;
	TSharedPtr< FUICommandInfo > FlipY;
	TSharedPtr< FUICommandInfo > FlipZ;

	TSharedPtr< FUICommandInfo > DisplayThisNode;
	TSharedPtr< FUICommandInfo > DisplayFinalResult;
	TSharedPtr< FUICommandInfo > Build;

	TSharedPtr< FUICommandInfo > EnableSimulation;
	TSharedPtr< FUICommandInfo > ResetCamera;
	TSharedPtr< FUICommandInfo > ShowFloor;
	TSharedPtr< FUICommandInfo > ShowGrid;
	/**
	 * Initialize commands
	 */
	virtual void RegisterCommands() override;

public:
};
