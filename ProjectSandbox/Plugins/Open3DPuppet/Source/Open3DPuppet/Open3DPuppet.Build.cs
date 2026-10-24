// Copyright 2026 Lifelike & Believable. All Rights Reserved.

using UnrealBuildTool;

// Editor, Game and Client; Server and Program are excluded (ADR 0001).
[SupportedTargetTypes(TargetType.Editor, TargetType.Game, TargetType.Client)]
public class Open3DPuppet : ModuleRules
{
	public Open3DPuppet(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine"
		});
	}
}
