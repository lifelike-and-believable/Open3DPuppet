# Changelog

The one changelog for the Open3DPuppet plugin. Each release gets a `## [x.y.z] - date` section.
Until then changes collect under `## Unreleased`.

## Unreleased

### Added

- **UE build and test CI** (WP-9a): `.github/workflows/ue-build-test.yml` builds the plugin with
  RunUAT BuildPlugin on UE 5.8 (warnings in plugin sources fail it) and runs the automation
  tests against the package, on the self-hosted runner Open3DBroadcast uses.
  `Build/Scripts/Build-Plugin.ps1` and `Run-AutomationTests.ps1` come from Open3DBroadcast.
- **Solver core and first modules** (WP-1, ADRs 0002 to 0004): `UPuppetAsset` (performer slots,
  root slot, ordered modules), the module contract (`UPuppetModule`, `FPuppetModuleSolver`,
  `FPuppetBindContext`, `FPuppetSolveContext`), the shared retargeting math (`PuppetRetarget`),
  `FPuppetInstance`, and the Root, Chain and Two-Bone Limb modules. The **Puppet Solve** AnimGraph
  node reads each performer slot from a Live Link Animation-role subject; `UPuppetPerformerProfile`
  maps other naming schemes to the canonical (mannequin) joint names; the Puppet Calibration
  subsystem captures and holds each performer's calibration. 34 automation tests. The guide
  `docs/guides/authoring-puppet-modules.md` explains how to add modules.
- **Foundation** (WP-0): coding-agent instructions (`AGENTS.md`, rules in `.claude/rules/`),
  CI checks for copyright headers, editor dependencies in runtime modules and Markdown links,
  the design overview, ADRs 0001 to 0005, the roadmap, the `ProjectSandbox` host project and an
  empty `Open3DPuppet` plugin with runtime, editor and test modules (UE 5.8, Win64).
