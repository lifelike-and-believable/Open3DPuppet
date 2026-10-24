---
id: OP-006
title: New puppet modules follow the module contract
level: MUST
scope: project
paths: ["**/Open3DPuppet/**/PuppetModules/**", "**/Open3DPuppet/**/PuppetModule*.h"]
verified-by: [review]
targets-failure: convention-drift
observed-on: []
rationale: The library only stays composable if every module reads performer data and writes puppet bones the same way (ADR 0002).
---
A new puppet module subclasses `UPuppetModule` and follows `docs/guides/authoring-puppet-modules.md`: it reads only the performer channels it declares, writes only the puppet bones it claims, keeps per-frame work allocation-free and off the game thread, exposes its retargeting controls as `UPROPERTY` with a `Category` and tooltip, and has solver tests under `Open3DPuppetTests/Private/Modules/`. Changing the contract itself (the `UPuppetModule` virtuals, `FPuppetSolveContext`, the performer pose types) needs an ADR.
