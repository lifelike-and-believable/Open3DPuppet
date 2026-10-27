// Copyright 2026 Lifelike & Believable. All Rights Reserved.

using UnrealBuildTool;

// The Control Rig adapter module (ADR 0010), apart from Open3DPuppet so the core does not depend
// on Control Rig (ADR 0002). Editor, Game and Client, as the core.
[SupportedTargetTypes(TargetType.Editor, TargetType.Game, TargetType.Client)]
public class Open3DPuppetControlRig : ModuleRules
{
	public Open3DPuppetControlRig(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		// ControlRig and RigVM are public: the module's settings hold an FControlRigAssetStrongReference.
		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"ControlRig",
			"RigVM",
			"Open3DPuppet"
		});
	}
}
