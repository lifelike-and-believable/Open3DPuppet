# 0003: Performer input and calibration

- **Status:** Proposed (pending maintainer sign-off)
- **Date:** 2026-10-24
- **Related:** [ADR 0002](0002-puppet-module-architecture.md), [design overview](../design/overview.md) §3, §4 and §7

## Context

The maintainer chose Live Link as the only input (2026-10-24), so any capture system with a Live
Link source works, Open3DBroadcast's receiver included. A Live Link Animation-role subject provides
bone names and parent indices as static data
(`Engine/Source/Runtime/LiveLinkInterface/Public/Roles/LiveLinkAnimationTypes.h:38`) and local
transforms per frame (`LiveLinkAnimationTypes.h:57`). The stock Live Link Pose node reads a subject
by fetching the `ILiveLinkClient` modular feature on the game thread in `PreUpdate`
(`Engine/Source/Runtime/LiveLinkAnimationCore/Private/AnimNode_LiveLinkPose.cpp:65-71`) and calling
`EvaluateFrame_AnyThread` with `ULiveLinkAnimationRole` during evaluation (`:124`). (Read in UE
5.7.4; these APIs are unchanged in 5.8.4, which also adds a Character role,
`Roles/LiveLinkCharacterRole.h`, that extends the Animation role, so its subjects are read the
same way.)

Capture systems name joints differently, and their rest poses differ from the puppet's.

## Decision drivers

1. A puppet works with any capture system without editing the puppet.
2. Retargeting does not depend on how either skeleton orients its bone axes.
3. Setting up a new performer takes seconds on a stage.

## Decision

1. **Reading.** The Puppet Solve node reads each slot's subject the way `FAnimNode_LiveLinkPose`
   does: the client is fetched in `PreUpdate`, the frame is evaluated on the worker thread.
2. **Canonical joints.** Modules name performer joints by the UE5 mannequin's bone names
   (`pelvis`, `spine_01` to `spine_05`, `neck_01`, `neck_02`, `head`, `clavicle_l`,
   `upperarm_l`, `lowerarm_l`, `hand_l`, `thigh_l`, `calf_l`, `foot_l`, `ball_l` and the `_r`
   counterparts). A source that streams mannequin names, such as Open3DBroadcast from a
   mannequin-based character, needs nothing else.
3. **Performer profile.** `UPuppetPerformerProfile : UDataAsset` maps canonical joint names to a
   source's bone names, for other naming schemes. A canonical name the profile does not map is
   looked up as is. The cast entry for a slot may name a profile.
4. **Calibration.** `FPuppetPerformerCalibration` holds the performer's component-space rest pose
   (captured while the performer stands in the reference pose, T-pose by default) and an
   alignment rotation that turns the performer's facing into the puppet's. Retargeting uses
   rotation deltas from that rest pose (design overview §4).
5. **Where calibrations live.** `UPuppetCalibrationSubsystem` (`UEngineSubsystem`, so it works in
   the editor viewport and in game) keeps the current calibration per Live Link subject name.
   `CalibratePerformer(SubjectName, Alignment)` (BlueprintCallable) captures the subject's current
   frame on the game thread. The node copies the calibrations it needs in `PreUpdate`. A
   calibration is a `BlueprintType` struct, so a project can save and restore it; saving it in an
   asset from the editor is WP-2.
6. **Not calibrated.** A slot whose subject has no calibration leaves its modules' bones as the
   node's input pose, and the node logs once that the subject needs calibrating.

## Consequences

- Performer setup on a stage: stand in T-pose, calibrate, go.
- A performer whose rest pose is not a T-pose (A-pose rigs) calibrates in their own rest pose, and
  the puppet's reference pose must match it, or the module offsets absorb the difference. WP-2
  adds a choice of reference pose.

## Open questions for the maintainer

1. Are the UE5 mannequin's names the right canonical set, or should it be a neutral list?
2. needs-verification: Take Recorder records Live Link subjects and Sequencer plays the recording
   back into Live Link on 5.8, so recorded takes drive the puppet like live ones (design overview
   §7). To be confirmed at the desk in WP-3.
