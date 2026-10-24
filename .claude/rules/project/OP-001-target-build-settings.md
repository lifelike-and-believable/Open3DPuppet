---
id: OP-001
title: Target.cs build settings (UE 5.8 and later)
level: SHOULD
scope: project
paths: ["**/*.Target.cs"]
verified-by: [review]
targets-failure: project-decision
observed-on: []
rationale: The project supports UE 5.8 and later (ADR 0001), so the settings are pinned to 5.8 rather than Latest; replaces UE-006.
---
In every `*.Target.cs`, set `DefaultBuildSettings = BuildSettingsVersion.V7;` and `IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_8;`, because 5.8 is the oldest engine the project supports (ADR 0001). When a later engine is added to the supported set, select its settings under `#if UE_5_<n>_OR_LATER` (UnrealBuildTool defines these up to its own minor version) in the ADR's PR. Build with the commands in AGENTS.md.
