// Copyright 2026 Lifelike & Believable. All Rights Reserved.

using UnrealBuildTool;

// Editor-only code: the AnimGraph node and asset tools (ADR 0002). Runtime modules never depend
// on this module; Build/Scripts/check-runtime-editor-deps.py enforces it.
[SupportedTargetTypes(TargetType.Editor)]
public class Open3DPuppetEditor : ModuleRules
{
	public Open3DPuppetEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"Open3DPuppet"
		});
	}
}
