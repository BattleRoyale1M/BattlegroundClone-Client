// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class BattlegroundClone : ModuleRules
{
	public BattlegroundClone(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"UMG",
			"Niagara",
			"AIModule",
			"NavigationSystem",
			"GameplayTasks",
			"PhysicsCore"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore", "HTTP", "Json", "JsonUtilities", "Sockets" });

		PublicIncludePaths.AddRange(new string[] {
			"BattlegroundClone"
		});
	}
}
