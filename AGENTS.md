# Open3DPuppet

An Unreal Engine plugin for mocap-driven virtual puppetry. A user takes a skeletal mesh, attaches
a set of puppet modules to it (biped core, limbs, digitigrade legs, tails, wings, props), and
puppeteers it right away from live motion capture or recorded motion. Each module carries its own
retargeting controls, and a puppet can be driven by more than one performer. Performer input
comes from Live Link, so any capture system with a Live Link source works. This file is the one
instruction file for coding agents (Claude Code reads it through `CLAUDE.md`, Copilot reads it
directly). Rules for Claude Code are in `.claude/rules/` (rules-for-robots core and
`unreal-plugin` packs, plus project rules prefixed `OP-`).

## Where things are

- Plugin: `ProjectSandbox/Plugins/Open3DPuppet/`. Host project: `ProjectSandbox/ProjectSandbox.uproject`.
- Design: `docs/design/overview.md` (read it first). Decisions: `docs/adr/`. Plan and current state: `docs/roadmap/roadmap.md`. Where the last session left things and what to do next: `docs/roadmap/handoff.md` (read it first in a new session). Writing a new puppet module: `docs/guides/authoring-puppet-modules.md`. Build and CI: `Build/README.md`.
- Across components (Open3DBroadcast, VMCLiveLink, Open3DPuppet): the contracts between them (Live Link roles, subject, bone and curve names), the engine versions that work together, cross-repository decisions and show milestones are in [`lifelike-and-believable/live-virtual-performance`](https://github.com/lifelike-and-believable/live-virtual-performance) (private). A change to a contract another component relies on is written there first. Issues use the shared labels `desk`, `needs-decision`, `interface` and `show:<name>` (its `docs/process.md`).

## Commands

- UE build (UE 5.8): `"C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" ProjectSandboxEditor Win64 Development "-Project=<repo>\ProjectSandbox\ProjectSandbox.uproject" -WaitMutex`
- UE automation tests: `Build/Scripts/Run-AutomationTests.ps1 -UEPath "C:\Program Files\Epic Games\UE_5.8" -ProjectFile <repo>\ProjectSandbox\ProjectSandbox.uproject -MinTestsKey plugin` (decide pass or fail from its report, not the editor's exit code). CI enforces a floor (`Build/automation-test-floors.json`); raise it in the PR that adds tests.
- Plugin package as CI builds it: `Build/Scripts/Build-Plugin.ps1 -UEPath "C:\Program Files\Epic Games\UE_5.8" -PluginUPluginPath <repo>\ProjectSandbox\Plugins\Open3DPuppet\Open3DPuppet.uplugin -OutDir <scratch>\Package -FailOnWarnings`.
- Before a local UE build in a worktree that was built before, delete `ProjectSandbox/Intermediate/Build/Win64/x64/ProjectSandboxEditor/Development/Makefile.bin` when files were added, removed, merged in or changed by a branch switch: UBT can otherwise reuse a stale makefile, and after a failed build the tests run the old binaries.
- Checks run by CI, all cheap: `python3 Build/Scripts/check-copyright-headers.py`, `python3 Build/Scripts/check-runtime-editor-deps.py`, `python3 Build/Scripts/check-markdown-links.py`.
- Verify before reporting a change as done: the UE build and the automation tests for plugin changes, and the checks above. Include their output. A plugin change that could not be built (for example from a Linux container with no engine) says so.

## Project decisions

- Unreal Engine 5.8 and later, Win64 only (ADR 0001). Target.cs files use `BuildSettingsVersion.V7` and `EngineIncludeOrderVersion.Unreal5_8` (rule OP-001). The source `.uplugin` carries no `EngineVersion`; packaging stamps it (rule OP-005).
- Copyright: new plugin source files under `Source/` (outside `ThirdParty/`, not generated) start with `// Copyright <year> Lifelike & Believable. All Rights Reserved.` (the current year) and a blank line (rule OP-002).
- Architecture (ADR 0002): a `UPuppetAsset` lists `UPuppetModule` instances; one AnimGraph node, `Puppet Solve`, evaluates the performers and runs the modules in order. A module is configuration (a `UObject`, shared by every instance of the puppet); its per-instance state lives in the solver it creates. Rule OP-006 and the authoring guide define the module contract; changing the contract needs an ADR.
- Performer input (ADR 0003): Live Link subjects with the Animation role, or performer clips (AnimSequences of human motion, ADR 0008), read through canonical joint names (the UE5 mannequin's bone names) and an optional `UPuppetPerformerProfile` that maps a source's own names to them. Retargeting works from calibrated rest poses; a performer that is not calibrated leaves the puppet in its reference pose.
- Several performers (ADR 0004): a puppet declares performer slots; the AnimGraph node's cast binds each slot to a Live Link subject or a performer clip. Exactly one slot owns the root.
- Threads: nothing blocks the game thread. Solving runs on the animation worker threads; game-thread state (calibrations) is copied in `PreUpdate`. Per-frame solver code does not allocate once bound.
- Hot paths stay quiet in logs: state changes and errors only.
- Tests (ADR 0005): the plugin's UE tests live in the editor-only `Source/Open3DPuppetTests/Private/<Area>/`, named `Open3DPuppet.<Area>.<Unit>.<Case>`, wrapped in `#if WITH_DEV_AUTOMATION_TESTS`, with white-box access through the owning module's `Public/Testing/*.h`. Solver tests build their skeletons in code (rule OP-003).
- No hard-coded credentials, ports or absolute paths in source control.
- Docs describe the intended behaviour. When code and docs disagree, fix the code unless the docs are demonstrably wrong; then fix the docs and say why in the commit.
- Rename or move files with `git mv`; don't delete and recreate them.

## External sources of truth

- Unreal APIs: the installed UE 5.8 engine source (`C:\Program Files\Epic Games\UE_5.8\Engine\Source`), or `lifelike-and-believable/UnrealEngine` on GitHub: branch `release` is UE 5.8 (5.8.4 on 2026-10-24), branch `5.7` is 5.7.4. A sparse, blob-filtered clone of only the paths you need is enough. Compiler output beats memory.
- Live Link: `Engine/Source/Runtime/LiveLinkInterface` (`ILiveLinkClient`, `Roles/LiveLinkAnimationTypes.h`) and, as the reference for reading a subject inside an AnimGraph node, `Engine/Source/Runtime/LiveLinkAnimationCore/Private/AnimNode_LiveLinkPose.cpp`.

## Git and pull requests

- Branch from `develop`; one work package (WP) per PR. Titles start with the WP id from `docs/roadmap/roadmap.md`, for example `WP-4: digitigrade leg module (ADR 0002)`. Update `CHANGELOG.md` and the affected docs in the same PR.
- Never rebase or force-push a pushed branch. To update one, merge `develop` into it.
- Merge pull requests into `develop` by squash, only when every check on the head commit has passed or been skipped.
- Line endings: `.gitattributes` stores every text file with LF; a checkout gets the platform's endings. `.sh` files are LF everywhere, and binaries are marked `binary`. A new binary type goes in `.gitattributes`.
- If git reports "dubious ownership", pass `safe.directory` for that one command (`GIT_CONFIG_COUNT=1 GIT_CONFIG_KEY_0=safe.directory GIT_CONFIG_VALUE_0=*`); never change the global git config.
- Never run two Unreal builds at once on one machine; wait for any running build (including other projects') to finish.
- Anything that needs a person at the desk (a capture stage, a live performer, a UE build on Windows when the change was made elsewhere) is reported as not verified and listed in the roadmap's "Next" section.
