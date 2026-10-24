// Copyright 2026 Lifelike & Believable. All Rights Reserved.

using UnrealBuildTool;

public class ProjectSandbox : ModuleRules
{
	public ProjectSandbox(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine" });
	}
}
