// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class CameraProject : ModuleRules
{
	public CameraProject(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"UMG",
			"Slate"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] {
			"CameraProject",
			"CameraProject/LockOn",
			"CameraProject/Tests",
			"CameraProject/Variant_Platforming",
			"CameraProject/Variant_Platforming/Animation",
			"CameraProject/Variant_Combat",
			"CameraProject/Variant_Combat/AI",
			"CameraProject/Variant_Combat/Animation",
			"CameraProject/Variant_Combat/Gameplay",
			"CameraProject/Variant_Combat/Interfaces",
			"CameraProject/Variant_Combat/UI",
			"CameraProject/Variant_SideScrolling",
			"CameraProject/Variant_SideScrolling/AI",
			"CameraProject/Variant_SideScrolling/Gameplay",
			"CameraProject/Variant_SideScrolling/Interfaces",
			"CameraProject/Variant_SideScrolling/UI"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
