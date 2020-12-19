// Author: LiJiayu (JerryLi)
// Mail: lijiayu83@gmail.com (fullike@163.com)
// Copyright 2019. All Rights Reserved.

namespace UnrealBuildTool.Rules
{
	public class Arteries : ModuleRules
	{
		public Arteries(ReadOnlyTargetRules Target) : base(Target)
		{
            PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
            PrivatePCHHeaderFile = "Public/Arteries.h";
            bEnableUndefinedIdentifierWarnings = false;
            bEnableExceptions = true;
            PublicIncludePaths.AddRange(
				new string[] {
					// ... add public include paths required here ...
				}
				);
            string ThirdPartyDir = System.IO.Path.Combine(ModuleDirectory, "ThirdParty/");
            PublicIncludePaths.AddRange(
                new string[]
                {
                    ThirdPartyDir,
                    ThirdPartyDir + "GluTess",
                    ThirdPartyDir + "Lua",
                    ThirdPartyDir + "LuaBridge",
                    ThirdPartyDir + "Voronoi",
                }
                );
            PublicDependencyModuleNames.AddRange(
				new string[]
				{
					"Core",
                    "CoreUObject",
                    "Engine",
                    "ProceduralMeshComponent",
                    "RenderCore",
                    "RHI",
					// ... add other public dependencies that you statically link with here ...
				}
				);
            PrivateDependencyModuleNames.AddRange(
				new string[]
				{
					// ... add private dependencies that you statically link with here ...
				}
				);

			DynamicallyLoadedModuleNames.AddRange(
				new string[]
				{
					// ... add any modules that your module loads dynamically here ...
				}
				);
        }
	}
}
