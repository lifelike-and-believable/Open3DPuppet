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

		// AnimGraph and BlueprintGraph: UAnimGraphNode_PuppetSolve and its UK2Node base, as the engine's
		// LiveLinkGraphNode module does for the Live Link Pose node. AssetDefinition: how the Content
		// Browser shows the plugin's asset types.
		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"AssetDefinition",
			"Core",
			"CoreUObject",
			"Engine",
			"AnimGraph",
			"BlueprintGraph",
			"LiveLinkInterface",
			"UnrealEd",
			"Open3DPuppet"
		});
	}
}
