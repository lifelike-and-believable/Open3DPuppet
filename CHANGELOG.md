# Changelog

The one changelog for the Open3DPuppet plugin. Each release gets a `## [x.y.z] - date` section.
Until then changes collect under `## Unreleased`.

## Unreleased

### Planned

- **Fingers, face and the first characters** (ADR 0006): curves in the pipeline, Digits, face and
  eye modules, and the Wolf and Eagle of *Mamaawi Together Through the Fire* as the first puppets
  (design overview §10, roadmap WP-11 to WP-13, WP-M1, WP-M2).
- **Physics-driven modules** (ADR 0007): solver dynamics in the core plugin and an optional
  hand-off to the engine's Rigid Body With Control node (design overview §11, roadmap WP-14,
  WP-15).
- **Animation sources and hand-offs** (ADR 0008): AnimSequences of human motion as a cast source,
  creature clips through the input pose, per-slot control blends with timed transitions, and root
  anchoring that converges on the performer's position, for entrances such as the Wolf's run-in and the Eagle's fly-in (design overview §12,
  roadmap WP-3, WP-16, WP-17).

### Added

- **Hand-offs between an animation and the performer** (WP-16, ADR 0008): each performer slot
  has a control blend (0 the Puppet Solve node's input pose, 1 the performer) that scales every
  module weight of the slot; modules blend with the new `FPuppetSolveContext::Weight`.
  `UPuppetPerformanceComponent` on the puppet's actor runs timed, eased transitions per slot
  (`TransitionToPerformer`, `TransitionToAnimation`, `SetControlBlend`, `Recentre`, an event when
  a transition ends; slots can start on the animation). A hand-off to the performer or a manual
  re-centre captures a hand-off offset (a yaw turn and a horizontal move, in the solve context as
  `HandoffOffset`) so the puppet continues from where the animation left it; the Root module
  converges it on the performer over `ConvergenceTime`, by default only while the performer moves,
  or holds it. The context helpers apply the offset, so every module turns with the puppet.
  11 new tests (53).
- **Curves in the pipeline** (WP-11, ADR 0006): performers carry curves from Live Link (curve
  names from static data, values per frame), looked up by canonical name with an optional
  mapping in the performer profile (`CurveCanonicalToSource`). Curve-only (Basic-role) subjects,
  such as a face, can be cast into a slot. Calibration captures each curve's neutral value.
  Modules read a curve's change from neutral (`FPuppetSolveContext::GetCurveDelta`) and write
  output curves (`FPuppetPose::SetCurve`), which the Puppet Solve node writes to the pose's
  curves. `GetJointDeltaInParent` gives a joint's rotation relative to its parent, for fingers.
  8 new tests (42).
- **Licence:** MIT, as Open3DBroadcast (maintainer, 2026-10-24), at the repository root and the
  plugin root; `Config/FilterPlugin.ini` puts it in the plugin package.
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
