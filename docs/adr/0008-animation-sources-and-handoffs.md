# 0008: Animation sources and hand-offs

- **Status:** Proposed (pending maintainer sign-off)
- **Date:** 2026-10-24
- **Related:** [ADR 0002](0002-puppet-module-architecture.md) and [ADR 0004](0004-multiple-performers.md) (amends the cast and the solve context), [ADR 0003](0003-performer-input-and-calibration.md), [design overview](../design/overview.md) §7 and §12

## Context

In *Mamaawi Together Through the Fire* the Wolf and the Eagle enter through an AnimSequence (the
Wolf runs in, the Eagle flies in) and then pass to motion-capture control (maintainer,
2026-10-24). The maintainer also wants a recorded take to be a simple choice of source rather
than a trip through Take Recorder's Live Link playback (ADR 0003 open question 2).

Two kinds of clip are in play, and both are to be supported (maintainer, 2026-10-24):

- **Creature clips**, authored on the puppet's own skeleton (a Wolf run cycle made by an
  animator). They need no retargeting.
- **Performer clips**, human motion (a mocap take, a recording) on a performer skeleton. They are
  retargeted through the puppet's modules, exactly like live motion.

What exists after WP-1: the Puppet Solve node has an input pose (`Source`), and every module's
`Weight` blends its result over it, so a creature clip playing into `Source` already shows through
where module weights are 0. A cast entry can only name a Live Link subject. Nothing moves a
weight over time, and nothing keeps the puppet where a clip left it when the performer takes over:
the Root module places the pelvis from the performer's stage position, so control would snap the
creature to wherever the performer stands.

UE 5.8.4 evaluates a sequence's pose at a time through `UAnimSequence::GetAnimationPose`
(`Engine/Source/Runtime/Engine/Classes/Animation/AnimSequence.h:427`) or bone by bone through
`GetBoneTransform` (`:508`), both with an `FAnimExtractContext` that carries the time.

## Decision drivers

1. Entrances and exits are reliable in a live show: no snap, no foot slide, repeatable.
2. A recorded take is one field away from a live performer.
3. A hand-off can be triggered in more than one way; which ones the show uses is still open, so
   no trigger is built into the core.
4. Per slot, so the body can take over before the tail, or one performer before another.

## Decision

1. **Performer clips as a cast source.** A cast entry names its source: a Live Link subject (as
   now) or an AnimSequence with playback settings (play rate, loop, start time). The node evaluates
   the sequence on its own skeleton on the worker thread and feeds the slot the same data a Live
   Link subject gives (bone names, parents, local transforms), so calibration, profiles and modules
   work unchanged. Playback time comes from the node's update by default, or from a time pin, so
   Sequencer can scrub it. Calibration for a clip is taken from a chosen frame (the first by
   default), or from the sequence's reference pose.
2. **Creature clips through `Source`.** No new source type: a creature clip plays into the node's
   input pose (a Sequence Player, a Slot for montages, or a Sequencer animation track). Item 3
   decides how much of it shows.
3. **Control blend per slot.** Each performer slot has a control blend from 0 (the input pose,
   so the creature clip) to 1 (the performer drives the slot's modules). It multiplies every
   module weight of the slot. A transition moves it over a duration with an easing curve, in either
   direction.
4. **One entry point for transitions.** `UPuppetPerformanceComponent`, on the puppet's actor, owns
   the transitions: `TransitionToPerformer(Slot, Duration, Easing)`,
   `TransitionToAnimation(Slot, Duration, Easing)`, `SetControlBlend(Slot, Value)` and
   `GetControlBlend(Slot)`, all BlueprintCallable, with an event when a transition ends. The node
   copies the blends in `PreUpdate`, as it copies calibrations (ADR 0003). Triggers are thin
   adapters onto this entry point, each its own work package:
   - **Sequencer cue:** an event track calling the component.
   - **Operator:** an input action, or Remote Control for a hardware panel.
   - **Performer gesture:** a trigger that watches the performer (a held pose, a position mark, a
     curve over a threshold) and calls the component.
5. **Anchoring the root at a hand-off.** When a slot that owns the root (ADR 0004) starts a
   transition to the performer, the node captures an anchor: the puppet's current root and pelvis
   transform, taken from the pose it is about to blend from, against the performer's current pelvis
   on the stage. From then on the Root module applies the performer's movement relative to the
   anchor, so the creature continues from where the clip left it, facing the way it faced. The
   anchor's facing is yaw only, by default, so the performer's lean and crouch still come through.
   A transition back to the animation releases the anchor. The solve context gains the anchor,
   which is a contract change, recorded here.
6. **Continuity during the blend.** Modules blend from the clip's pose to the retargeted pose as
   the control blend rises; limbs in Pelvis Relative mode keep the feet from sliding while the root
   is anchored.

## Consequences

- An entrance is: a creature clip (or a performer clip) plays, a trigger calls
  `TransitionToPerformer`, the creature carries on under the performer's control from where it
  is. An exit is the same in reverse.
- Recorded takes no longer depend on Take Recorder's Live Link playback; ADR 0003's open question
  2 matters only for takes recorded that way.
- Every module gets the control blend for free through its weight; module authors write nothing
  for it.

## Open questions for the maintainer

1. Which triggers the show uses: a Sequencer cue, an operator, a performer gesture, or several
   (maintainer, 2026-10-24: still open; all three are designed for).
2. Should a slot's anchor also be settable by hand (re-centring a performer mid-scene without a
   transition)?
3. needs-verification: evaluating a sequence on the animation worker thread for a skeleton other
   than the mesh's, through `GetAnimationPose` with a bone container built for the sequence's
   skeleton, or `GetBoneTransform` per bone (to be settled in WP-3).
