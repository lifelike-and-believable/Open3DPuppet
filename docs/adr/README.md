# Architecture Decision Records

This folder holds the Architecture Decision Records (ADRs) for Open3DPuppet. Each ADR records one
decision: the problem, the options that were considered, what was chosen and why, and what follows
from it. ADRs need maintainer sign-off before coding work that depends on them is merged.

## Index

| ADR | Title | Status |
|---|---|---|
| [0001](0001-platform-and-engine-scope.md) | Platform and engine scope | Accepted |
| [0002](0002-puppet-module-architecture.md) | Puppet module architecture | Accepted |
| [0003](0003-performer-input-and-calibration.md) | Performer input and calibration | Accepted |
| [0004](0004-multiple-performers.md) | Several performers on one puppet | Accepted |
| [0005](0005-test-layout.md) | Test layout | Accepted |
| [0006](0006-fingers-face-and-curves.md) | Fingers, face and curves | Accepted |
| [0007](0007-physics-driven-modules.md) | Physics-driven modules | Accepted |
| [0008](0008-animation-sources-and-handoffs.md) | Animation sources and hand-offs | Accepted |
| [0009](0009-derived-finger-curl-curves.md) | Derived finger-curl curves | Accepted |
| [0010](0010-control-rig-adapter.md) | Control Rig adapter | Proposed |

## Answers to open questions

An accepted ADR is not edited (see Conventions), so the maintainer's answers to the open
questions of accepted ADRs are recorded here.

| ADR | Question | Answer (maintainer, 2026-10-24) |
|---|---|---|
| 0001 | Licence and `LICENSE` file; Fab | As Open3DBroadcast: MIT licence, and the plugin is prepared for Fab under the same rules (packaging in WP-9, following Open3DBroadcast's `fab-package.py`). |
| 0003 | Canonical joint names | The Unreal standard skeleton's (UE5 mannequin) names, fingers included. |
| 0006 | Finger capture | Standard Live Link: finger bones on an Animation-role subject, named as the Unreal standard skeleton. |
| 0006 | Face capture | Configurable. The default is the most common source: ARKit's 52 blend-shape curves (`jawOpen`, `eyeBlinkLeft`, `browInnerUp`, ...) as Live Link Face sends them. Other sources map their curve names through the performer profile. |
| 0007 | Physics assets for the Wolf and Eagle | None yet. Tier 1 (solver dynamics) needs none; Tier 2 (WP-15) needs them authored first. |
| 0008 | Manual re-centre | Yes: `UPuppetPerformanceComponent` gets a re-centre call that captures the same offset as a hand-off, mid-scene, which then converges on the performer as a hand-off's does (WP-16). |
| 0008 | Question 3 (needs-verification): evaluating a sequence for its own skeleton on the worker thread | Verified in WP-3, not a maintainer answer: a bone container built once for the sequence's skeleton (`FBoneContainer::InitializeTo` with the `USkeleton` as the asset), with retargeting disabled so bones in Skeleton translation mode keep the take's translations, then `GetAnimationPose` into an `FCompactPose` on the animation stack each frame, as PoseSearch samples an asset (UE 5.8.4, `Engine/Plugins/Animation/PoseSearch/Source/Runtime/Private/PoseSearchAssetSamplerLibrary.cpp:33`). Per-bone `GetBoneTransform` was not used: it builds a decompression context per call (`Engine/Source/Runtime/Engine/Private/Animation/AnimSequence.cpp:1515`). |
| 0008 | Item 5: a transition back to the animation "drops the offset" | The offset is kept until the next hand-off or re-centre replaces it, so a slot still under its performer does not snap when another slot hands back (WP-16). |

## Conventions

- **File name:** `NNNN-short-slug.md`, numbered in the order they are written. Numbers are never reused.
- **Status values:** `Proposed` (written, awaiting sign-off), `Accepted` (maintainer signed off), `Superseded by NNNN`, `Rejected`. When an ADR is accepted, update its Status line and the index above.
- **Evidence:** factual claims about this repository or the engine cite `path:line`. Engine paths are relative to the engine root (`Engine/...`) and name the engine version they were read from.
- **Unverified claims:** anything that depends on Unreal Engine behaviour that could not be confirmed from source is marked **needs-verification** and repeated under *Open questions*.
- An ADR is not edited after acceptance except to change its Status. A changed decision gets a new ADR that supersedes the old one.

## Template

```markdown
# NNNN: <Title>

- **Status:** Proposed (pending maintainer sign-off)
- **Date:** YYYY-MM-DD
- **Related:** <other ADRs, work packages>

## Context
What is true today, with evidence. What forces a decision now.

## Decision drivers
The criteria the options are judged against, most important first.

## Options considered
### Option A: <name>
- Pros:
- Cons:

## Decision
One clear recommendation, stated so a coding agent can act on it.

## Consequences
What gets easier, what gets harder, which work packages this enables or constrains.

## Open questions for the maintainer
Numbered, including every needs-verification item.
```
