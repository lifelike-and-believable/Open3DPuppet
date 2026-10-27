// Copyright 2026 Lifelike & Believable. All Rights Reserved.

using UnrealBuildTool;

// The engine physics hand-off (ADR 0007, Tier 2), apart from Open3DPuppet so the core does not
// depend on Physics Control (ADR 0011). Editor, Game and Client, as the core.
[SupportedTargetTypes(TargetType.Editor, TargetType.Game, TargetType.Client)]
public class Open3DPuppetPhysics : ModuleRules
{
	public Open3DPuppetPhysics(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		// PhysicsControl is public: the function library returns its control parameters.
		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"PhysicsControl",
			"Open3DPuppet"
		});
	}
}
