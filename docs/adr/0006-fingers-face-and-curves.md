# 0006: Fingers, face and curves

- **Status:** Accepted (maintainer sign-off 2026-10-24)
- **Date:** 2026-10-24
- **Related:** [ADR 0002](0002-puppet-module-architecture.md) (amends the module contract), [ADR 0003](0003-performer-input-and-calibration.md), [ADR 0004](0004-multiple-performers.md), [design overview](../design/overview.md) §5 and §10

## Context

The maintainer asked (2026-10-24) for the toolkit to support and make use of finger and facial
tracking, with the Wolf and Eagle of *Mamaawi Together Through the Fire* as the first puppets.
Neither creature has human hands or a human face, so the point is not to copy fingers and
expressions one to one. It is to give puppeteers more channels of control: finger curl and spread
can drive paws, talons and wing feathers, and facial expressions can drive jaws, beaks, ears,
eyelids and crests.

What the input looks like:

- **Fingers** arrive as bones. A suit or glove system either adds finger bones to the body subject
  or publishes the hands as a separate Live Link subject. Either way they are Animation-role data
  that the performer pose already reads.
- **Faces** arrive as curves (named float values), on a subject of their own. Every Live Link role
  carries curves: static data names them (`FLiveLinkBaseStaticData::PropertyNames`,
  `Engine/Source/Runtime/LiveLinkInterface/Public/LiveLinkTypes.h:261`) and each frame carries
  their values (`FLiveLinkBaseFrameData::PropertyValues`, `LiveLinkTypes.h:230`). The stock Live
  Link Pose node reads curve-only subjects through the Basic role
  (`Engine/Source/Runtime/LiveLinkAnimationCore/Private/AnimNode_LiveLinkPose.cpp:131`). UE 5.7.4
  source; needs-verification on 5.8.

What WP-1 cannot do: `FPuppetPerformerPose` has bones only. The node reads only Animation-role
subjects. The puppet pose has bones only, so a module cannot drive a morph target or a material
parameter. And chain deltas are measured in component space, which is wrong for fingers: a paw
whose rest orientation differs from the performer's hand needs the finger's motion relative to the
hand, not relative to the stage.

## Decision drivers

1. Fingers and faces from any Live Link source, the same way bodies work.
2. A creature's anatomy decides how the channels are used; the toolkit does not assume a human
   face or hand on the puppet.
3. The contract stays small, and existing modules keep working.

## Options considered

### Option A: a separate face pipeline (Live Link Pose node plus a remap asset), outside the puppet
- Pros: no contract change.
- Cons: face and body would be tuned in two places, and faces could not drive bones that body
  modules also write (a wolf's ears, an eagle's crest).

### Option B: curves as first-class performer and puppet data (chosen)

## Decision

1. **Performer curves.** `FPuppetPerformerPose` also carries curves: names from static data, values
   per frame, looked up by name (`FindCurve`). A profile may map curve names as it maps joints, for
   sources whose names differ.
2. **Curve-only subjects.** A cast entry may bind a subject with the Basic role. Its performer pose
   has curves and no bones. The node tries the Animation role first, then the Basic role, as the
   stock Live Link Pose node does.
3. **Separate subjects are separate slots.** A performer whose hands or face come from another
   system gets a slot per source, for example `Body`, `Hands` and `Face` (ADR 0004). Modules
   already name their slot, so no merging of subjects is needed.
4. **Calibration covers curves.** A calibration also stores each curve's value at rest (the
   performer's neutral face), and curve modules work from the change since neutral. A curve-only
   slot is calibrated like any other: the performer holds a neutral face and the subsystem
   captures it.
5. **Puppet curves.** The puppet pose also carries output curves, which modules write by name
   (`SetCurve`); the node writes them into the output pose's curves, where they drive morph
   targets and material parameters. Curves no module writes pass through from the input pose.
6. **Parent-relative deltas.** A helper, `FPuppetSolveContext::GetJointLocalDelta(joint)`, gives a
   joint's rotation change relative to its parent since calibration. Finger modules use it, so a
   finger curls the same way whatever the hand is doing and whatever the paw's rest orientation
   is.
7. **Modules this enables**, each its own work package (roadmap):
   - **Digits** (fingers, toes, talons, feathers): maps performer fingers onto any number of
     puppet digits with any number of joints, with curl and spread gains, using parent-relative
     deltas and chain resampling.
   - **Curve to bone**: a performer curve (`jawOpen`, `browInnerUp`, a finger curl) rotates or
     moves a puppet bone through a range, with remap, dead zone and smoothing. Beak, jaw, ears,
     eyelids, crest.
   - **Curve remap**: performer curves to puppet curves (morph targets), with per-curve remap,
     combination and smoothing.
   - **Eyes**: eye-look curves or eye bones to puppet eyes, with range limits per creature.
   - **Finger curl** as a derived performer curve (`curl_index_l` and so on), computed from finger
     bones, so any curve-driven module can use hand gestures as controls.
8. **Contract change.** Items 1, 4, 5 and 6 change the types ADR 0002 lists as the contract, so
   this ADR records them. Existing modules need no change.

## Consequences

- One puppet asset holds body, hands and face, and one calibration call per slot sets them up.
- Faces need a neutral-face calibration; without it the face slot is "not calibrated" and its
  modules leave the input.
- The node evaluates one more role per slot when the Animation role is missing.

## Open questions for the maintainer

1. Which systems capture fingers and faces for the show (for example Live Link Face with ARKit
   curves, MetaHuman Animator, a glove system, a suit with finger bones)? Their curve and bone names
   decide the default profiles.
2. Do the Wolf and Eagle meshes use morph targets, face bones, or both?
3. needs-verification: the Live Link Face app's subjects use the Basic role, and the ARKit curve
   names reach Live Link unchanged, on 5.8.
