// Author: LiJiayu (JerryLi)
// Mail: lijiayu83@gmail.com (fullike@163.com)
// Copyright 2019. All Rights Reserved.

namespace UnrealBuildTool.Rules
{
	public class ArteriesEditor : ModuleRules
	{
		public ArteriesEditor(ReadOnlyTargetRules Target) : base(Target)
		{
            PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
            PrivatePCHHeaderFile = "Public/ArteriesEditor.h";

			PublicDependencyModuleNames.AddRange(
				new string[]
				{
                    "ApplicationCore",
                    "Arteries",
                    "BlueprintGraph",
                    "ContentBrowser",
                    "Core",
                    "CoreUObject",
                    "DetailCustomizations",
                    "EditorStyle",
                    "Engine",
                    "GraphEditor",
                    "InputCore",
                    "Json",
                    "Kismet",
                    "KismetCompiler",
                    "ProceduralMeshComponent",
                    "PropertyEditor",
                    "RenderCore",
                    "RHI",
                    "Slate",
                    "SlateCore",
                    "UnrealEd",
                    "XmlParser",
                });

			PrivateDependencyModuleNames.AddRange(
				new string[]
				{
					// ... add private dependencies that you statically link with here ...
				});

            PublicIncludePaths.AddRange(
                new string[] {
					// ... add public include paths required here ...
				});

            PrivateIncludePathModuleNames.AddRange(
                new string[] {
                "AssetTools",
                "AssetRegistry"
                });

            DynamicallyLoadedModuleNames.AddRange(
                new string[] {
                "AssetTools",
                "AssetRegistry"
                });
        }
	}
}
