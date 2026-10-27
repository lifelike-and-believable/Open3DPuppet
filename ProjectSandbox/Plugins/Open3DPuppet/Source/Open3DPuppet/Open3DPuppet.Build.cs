// Copyright 2026 Lifelike & Believable. All Rights Reserved.

using UnrealBuildTool;

// Editor, Game and Client; Server and Program are excluded (ADR 0001).
[SupportedTargetTypes(TargetType.Editor, TargetType.Game, TargetType.Client)]
public class Open3DPuppet : ModuleRules
{
	public Open3DPuppet(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		// LiveLinkInterface is public: the AnimGraph node and the calibration subsystem take
		// FLiveLinkSubjectName (ADR 0003).
		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"DeveloperSettings",
			"LiveLinkInterface"
		});

		// AnimationCore: two-bone IK for the limb module.
		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"AnimationCore"
		});
	}
}
