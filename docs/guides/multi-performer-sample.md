# Multi-performer sample: a body and a puppeteered tail

A creature played by two people (ADR 0004, design overview §6): a performer in a capture suit
plays the body, and a puppeteer plays the tail with a tracked rod. The automation tests
`Open3DPuppet.MultiPerformer.BodyAndTail.*`
(`Source/Open3DPuppetTests/Private/MultiPerformer/PuppetMultiPerformerTests.cpp`) build this same
setup in code and check what each step below says to expect; the plugin ships no meshes, so the
setup is described here rather than shipped as assets.

## What you need

- A skeletal mesh with a tail (here `tail_01` to `tail_03`, under the pelvis).
- A Live Link subject for the body: an Animation-role subject with mannequin bone names, or a
  performer profile for its names ([Desk test plan](desk-test-plan.md) part 3).
- A Live Link subject for the rod: a Transform-role subject, one tracked rigid body. Open3DPuppet
  reads it as a performer with one joint, `prop`. Any capture system's rigid body works; so does
  any other performer joint (a hand), named in the module.

## 1. The puppet asset

1. Create a Puppet Asset for the mesh.
2. **Performer Slots:** `Body` and `Tail`. **Root Slot:** `Body`, so only the body performer
   places the creature in the world.
3. Modules on `Body` (the default slot): **Root**, a **Chain** for the spine, **Two-Bone Limb**s
   for the legs and arms, as for any biped.
4. A **Prop Chain** module with **Performer Slot** `Tail`:
   - **Performer Joint:** `prop` (the rod).
   - **Puppet Start** `tail_01`, **Puppet End** `tail_03`.
   - **Shape:** `Bend` (the tail bends from its base, its tip turning fully with the rod) or
     `Rigid` (the whole tail turns as one piece).
   - **Space:** `Parent` (the rod's motion adds to the body's, so the tail moves with the body
     and the puppeteer only adds the swing) or `Stage` (the tip points where the rod points on
     the stage, whatever the body does).
   - **Amplification** to exaggerate a small rod movement; **Smoothing** (seconds) to calm a
     noisy track.

## 2. The cast

In the Animation Blueprint's **Puppet Solve** node, add two cast entries:

| Slot | Source | Subject |
|---|---|---|
| `Body` | Live Link | the body performer's subject |
| `Tail` | Live Link | the rod's subject |

## 3. Calibrate both

Each performer is calibrated on their own (ADR 0004):

1. The body performer stands in T-pose: **Capture** on a Puppet Calibration Asset for the body's
   subject.
2. The puppeteer holds the rod in its rest position, the pose where the tail should be at rest:
   **Capture** on a second Puppet Calibration Asset for the rod's subject.

Name each asset as its cast entry's **Saved Calibration** to keep them across restarts.

## 4. What to expect

These are what the sample's automation tests check:

- **The body moves the puppet; the tail goes with it.** When the body performer walks, the
  creature walks; with the rod still, the tail keeps its rest relative to the body
  (`TheBodyMovesThePuppetAndTheTailFollows`).
- **The rod bends only the tail.** Swinging the rod swings the tail; the spine stays the body
  performer's, and the rod never moves the creature (`TheRodBendsOnlyTheTail`).
- **An uncalibrated rod leaves the tail alone.** Until the rod is calibrated, the tail stays in
  the node's input pose while the body drives (`AnUncalibratedRodLeavesTheTail`).

The tail can also be handed between an animation and the puppeteer like any slot (ADR 0008):
the performance component's transitions take the slot `Tail`.
