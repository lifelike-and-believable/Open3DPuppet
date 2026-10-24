---
id: OP-005
title: Descriptor requirements for the plugin
level: SHOULD
scope: project
paths: ["**/Open3DPuppet.uplugin"]
verified-by: [review]
targets-failure: project-decision
observed-on: []
rationale: The plugin is meant for Fab (ADR 0001); replaces FAB-001.
---
In `Open3DPuppet.uplugin`, leave `EngineVersion` out (packaging stamps the engine that built the package), give every module `"PlatformAllowList": [ "Win64" ]` (ADR 0001), and depend only on plugins that ship with the engine. Add `FabURL` once the Fab listing exists.
