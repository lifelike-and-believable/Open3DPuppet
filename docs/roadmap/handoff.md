# Handoff

Where the last working session left Open3DPuppet, and how to start the next one. Last updated
2026-10-26. The [roadmap](roadmap.md) stays the source of truth for work packages; this page says
what changed, what is open, and what to do first. Work that spans Open3DBroadcast, VMCLiveLink
and Open3DPuppet together is planned in
[`live-virtual-performance`](https://github.com/lifelike-and-believable/live-virtual-performance)
(AGENTS.md).

## Start a session

Read [`AGENTS.md`](../../AGENTS.md), then this page, then the [roadmap](roadmap.md). The design
is in [`docs/design/overview.md`](../design/overview.md) and the decisions in
[`docs/adr/`](../adr/README.md), including the maintainer's answers to open questions.

## Where things stand (2026-10-26)

- `develop` at `da4d7cc`.
- Done: WP-0, WP-1, WP-9a, WP-11 (curves, #8), WP-12 (Digits #13, finger-curl curves #14, ADR
  0009), WP-13 (face modules, #15), WP-4 (digitigrade leg, #16), WP-14 (solver dynamics, #18),
  WP-5 (dynamic chain, #19), WP-6 (wing, #20), WP-16 (hand-offs, #9; Two-Bone Limb weight fix #17)
  and WP-3 (performer clips as a cast source, #21).
- 132 automation tests, all passing on the self-hosted runner.
- Every module the Wolf and the Eagle need exists, except the Tier 2 physics hand-off (WP-15).

## Next steps

In this order unless the maintainer says otherwise:

1. **WP-2, calibration and profiles in the editor** (calibrate button, saving calibrations to
   assets, choice of reference pose, profile auto-mapping by name), the next work package in the
   roadmap's order that is not blocked:
   - WP-15 waits on physics assets for the Wolf and the Eagle;
   - WP-17 waits on which hand-off triggers the show uses (ADR 0008 question 1);
   - WP-M1 and WP-M2 (the Wolf and the Eagle) wait on their meshes.
2. **At the desk** (not verifiable without a person and a capture setup): WP-1, WP-3, WP-4, WP-5,
   WP-6, WP-11, WP-12, WP-13 and WP-16. The roadmap's [Next](roadmap.md#next) lists what to try
   and tune for each.

## Waiting on the maintainer

The roadmap's [Next](roadmap.md#next) section lists them. In short:

- Which hand-off triggers the show uses (ADR 0008 question 1, WP-17).
- The Wolf and Eagle meshes, their morph targets, and how each is performed.
- Physics assets for the Wolf and the Eagle, before WP-15.

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
- **Local runs:**
  - An automation run creates an untracked `ProjectSandbox/Config/` (`DefaultEngine.ini`,
    `DefaultInput.ini`). Delete it; do not commit it.
  - `Build-Plugin.ps1` needs a short `-OutDir` (for example `<repo>\Artifacts\Package`, which is
    ignored): a deep one, such as a temporary scratch folder, pushes intermediate paths past 260
    characters and the build fails with `OtherCompilationError`.
