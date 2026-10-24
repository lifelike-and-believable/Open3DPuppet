---
id: OP-003
title: Where UE tests live and how they are named
level: SHOULD
scope: project
paths: ["ProjectSandbox/Plugins/**/*.cpp", "ProjectSandbox/Plugins/**/*.h"]
verified-by: [ci]
check: "ci: UE build and tests (Run-AutomationTests.ps1 -TestFilter Open3DPuppet)"
targets-failure: convention-drift
observed-on: []
rationale: ADR 0005 keeps tests out of Runtime modules and the test filter finds them by name; replaces UE-007.
---
Put UE automation tests in `Source/Open3DPuppetTests/Private/<Area>/`, never in a Runtime module. Name them `Open3DPuppet.<Area>.<Unit>.<Case>` and wrap them in `#if WITH_DEV_AUTOMATION_TESTS`. Reach internals through the owning module's `Public/Testing/*.h`. Every puppet module in the library has solver tests that run on a skeleton built in code (ADR 0005). Decide pass or fail from the automation report, not the editor's exit code.
