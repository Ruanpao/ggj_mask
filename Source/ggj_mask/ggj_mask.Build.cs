// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class ggj_mask : ModuleRules
{
	public ggj_mask(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput" });
	}
}
