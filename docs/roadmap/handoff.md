# Handoff

Where the last working session left Open3DPuppet, and how to start the next one. Last updated
2026-10-27. The [roadmap](roadmap.md) stays the source of truth for work packages; this page says
what changed, what is open, and what to do first. Work that spans Open3DBroadcast, VMCLiveLink
and Open3DPuppet together is planned in
[`live-virtual-performance`](https://github.com/lifelike-and-believable/live-virtual-performance)
(AGENTS.md).

## Start a session

Read [`AGENTS.md`](../../AGENTS.md), then this page, then the [roadmap](roadmap.md). The design
is in [`docs/design/overview.md`](../design/overview.md) and the decisions in
[`docs/adr/`](../adr/README.md), including the maintainer's answers to open questions.

## Where things stand (2026-10-27)

- `develop` after WP-15 (#36).
- Done: WP-0, WP-1, WP-9a, WP-11 (curves, #8), WP-12 (Digits #13, finger-curl curves #14, ADR
  0009), WP-13 (face modules, #15), WP-4 (digitigrade leg, #16), WP-14 (solver dynamics, #18),
  WP-5 (dynamic chain, #19), WP-6 (wing, #20), WP-16 (hand-offs, #9; Two-Bone Limb weight fix #17)
  WP-3 (performer clips as a cast source, #21), CI status comments (#23) and WP-2 (saved
  calibrations #24, naming schemes and profile auto-mapping #25, puppet rest pose #26), the desk
  test plan (#27), WP-7 (Prop Chain and the multi-performer sample, #28), WP-8 (bone pickers
  #29, setup assistant #30, live tuning and slot status #31, the Puppet Session panel #33) and
  the asset types in the Add menu (#32), and WP-9 (the Fab source zip, its checks and the
  plugin icon, #34), WP-10 (the Control Rig adapter, #35; ADR 0010) and WP-15 (the engine
  physics hand-off, #36; ADR 0011).
- 196 automation tests, all passing on the self-hosted runner.
- Every module the Wolf and the Eagle need exists. Their meshes and physics assets are in the
  host project (not committed: licensed assets); the show uses the Mamaawi copies.

## Next steps

In this order unless the maintainer says otherwise:

1. **Desk tests first** (maintainer, 2026-10-27): the maintainer runs the
   [desk test plan](../guides/desk-test-plan.md), recorded mannequin motion first (parts 9 to
   18), then live with a Rokoko suit (part 19). Fix what the `desk` issues report before new
   features. The Wolf and the Eagle are test meshes, not production puppets yet; WP-M1 and WP-M2
   come after. WP-17 still waits on which hand-off triggers the show uses (ADR 0008 question 1).
2. **At the desk** (not verifiable without a person and a capture setup): WP-1, WP-2, WP-3, WP-4, WP-5, WP-7, WP-8,
   WP-6, WP-10, WP-11, WP-12, WP-13, WP-15 and WP-16. The roadmap's [Next](roadmap.md#next) lists what to try
   and tune for each; the [desk test plan](../guides/desk-test-plan.md) has the steps for WP-2,
   WP-3 and WP-7.

## Waiting on the maintainer

The roadmap's [Next](roadmap.md#next) section lists them. In short:

- Which hand-off triggers the show uses (ADR 0008 question 1, WP-17).
- How the Wolf and the Eagle are performed, and their entrance clips (WP-M1, WP-M2).
- Which regions are physical in the show (ADR 0007 question 1).

## Working conventions learned

These are not obvious from the rest of the repository's docs:

- **Merging:** the maintainer said "merge when green" (2026-10-24, again 2026-10-26).
  Squash-merge a PR once every check on its head commit has passed or been skipped for a reason
  other than the PR being a draft. `develop` is not protected, so GitHub does not refuse a red
  merge. Pass the full 40-character head SHA to `--match-head-commit`.
- **CI and drafts:**
  - Draft PRs skip the UE build job. Open the PR as a draft, then mark it ready, which starts the
    UE build and tests (a few minutes on the self-hosted runner).
  - A push while the PR is a draft, made just before marking it ready, can cancel the
    ready-for-review run (one concurrency group per branch), leaving only a skipped UE build on
    the head. If the head's UE build shows skipped, convert the PR to a draft and mark it ready
    again.
  - CI posts a comment on the PR each time it finishes (not on drafts): "Open3DPuppet CI
    completed successfully!" with the head SHA when every job passed. In the Claude desktop app
    that comment wakes a session waiting to merge; still confirm the checks on the head commit.
- **Test-driven work** goes as two pushed commits: a red one (API and tests, implementation
  stubbed, with the expected failures) and a green one. Guard a red test so it fails rather than
  crashes the editor (check array sizes before indexing a stub's empty output). Raise
  `Build/automation-test-floors.json` in the PR that adds tests.
- **Fixtures:** `PuppetTests::MakeSkeleton` and `PuppetTests::MakeClip`
  (`Source/Open3DPuppetTests/Private/Harness/PuppetTestFixtures.h`) build a `USkeleton` and a
  `UAnimSequence` in code, for tests that need assets.
- **Control Rig tests:** `UPuppetTestControlRig`
  (`Source/Open3DPuppetTests/Private/Harness/PuppetTestControlRig.h`) is a native rig built in
  code, as the engine's FK Control Rig is, so the adapter is tested without a rig asset.
- **Licensed content:** the Wolf, Eagle and Bison meshes in `ProjectSandbox/Content/` are
  commercial assets and the repository is public. They stay untracked, and tests never load
  `/Game/...` (CI does not have them); tests build the skeletons they need in code.
- **Reading assets without the editor UI:** a Python script run by
  `UnrealEditor-Cmd.exe <uproject> -run=pythonscript -script=<file> -EnablePlugins=PythonScriptPlugin -unattended -nop4 -nosplash -NoSound -stdout`
  can list a mesh's bones (`unreal.AnimPoseExtensions` on the skeleton's reference pose), morph
  targets and a physics asset's constraints (`get_constraints`, then
  `ConstraintInstanceBlueprintLibrary.get_attached_body_names`), saving nothing.
- **Local runs:**
  - An automation run creates an untracked `ProjectSandbox/Config/` (`DefaultEngine.ini`,
    `DefaultInput.ini`). Delete it; do not commit it.
  - `Build-Plugin.ps1` needs a short `-OutDir` (for example `<repo>\Artifacts\Package`, which is
    ignored): a deep one, such as a temporary scratch folder, pushes intermediate paths past 260
    characters and the build fails with `OtherCompilationError`.
