// Copyright 2026 Lifelike & Believable. All Rights Reserved.

using UnrealBuildTool;

// Automation tests and fixtures (ADR 0005). Type "Editor" in the .uplugin, so it loads only in
// editor targets. Runtime modules never depend on this module.
[SupportedTargetTypes(TargetType.Editor)]
public class Open3DPuppetTests : ModuleRules
{
	public Open3DPuppetTests(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"Open3DPuppet"
		});

		// The asset type tests read the editor's factories and asset definitions.
		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"AssetDefinition",
			"UnrealEd"
		});
	}
}
