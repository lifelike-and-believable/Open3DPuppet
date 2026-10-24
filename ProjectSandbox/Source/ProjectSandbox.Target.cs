// Copyright 2026 Lifelike & Believable. All Rights Reserved.

using UnrealBuildTool;

public class ProjectSandboxTarget : TargetRules
{
	public ProjectSandboxTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		// UE 5.8 is the oldest supported engine (ADR 0001, rule OP-001).
		DefaultBuildSettings = BuildSettingsVersion.V7;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_8;

		ExtraModuleNames.Add("ProjectSandbox");
	}
}
