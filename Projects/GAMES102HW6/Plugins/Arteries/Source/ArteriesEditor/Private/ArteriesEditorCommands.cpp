// Author: LiJiayu (JerryLi)
// Mail: lijiayu83@gmail.com (fullike@163.com)
// Copyright 2019. All Rights Reserved.

#include "ArteriesEditorCommands.h"

#define LOCTEXT_NAMESPACE "ArteriesEditorCommands"

void FArteriesEditorCommands::RegisterCommands()
{
	UI_COMMAND(Select, "Select", "Select", EUserInterfaceActionType::ToggleButton, FInputChord());
	UI_COMMAND(Create, "Create", "Create", EUserInterfaceActionType::ToggleButton, FInputChord());
	UI_COMMAND(Settings, "Settings", "Settings", EUserInterfaceActionType::ToggleButton, FInputChord());
	UI_COMMAND(Stats, "Stats", "Stats", EUserInterfaceActionType::ToggleButton, FInputChord());
	UI_COMMAND(InternalObjects, "Internal Objects", "Internal Objects", EUserInterfaceActionType::ToggleButton, FInputChord());

	UI_COMMAND(Delete, "Delete", "Delete", EUserInterfaceActionType::Button, FInputChord(EKeys::Delete));
	UI_COMMAND(Reverse, "Reverse", "Reverse", EUserInterfaceActionType::Button, FInputChord(EKeys::R));
	UI_COMMAND(FlipX, "FlipX", "FlipX", EUserInterfaceActionType::Button, FInputChord(EKeys::X));
	UI_COMMAND(FlipY, "FlipY", "FlipY", EUserInterfaceActionType::Button, FInputChord(EKeys::Y));
	UI_COMMAND(FlipZ, "FlipZ", "FlipZ", EUserInterfaceActionType::Button, FInputChord(EKeys::Z));

	UI_COMMAND(DisplayThisNode, "DisplayThisNode", "DisplayThisNode", EUserInterfaceActionType::Button, FInputChord(EKeys::D));
	UI_COMMAND(DisplayFinalResult, "DisplayFinalResult", "DisplayFinalResult", EUserInterfaceActionType::Button, FInputChord(EKeys::F));
	UI_COMMAND(Build, "Build", "Build", EUserInterfaceActionType::Button, FInputChord(EKeys::B));

	// Preview commands
	UI_COMMAND(ResetCamera, "Reset Camera", "Resets the camera to focus on the mesh", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(EnableSimulation, "Simulation", "Enables the simulation of the blueprint and ticking", EUserInterfaceActionType::ToggleButton, FInputChord());
	UI_COMMAND(ShowFloor, "Show Floor", "Toggles a ground mesh for collision", EUserInterfaceActionType::ToggleButton, FInputChord());
	UI_COMMAND(ShowGrid, "Show Grid", "Toggles viewport grid", EUserInterfaceActionType::ToggleButton, FInputChord());
}

#undef LOCTEXT_NAMESPACE
