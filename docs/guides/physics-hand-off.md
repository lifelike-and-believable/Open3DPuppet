# Engine physics hand-off (Tier 2)

How to put the engine's physics on a puppet after Puppet Solve, and play how physical each region
is while performing ([ADR 0007](../adr/0007-physics-driven-modules.md), Tier 2). For secondary
motion that needs no physics asset (a tail that swings and settles), use the Dynamic Chain module
instead (Tier 1).

The pieces, all in the plugin's `Open3DPuppetPhysics` module
([ADR 0011](../adr/0011-physics-module-in-the-plugin.md)):

- The **Physics Region** puppet module writes no bones. For one region it publishes two puppet
  curves: `Physics_<Region>`, the region's physics weight (0 keeps its physics controls as
  authored, 1 takes their strength to 0, limp), and `Physics_<Region>_Damping`, extra damping (0
  as authored, 1 doubles the damping ratio). Each is a fixed value or a performer curve's change
  from neutral, times the module's weight.
- **Make Physics Region Parameters** (`UPuppetPhysicsLibrary::MakeRegionParameters`) reads those
  curves from the animation instance and returns the control multipliers for Rigid Body With
  Control. A region without curves keeps its authored values, so a module at weight 0, a
  performer at neutral or a blend that drops the curves leaves the physics as authored.

## 1. The physics asset

The mesh needs a physics asset whose bodies cover the regions that should be physical. Bodies
are authored in the Physics Asset editor, outside this toolkit. A region with no bodies has
nothing to simulate; the puppet's other bones follow Puppet Solve.

## 2. The AnimGraph

```
Live Link / clips ──► Puppet Solve ──► Rigid Body With Control ──► Output Pose
                                         ▲ Control And Modifier Parameters
                     Make Physics Region Parameters (Self, Regions)
```

1. Add **Rigid Body With Control** after **Puppet Solve**.
2. On the node, set **Override Physics Asset** if the mesh's own physics asset is not the one to
   use, and under **Character Setup Data** list one limb per region:
   - **Limb Name** is the region's name (`Tail`, `LeftFrontLeg`, `Body`). The node puts the limb's
     controls and body modifiers in a set of that name, which is what the region's multipliers
     address.
   - **Start Bone** is the region's root-most bone. The limb takes the bodies at and below it.
   - Each body belongs to the first limb that reaches it, so list the extremities (tail, legs,
     head) before the limb that starts at the pelvis.
   - Set the limbs' default strengths and damping (**Default Parent Space Control Data**, **Default
     World Space Control Data**) to how the region should hold the solved pose when the
     puppeteer asks for nothing. The regions scale these.
3. Connect **Make Physics Region Parameters** to the node's **Control And Modifier Parameters**
   pin: **Anim Instance** is Self, **Regions** lists the limb names that have a Physics Region
   module. The function is marked thread safe, so the AnimGraph can call it on the animation
   worker threads when the animation blueprint uses multi-threaded update.
   - Spell each region exactly as its limb. The node logs a warning every frame for a name that
     matches no limb or control (`SetRecordParameters: Failed to find control with name ...`,
     `Engine/Plugins/Animation/PhysicsControl/Source/PhysicsControl/Private/RigidBodyWithControl.cpp:96`
     in UE 5.8.4), so a typo floods the log.

The node reads the curves the animation instance had after its last evaluation, so the physics
follows a change of weight one frame late.

## 3. The Physics Region modules

In the puppet asset, add one **Physics Region** module per region, after the modules that pose
it:

- **Region**: the limb name.
- **Physics Weight**: **Weight** for a fixed amount, or **Curve** to let a performer curve play
  it (its change from neutral, clamped to 0 to 1). For example a hand gesture's curve to let the
  tail go limp.
- **Damping**: the same, for extra damping.
- **Weight** (the module's): blends towards the authored physics; hand-offs fade it like any
  module's.

Do not run a Dynamic Chain (Tier 1) and a physics region over the same bones: both would move
them, and the physics region's bodies would follow the chain's simulated pose.

Takes that use Tier 2 also need the solved output pose recorded, because engine physics is not
deterministic: re-solving the performance does not reproduce it (ADR 0007, item 3).

## 4. The Wolf and the Eagle

The show uses the Mamaawi copies of the meshes (maintainer, 2026-10-27), with their physics
assets. The bodies below were read from each physics asset's constraints; a body with no
constraint would not be listed. Which regions are physical in the show is still open (ADR 0007
question 1), so the limbs below are a starting point.

### Wolf (`GrayWolf_oneMat`, `GrayWolf_oneMat_PhysicsAsset`)

15 bodies: `RigPelvis`, `RigSpine2`, `RigSpine3`, `RigChest`, `RigNeck1`, `RigNeck3`, `RigHead`,
`RigLFLeg2`, `RigRFLeg2`, `RigLBLeg1`, `RigLBLeg3`, `RigRBLeg1`, `RigRBLeg3`, `RigTail1`,
`RigTail4`.

| Limb (region) | Start Bone | Bodies |
|---|---|---|
| `Tail` | `RigTail1` | `RigTail1`, `RigTail4` |
| `LeftFrontLeg` | `RigLFLegCollarbone` | `RigLFLeg2` |
| `RightFrontLeg` | `RigRFLegCollarbone` | `RigRFLeg2` |
| `LeftHindLeg` | `RigLBLeg1` | `RigLBLeg1`, `RigLBLeg3` |
| `RightHindLeg` | `RigRBLeg1` | `RigRBLeg1`, `RigRBLeg3` |
| `Head` | `RigNeck1` | `RigNeck1`, `RigNeck3`, `RigHead` |
| `Body` (last) | `RigPelvis` | `RigPelvis`, `RigSpine2`, `RigSpine3`, `RigChest` |

A limb takes one start bone, so each leg is its own limb and its own Physics Region module; a
performer curve can drive several modules to play a pair of legs together. The ears, paws and
jaw have no bodies.

### Eagle (`BaldEagle_removedFeathers`, `BaldEagle_removedFeathers_PhysicsAsset`)

23 bodies: `RigPelvis`, `RigChest`, `RigNeck1`, `RigHead`, `RigTail_1`, `RigLLeg1`,
`RigLLegAnkle`, `RigRLeg1`, `RigRLegAnkle`, and on the wings `RigLArm1`, `RigLArm4`,
`RigLArmDigit11`, `RigLArmDigit21`, `RigLArmDigit32`, `RigLArmDigit41`, `RigLArmDigit51`,
`RigRArmCollarbone`, `RigRArm2`, `RigRArmDigit11`, `RigRArmDigit21`, `RigRArmDigit31`,
`RigRArmDigit41`, `RigRArmDigit51`.

| Limb (region) | Start Bone | Bodies |
|---|---|---|
| `LeftWing` | `RigLArmCollarbone` | `RigLArm1`, `RigLArm4` and five feather bodies |
| `RightWing` | `RigRArmCollarbone` | `RigRArmCollarbone`, `RigRArm2` and five feather bodies |
| `LeftLeg` | `RigLLeg1` | `RigLLeg1`, `RigLLegAnkle` |
| `RightLeg` | `RigRLeg1` | `RigRLeg1`, `RigRLegAnkle` |
| `Tail` | `RigTail_1` | `RigTail_1` |
| `Head` | `RigNeck1` | `RigNeck1`, `RigHead` |
| `Body` (last) | `RigPelvis` | `RigPelvis`, `RigChest` |

The two wings' bodies are not mirror images (`RigLArm1` and `RigLArm4` against
`RigRArmCollarbone` and `RigRArm2`; `RigLArmDigit32` against `RigRArmDigit31`), so the wings
react differently to the same strength until the physics asset is made symmetric. The talons
and the tail fan's feathers have no bodies.

## 5. Not verified yet

Needs a person at the desk:

- The curves reaching the animation instance: Puppet Solve's output curves on the pose that
  Make Physics Region Parameters reads (`UAnimInstance::GetCurveValueWithDefault`).
- The Wolf and the Eagle on Rigid Body With Control with these limbs: stable at rest, a region
  going limp and recovering as its curve plays, and contact with the ground.
- The cost of the node and of building the parameters every frame, in Unreal Insights. The
  function builds each region's curve names and a new parameter array on every call.
- Which thread the function runs on in the Wolf's and Eagle's animation blueprints.
