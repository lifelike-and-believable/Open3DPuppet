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

		// The Control Rig adapter's tests run a native rig built in code (ADR 0010); the physics
		// tests read the Physics Control parameters the physics module makes (ADR 0007).
		// The asset type tests read the editor's factories and asset definitions; the session tests
		// the session panel's model and its tab.
		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"AssetDefinition",
			"ControlRig",
			"LiveLinkInterface",
			"Open3DPuppetControlRig",
			"Open3DPuppetEditor",
			"Open3DPuppetPhysics",
			"PhysicsControl",
			"RigVM",
			"Slate",
			"SlateCore",
			"UnrealEd"
		});
	}
}
