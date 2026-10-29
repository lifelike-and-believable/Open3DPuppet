# Desk test plan

Step-by-step checks in the editor, one part per feature. Automated tests cover the solver and
helpers; these cover what only a running editor shows: the node's wiring, the buttons in the
details panel, the modules on real meshes and real capture systems.

- Parts 1 to 8: performer clips (WP-3), calibrations, naming schemes and rest pose (WP-2), a prop
  driving a chain (WP-7), and the bone pickers, setup assistant and Puppet Session panel (WP-8).
- Parts 9 to 18: each module, driven by recorded mannequin motion (WP-1, WP-4, WP-5, WP-6, WP-10,
  WP-11 to WP-16), on the mannequin and on the Wolf and the Eagle as test meshes.
- Part 19: the same live, with a suit, face capture and fingers.

The Wolf and the Eagle are test cases for the features here, not production puppets yet.

Parts 1, 3, 4 and 9 to 18 need no performer: a recorded take stands in for one. The steps were written from
the code and docs, not in a running editor, so menu paths may differ slightly.

## 0. Setup (once)

1. Pull `develop`. Delete
   `ProjectSandbox/Intermediate/Build/Win64/x64/ProjectSandboxEditor/Development/Makefile.bin`,
   build `ProjectSandboxEditor` ([AGENTS.md](../../AGENTS.md#commands)), and open
   `ProjectSandbox.uproject`.
2. Bring in test content. Migrating from a Third Person template project is enough:
   - the UE5 mannequin (`SKM_Manny_Simple`, `SKM_Quinn_Simple` and their skeleton);
   - two or three mannequin animations, at least one that travels (a walk or run) and one in
     place (an idle).
3. Create the puppet asset (Content Browser > Add > Puppet > **Puppet Asset**; the Puppet
   category also offers Puppet Performer Profile and Puppet Calibration Asset),
   named `PA_Test`.
   - Set Target Mesh to `SKM_Manny_Simple`.
   - Add modules: **Root**; **Chain** (`spine_01` to `spine_05`); **Chain** (`neck_01` to
     `head`); **Two-Bone Limb** for each arm and leg.
4. Create an Animation Blueprint for `SKM_Manny_Simple`, named `ABP_PuppetTest`.
   - Add **Puppet Solve** (in the Puppet category) and connect it to the Output Pose.
   - Set Puppet to `PA_Test` and add one cast entry with Slot `Body`.
5. Put `SKM_Manny_Simple` in a level with `ABP_PuppetTest` and open the Output Log.

**Pass if** the log shows a slot state line such as
`Puppet PA_Test, slot Body: no performer data.` and nothing crashes.

## 1. Performer clips (WP-3)

1. In the cast entry, set **Source** to `Animation` and **Animation** to the walk or run.
   - **Expect:** the Live Link fields hide and the Animation fields show.
   - Compile. **Expect:** the puppet plays the walk in the viewport and travels with it, and the
     log reports the slot as driving.
2. **Play Rate:** set `2`, then `-1`. **Expect:** double speed, then backwards.
3. **Loop:** turn it off. **Expect:** the clip plays once and holds its last frame.
4. **Start Time:** set it to half the clip's length, then recompile or reopen. **Expect:**
   playback starts halfway.
5. **Use Time:** turn it on and set **Time** to a few values. **Expect:** the puppet shows that
   frame and stays there.
   - Optionally, the Sequencer route: expose **Cast** as a pin, drive Time from a Blueprint
     variable, and key that variable in Sequencer. **Expect:** scrubbing Sequencer scrubs the
     take.
6. **Calibration:**
   - With **Clip Calibration** set to `Frame` and **Calibration Time** at `0`, the first frame is
     the rest pose.
   - Set **Calibration Time** to a mid-stride frame. **Expect:** the whole performance shifts,
     because the rest pose changed.
   - Switch to `Reference Pose`. **Expect:** it retargets from the clip skeleton's reference
     pose.
   - Set **Calibration Alignment** yaw to `90`. **Expect:** the puppet travels 90 degrees to the
     side of the take's direction.
7. **Retargeting modes:** open the clip's skeleton, set the pelvis's translation retargeting to
   `Skeleton`, and save. **Expect:** the puppet still travels. Walking in place is a failure.
8. **Rejected clip:** use an additive animation as the clip. **Expect:** an Output Log warning,
   `... cannot play as a performer: the sequence is additive...`, and the slot stops updating.
9. **Back to Live Link:** set Source back to `Live Link`. **Expect:** the slot uses the subject
   and its calibration from the subsystem; nothing is left over from the clip's calibration.
10. **Hand-off from a clip (with WP-16):**
    - Add a **Puppet Performance Component** to the actor, set the slot to start on the
      animation, and feed the Puppet Solve **Source** pin an idle.
    - Call `TransitionToPerformer(Body, 2 s)` from the level Blueprint.
    - **Expect:** a smooth blend from the idle into the take, with no snap in position.

## 2. Saved calibrations (WP-2, #24). Needs a Live Link subject

1. Start any Live Link source (Open3DBroadcast, VMC, or a capture system) and check it is in the
   Live Link panel.
2. Create a **Puppet Calibration Asset** and set **Subject**. Have the performer stand in T-pose
   and press **Capture**.
   - **Expect:** the log says `Calibrated performer <subject> (N bones, M curves).` and the
     asset's calibration fills in.
   - Save the asset.
3. Press **Capture** with a misspelled subject. **Expect:** a `Cannot calibrate ...` warning,
   and the saved calibration is unchanged.
4. Set the cast entry's **Saved Calibration** to the asset and restart the editor without
   calibrating. **Expect:** the puppet follows the performer right away.
5. **Live wins over saved:** have the performer stand slightly differently and press **Capture**
   again. **Expect:** the puppet picks up the new calibration without a restart.
6. Press **Ctrl+Z** after a Capture. **Expect:** the previous calibration comes back and the
   puppet uses it.
7. **Apply** on a second asset that holds an older calibration. **Expect:** the puppet switches
   to that calibration.

## 3. Naming schemes and auto-mapping (WP-2, #25)

1. Open Project Settings > Plugins > **Open3DPuppet** > Naming Schemes.
   - **Expect:** 11 schemes: UE5 Mannequin, Xsens, OptiTrack / Motive, Rokoko, Vicon, Qualisys,
     Sony Mocopi, Move One, HumanIK / Motionbuilder, Mixamo, Unity humanoid / VMC.
   - Edit a value and restart. **Expect:** the edit persists, in the project's
     `Config/DefaultOpen3DPuppet.ini`.
2. Create a **Puppet Performer Profile**.
   - **Mannequin:** set Source Skeleton to the mannequin skeleton and press **Map From
     Skeleton**. **Expect:** Mapping Result says the bones already use canonical names and no
     mapping is needed.
   - **Mixamo:** import a Mixamo character (its bones are named `mixamorig:...`) and map from its
     skeleton. **Expect:** about 51 joints mapped as Mixamo (possibly with an equivalent scheme
     listed), `spine_05` to `mixamorig:Spine2`.
3. **For each capture system the show uses:** set Source Subject and press **Map From Live Link
   Subject**.
   - Read Mapping Result: which scheme matched, and which joints were not found.
   - Wrong or missing names mean that system streams other names than the engine's templates.
     Fix the scheme in Project Settings and note the system's real names.
4. Use a profile from step 3 in the cast entry. **Expect:** that system's performer drives the
   puppet with the spine, arms and legs right.

## 4. Puppet rest pose (WP-2, #26)

1. **A simulated A-pose, no special mesh needed:** make a one-frame AnimSequence of Manny with
   the arms lowered about 45 degrees, and set it as `PA_Test`'s **Rest Pose** (Rest Pose Time
   `0`).
2. Drive the puppet with a performer calibrated in T-pose (live, or a clip calibrated from a
   T-pose frame).
   - **Expect:** while the performer holds the T-pose, the puppet's arms hang about 45 degrees
     down, as in the rest pose.
   - **Expect:** arm movement follows from there.
3. Clear Rest Pose. **Expect:** the arms follow the mesh's reference pose again.
4. **Unusable rest pose:** set Rest Pose to an additive clip. **Expect:** a warning,
   `rest pose ... cannot be used ...; modules retarget from the mesh's reference pose`.
5. **A real A-pose mesh**, where one exists: a T-pose rest clip for it makes a T-pose-calibrated
   performer look right.

## 5. A prop drives a tail (WP-7)

Set up the [multi-performer sample](multi-performer-sample.md): a mesh with a tail, slots `Body`
and `Tail`, a Prop Chain on `Tail`, the rod's Transform-role subject cast into `Tail`.

1. Calibrate the rod at rest (**Capture** on a calibration asset for its subject). **Expect:** the
   log says `Calibrated performer <rod> (1 bones, 0 curves).`
2. Tilt the rod. **Expect** with Shape `Bend`: the tail bends from its base, its tip turning as
   far as the rod; with `Rigid`, the whole tail turns as one piece.
3. Turn the body performer around with the rod still. **Expect** with Space `Parent`: the tail
   turns with the body; with `Stage`: the tail's tip keeps pointing where the rod points.
4. **Amplification** `2`: the tail turns twice as far as the rod. **Smoothing** `0.2`: a sharp rod
   movement reaches the tail over about a fifth of a second, and tracking jitter calms.
5. Before calibrating the rod, or with the rod's subject gone: **Expect:** the tail stays in the
   node's input pose while the body still drives.
6. A hand instead of a rod: set Performer Joint to `hand_l` on a Body-slot Prop Chain. **Expect:**
   the tail follows the performer's left hand.

## 6. Bone pickers (WP-8)

1. Open `PA_Test` with Target Mesh set. **Expect:** every module's puppet bone field (Puppet
   Start, Puppet Upper, Feathers, a collision sphere's Bone, the asset's Pelvis Bone) is a
   dropdown of the mesh's bones, in skeleton order.
2. **Expect:** performer joint fields (Performer Start, Performer Joint, ...) list the canonical
   joints, `pelvis` first and `prop` last; Performer Slot and Root Slot list the asset's slots.
3. Clear Target Mesh. **Expect:** bone fields fall back to the canonical joints.
4. A performer profile: the mapping's keys list the canonical joints; its values list Source
   Skeleton's bones, or, without one, the naming schemes' bone names and the values already
   mapped.
5. Curve fields (Performer Curve, Puppet Curve) stay free text.

## 7. Setup assistant (WP-8)

1. Create a new Puppet Asset for `SKM_Manny_Simple` and press **Propose Modules**. **Expect:**
   Setup Result names the UE5 Mannequin scheme and lists Root, a spine Chain (`spine_01` to
   `spine_05`), a neck Chain, four Two-Bone Limbs (legs in Pelvis Relative), Digits for each
   hand, and no Dynamic Chain for the mannequin's IK, twist or corrective bones.
2. Press it again. **Expect:** nothing changes, and Setup Result says to remove the modules
   first.
3. A mesh with a tail or ears (the Wolf, `GrayWolf_oneMat`). **Expect:** a Dynamic Chain for each.
4. A Mixamo-named mesh. **Expect:** the puppet bones keep the mesh's names (`mixamorig:...`)
   while the performer joints stay canonical.
5. Drive the proposed puppet with a performer. **Expect:** the body follows without further
   setup; then check the right hand's Digits Spread Axis and tune the Dynamic Chains.

## 8. Puppet Session panel (WP-8)

Open **Window > Tools > Puppet Session** with the setup from part 0 and a Live Link source
running.

1. **Performers:** each Live Link subject is listed as Streaming, and each calibrated subject as
   Calibrated, also when its source is off. **Calibrate** (with an alignment yaw) calibrates it;
   **Clear** forgets it, and the puppet stops following.
2. Choose a Puppet Calibration Asset in **Save captures to** and press **Save to asset** on a
   performer. **Expect:** the asset holds the capture with that subject and alignment.
3. **Running puppets:** the puppet's actor and asset, and a row per slot with its source (Live Link
   subject or clip), its state (No data, Not calibrated, Driving) updating as the performer
   connects and calibrates. A puppet in a linked anim graph is listed too.
4. With a Puppet Performance Component on the actor: drag **Control** and watch the slot blend;
   **To performer** and **To animation** run a transition of the set duration; **Re-centre**
   keeps the puppet where it is. Without one, the row says the slot is always under the
   performer.
5. **Tune** shows the puppet asset below. Drag a module's Amplification or Stiffness while the
   performer moves after a hand-off. **Expect:** the change shows at once and the puppet does not
   snap back to the performer or reset its dynamics. Change a bone name: the puppet binds again
   (that edit is allowed to reset it).
6. Start Play In Editor. **Expect:** the panel switches to the PIE world's puppets, and back when
   play stops.

## 9. Recorded motion as the performer (parts 10 to 18)

Parts 10 to 18 test the modules with a mannequin AnimSequence cast as the performer, so every run
repeats exactly and needs no suit. Live checks come after, with the same setups (part 19).

- **Clips:** the Third Person template's mannequin animations under
  `Characters/Mannequins/Anims/`, all on the mannequin skeleton, so no performer profile is
  needed:
  - travelling: `Unarmed/Walk/MF_Unarmed_Walk_Fwd`, `Unarmed/Jog/MF_Unarmed_Jog_Fwd`;
  - in place: `Unarmed/MM_Idle`;
  - big arm and hand motion: `Unarmed/Attack/MM_Attack_01` (punches, fists),
    `Unarmed/Jump/MM_Jump`;
  - crouching or falling: `Death/MM_Death_Front_01`.
- **Cast entry for every part:** Source `Animation`, Loop on, **Clip Calibration**
  `Reference Pose`. The mannequin skeleton's reference pose is the performer's rest pose, so a
  module retargets the clip's motion away from that pose onto the puppet's reference pose. (Part
  1 tests the `Frame` calibration.)
- **Test meshes:** `SKM_Manny_Simple` and `SKM_Quinn_Simple` (mannequin skeleton), and the Wolf
  (`000-Maamawi/001-Characters/Animals_Fixed/GrayWolf_oneMat`) and the Eagle
  (`BaldEagle_removedFeathers`). Their bones are named `Rig...` (the Wolf's left front leg runs
  `RigLFLegCollarbone`, `RigLFLeg1`, `RigLFLeg2`, `RigLFLeg3`, `RigLFLegAnkle`,
  `RigLFLegDigit11`). Open the mesh's skeleton to read names; the bone pickers list them once the
  puppet asset's Target Mesh is set.
- **One puppet asset and Animation Blueprint per mesh,** as in part 0: `PA_Manny`, `PA_Quinn`,
  `PA_Wolf`, `PA_Eagle`, each with one slot `Body`. Watch in the Animation Blueprint's preview or
  in a level; the Output Log shows the slot state and any module that cannot run (`module ...
  cannot run: ...`).
- **Left and right:** the steps map the performer's `_l` joints to the creature's `RigL...` bones.
  If a creature's legs or wings move on the wrong side, swap the L and R bones and note it in the
  results.
- **Curves from a clip:** a clip gives bones only, but the finger-curl curves (`curl_index_l`,
  `curl_thumb_r`, ...: 0 with the fingers straight, about 1 in a fist) come from its finger bones.
  The curve-driven parts use them, with `MM_Attack_01`'s fists.
- **Not testable from recorded motion:** Eyes (it needs eye-look curves or eye joints; the
  mannequin has neither) and face curves (`jawOpen` and the other ARKit curves). They wait for
  Live Link Face (part 19).

## 10. Body: Root, Chain, Two-Bone Limb (WP-1)

1. `PA_Manny` with the modules of part 0 (Root, a spine Chain, a neck Chain, four Two-Bone
   Limbs), cast `MF_Unarmed_Walk_Fwd`. **Expect:** Manny walks as in the clip and travels, with no
   sliding feet.
2. **Proportions:** `PA_Quinn` with the same modules (or Propose Modules), same clip. **Expect:**
   Quinn walks with her own stride and the feet stay planted. On the Two-Bone Limb legs, try each
   **Effector Mode**:
   - `Pelvis Relative`: feet planted;
   - `Forward Kinematics`: rotations only, feet may slide or float a little;
   - `Scaled Reach`: reach kept relative to the limb's root;
   - `Absolute`: the clip's own foot positions.
3. **Root:**
   - **Horizontal Scale** `2`: the puppet travels twice as far per step (feet slide).
   - **Vertical Scale** with `MM_Jump`: the jump's height scales.
   - **Height Offset** `20`: the pelvis rises 20 cm.
   - Turn off **Drive Translation**: the puppet walks in place.
4. **Chain:** on the spine Chain, **Amplification** `0` (the spine stays in its reference pose)
   and `2` (the spine sways twice as much).
5. **Weight:** on one arm's Two-Bone Limb, Weight `0`: the arm stays in the node's input pose;
   `0.5`: halfway.
6. **Stretch:** on an arm with Effector Mode `Absolute`, cast `MM_Attack_01` on Quinn. With
   **Allow Stretching** on and **Max Stretch Scale** `1.2`, **Expect:** the arm stretches to reach
   up to 20% further, never more.

## 11. Hand-offs and re-centre (WP-16)

On `PA_Manny`, with a **Puppet Performance Component** on the actor (part 1, step 10, has the
setup):

1. **Slots Starting On Animation** lists `Body`, and Puppet Solve's **Source** pin plays
   `MM_Idle`. **Expect:** Manny idles; the cast walk does not show.
2. Call `TransitionToPerformer(Body, 2)` from the level Blueprint (or **To performer** in the
   Puppet Session panel). **Expect:** a 2-second blend into the walk; the puppet does not jump to
   the clip's position.
3. **Convergence:** while walking, the puppet's offset from the clip's path closes over Root's
   **Convergence Time** (2 s). With **Converge Only While Moving** on, cast `MM_Idle` instead and
   hand off: the offset holds while the performer stands still. **Hold Offset** on: it never
   converges.
4. `TransitionToAnimation(Body, 1)`: back to the idle in 1 second, no snap.
5. **Re-centre** mid-walk (`Recentre(Body)` or the panel's button). **Expect:** the puppet stays
   where it is and converges as after a hand-off.
6. **Control** slider in the Puppet Session panel at `0.5`: the pose is halfway between idle and
   walk.

## 12. Fingers: Digits and finger curls (WP-12)

1. On `PA_Manny`, add **Digits** for the left hand (or use Propose Modules, which adds one per
   hand) mapping each finger: `index_01_l` to `index_03_l` onto the same puppet bones, and so on
   for thumb, middle, ring and pinky. Cast `MM_Attack_01`.
   - **Expect:** the fingers close into fists as in the clip.
   - **Curl Gain** `0`: the fingers stay straight; `1.5`: they curl further.
   - **Spread Gain** `0`: the fingers keep their rest spread.
   - If the spread goes the wrong way on the right hand, flip **Spread Axis** and note it.
2. **Curl curves:** open the Animation Blueprint preview's **Anim Curves** panel, or add a Curve
   Remap (part 13) to see them. **Expect:** `curl_index_l` reads about 0 with the hand open and
   about 1 in a fist.
3. **Wolf paw toes** on `PA_Wolf`: Digits with Performer `index_01_l` to `index_03_l` and Puppet
   `RigLFLegDigit11` to `RigLFLegDigit11` (one toe bone per paw). **Expect:** the toe curls with
   the fist. If a one-bone digit is rejected, note the Output Log message.
4. **Eagle talons** on `PA_Eagle`: four digits per foot, for example index onto
   `RigLLegDigit11` to `RigLLegDigit13`, middle onto `RigLLegDigit21` to `23`, ring onto `31` to
   `33`, thumb onto `41` to `43`. **Expect:** the talons close with the fists.

## 13. Curves: Curve to Bone and Curve Remap (WP-11, WP-13)

Driven by the finger-curl curves from `MM_Attack_01` (part 9).

1. **Curve to Bone** on `PA_Wolf`: Performer Curve `curl_index_l`, Puppet Bone `RigJaw1`,
   **Rotation** pitch `-20` (start there; set the axis for the mesh). **Expect:** the jaw opens as
   the left fist closes.
   - **Input Max** `0.5`: full open at half a fist.
   - **Dead Zone** `0.3`: small curls do nothing.
   - **Smoothing Time** `0.2`: the jaw lags and calms.
2. The same on the ears: Curve to Bone on `RigLEar1` and `RigREar1` from `curl_index_r`, a few
   degrees of Rotation. **Expect:** the ears flatten with the right fist.
3. On `PA_Eagle`, the beak: Curve to Bone on `RigJaw` from `curl_index_l`.
4. **Curve Remap** on any puppet: a mapping from `curl_index_l` to a puppet curve `TestCurve`,
   Output Max `2`. **Expect** in the Anim Curves panel: `TestCurve` goes from 0 to about 2 with
   the fist. Two mappings into the same curve add up. (The Maamawi meshes have no morph targets,
   so nothing changes shape. A mesh with morph targets shows the remap on its shapes.)
5. **Neutral:** set Clip Calibration to `Frame` at a time with closed fists. **Expect:** the curve
   modules read 0 there, because the curl's change from neutral is what drives them.

## 14. Digitigrade leg (WP-4)

`PA_Wolf`: Root on `RigPelvis` (set the asset's **Pelvis Bone** to `RigPelvis`), a Chain from
`spine_01` to `spine_05` onto `RigSpine1` to `RigChest`, a Chain from `neck_01` to `head` onto
`RigNeck1` to `RigHead`, and four Digitigrade Legs:

| Leg | Performer Upper, Lower, End | Puppet Upper, Lower, Metatarsal, Paw |
|---|---|---|
| Left hind | `thigh_l`, `calf_l`, `foot_l` | `RigLBLeg1`, `RigLBLeg2`, `RigLBLeg3`, `RigLBLegAnkle` |
| Right hind | `thigh_r`, `calf_r`, `foot_r` | `RigRBLeg1`, `RigRBLeg2`, `RigRBLeg3`, `RigRBLegAnkle` |
| Left front | `upperarm_l`, `lowerarm_l`, `hand_l` | `RigLFLeg1`, `RigLFLeg2`, `RigLFLeg3`, `RigLFLegAnkle` |
| Right front | `upperarm_r`, `lowerarm_r`, `hand_r` | `RigRFLeg1`, `RigRFLeg2`, `RigRFLeg3`, `RigRFLegAnkle` |

1. Cast `MF_Unarmed_Walk_Fwd`. **Expect:** the hind legs step with the clip's legs, hocks bending
   back; the front legs swing with the arms. A human walk makes an odd wolf gait; this checks the
   legs, not the gait.
2. **Knee To Hock Ratio** `0`, `0.5`, `1` on a hind leg: the metatarsal stays at its reference
   angle, takes half, then all of the knee bend.
3. **Heel Lift** `20`: the hock rises with the paw where it was.
4. **Contact Height** with the walk: the paw stays level on the ground while within that height,
   and tilts above it.
5. **Stride Scale** `1.5`: longer steps forward and back, not higher.
6. `MM_Death_Front_01` (a fall): **Expect:** no leg flips or snaps as the legs fold.

## 15. Dynamic Chain: tail and solver dynamics (WP-5, WP-14)

`PA_Wolf` from part 14, plus a **Dynamic Chain (Tail)** from `RigTail1` to `RigTail6`.

1. Cast `MF_Unarmed_Jog_Fwd` and let the puppet travel. **Expect:** the tail lags, swings and
   settles when the clip turns or stops. At the clip's loop the puppet jumps back to the start of
   its path; note whether the tail whips there.
2. **Teleport:** move the actor more than the asset's **Teleport Distance** (200 cm) in one frame
   (set its location from the level Blueprint). **Expect:** the tail starts again from the pose
   instead of whipping across.
3. **Stiffness** `0` (swings freely), `1` (holds the pose); **Damping** `0.5` (keeps swinging),
   `8` (settles at once); **Amplitude** `2` (twice the swing).
4. **Physics Weight** `0`: the tail follows the pose exactly; `1`: fully simulated. **Curve**
   `curl_index_l` with `MM_Attack_01`: the tail goes physical as the fist closes.
5. **Collision:** add **Collision Bones** `RigLBLeg2` and `RigRBLeg2`, radius `6`. **Expect:** the
   tail no longer passes through the hind legs. **Collide With Ground** on, Ground Height `0`:
   the tail tip stays above the floor.
6. **Frame rate:** in the console, `t.MaxFPS 20`, then `t.MaxFPS 120`. **Expect:** the tail
   swings the same way at both; nothing explodes at 20.
7. **Input Mix** with Performer Start `spine_01`, Performer End `spine_05`, Input Mix `1`:
   **Expect:** the tail's followed pose bends with the performer's spine.

## 16. Wing (WP-6)

`PA_Eagle`: Root on `RigPelvis` (asset **Pelvis Bone** `RigPelvis`), and a **Wing** per side:

| Wing | Performer Upper, Lower | Puppet Upper, Lower, Hand | Feathers, in fan order |
|---|---|---|---|
| Left | `upperarm_l`, `lowerarm_l` | `RigLArm1`, `RigLArm2`, `RigLArm3` | `RigLArmDigit51`, `RigLArmDigit41`, `RigLArmDigit11`, `RigLArmDigit21`, `RigLArmDigit31` |
| Right | `upperarm_r`, `lowerarm_r` | `RigRArm1`, `RigRArm2`, `RigRArm3` | the same with `RigR...` |

1. Cast `MM_Jump` or `MM_Attack_01`. **Expect:** the wings flap as the arms rise and fall, and
   fold as the elbows bend.
2. **Flap Amplification** `2`: twice the flap. **Fold Start Angle** and **Fold End Angle**: the
   elbow bend range over which the wing folds.
3. **Forearm Fold**, **Hand Fold** and **Feather Fold** are set for the mesh: adjust them until a
   fully bent elbow gives a folded wing, and record the values that work for the Eagle.
4. The legs: two Digitigrade Legs from `thigh_l`, `calf_l`, `foot_l` onto `RigLLeg1`,
   `RigLLeg2`, `RigLLeg3`, `RigLLegAnkle` (and the right side), with the walk.
5. **The tail fan** (WP-18): add a **Fan** module after the Wings and after any module that moves
   the tail root, since the fan is posed under its parent as that is by then. **Performer
   Joints** `lowerarm_l`, `lowerarm_r` (the default); **Feathers** the 11 tail feathers, edge to
   edge. The mesh names them `RigTail_2_01` to `RigTail_2_11` (read from the asset file, not
   checked in the editor): in the skeleton tree, confirm their parent and that this order runs
   across the fan from one edge to the other, and correct the list if not.
   - Cast `MM_Attack_01`. **Expect:** the tail opens as the arms straighten and closes as the
     elbows bend, with the wings; the middle feather stays.
   - **Spread Axis** and **Spread Angle** are set for the mesh: find the axis that fans the
     feathers sideways (try `Z`, then `X` or `Y`), then an angle that opens the fan as wide as the
     mesh allows without feathers crossing. Record both.
   - **Open Bend** and **Closed Bend** default to the Wing's fold range (20 and 120); change them
     with the Wing's **Fold Start Angle** and **Fold End Angle** so tail and wings stay together.
   - **Smoothing Time** `0.1`: the tail opens and closes more softly than the wings.

## 17. Control Rig adapter (WP-10)

1. **A rig made in the editor:** on `SKM_Manny_Simple`, create a Control Rig (`CR_PuppetTest`).
   Import the hierarchy, add a transform control `head_ctrl` placed on `head`, and a public float
   variable `Tilt`. In Forwards Solve, set `head`'s global rotation from `head_ctrl`'s, turned by
   `Tilt` degrees about one axis.
2. On `PA_Manny`, add a **Control Rig** module: Rig `CR_PuppetTest`; **Inputs**: Control
   `head_ctrl`, Input `Joint Rotation`, Performer Joint `head`; **Bones**: `head`.
   - **Expect:** **Variables** lists `Tilt` with the rig's default when the rig is picked.
   - Cast the walk. **Expect:** the head moves with the clip's head through the rig.
   - Set `Tilt` to `30` on the module. **Expect:** the head tilts, live, without recompiling.
3. Remove `Tilt` from the rig and compile it, then press **Refresh Variables**. **Expect:** it
   goes from the list; a value set for a variable that still exists is kept.
4. **Errors:** a control name that does not exist, a bone not in Bones. **Expect:** `the Control
   Rig has no control '...'` in the Output Log, and the module skipped.
5. **Cost:** with two or three Manny puppets running the rig, record an Unreal Insights trace and
   note the Puppet Solve and rig times per frame.
6. Optional: the template's `Rigs/CR_Mannequin_Body` as the rig, with one of its controls as an
   input.

## 18. Physics Region (WP-15)

Follow the [setup guide](physics-hand-off.md). Start on Manny, whose physics asset
(`Rigs/PA_Mannequin`) covers the whole body, then the Wolf and the Eagle with the guide's limbs.

1. `ABP_PuppetTest`: Puppet Solve, then **Rigid Body With Control**, with Character Setup Data
   limbs `LeftArm` (Start Bone `clavicle_l`), `RightArm` (`clavicle_r`) and `Body` (`pelvis`,
   last). Connect **Make Physics Region Parameters** (Self, Regions `LeftArm`, `RightArm`) to its
   Control And Modifier Parameters pin.
2. On `PA_Manny`, a **Physics Region** with Region `LeftArm`. Cast the walk.
   - **Physics Weight** `0`: the arm follows the walk as authored.
   - `1`: the arm hangs limp. `0.5`: in between.
   - **Damping** `1`: the arm settles faster.
3. **Physics Weight Curve** `curl_index_l` with `MM_Attack_01`. **Expect:** the left arm goes
   limp as the fist closes, and recovers as it opens.
4. **Fails safe:** the module's Weight `0`, or the module disabled. **Expect:** the arm as
   authored, not limp.
5. **A typo:** Regions `LeftArn`. **Expect:** a warning every frame, `SetRecordParameters: Failed
   to find control with name LeftArn` (the setup guide warns of it).
6. Check that the curves `Physics_LeftArm` and `Physics_LeftArm_Damping` appear in the Anim
   Curves panel.
7. The Wolf's `Tail` and `LeftHindLeg`, the Eagle's `LeftWing` and `RightWing`, with the setup
   guide's limbs. **Expect:** stable at rest, limp and back as the weight plays. Note how the
   Eagle's two wings differ (its physics asset is not symmetric).

## 19. Live checks (after the recorded ones)

With the same puppets, a Rokoko suit through Live Link in place of the clip (cast Source
`Live Link`, a performer profile from **Map From Live Link Subject**, part 3), calibrated in the
Puppet Session panel (part 8):

- Parts 10 to 18 with the performer moving live: anything that worked with a clip and fails live
  points at calibration, naming or the stream.
- Face (Live Link Face): Eyes, and Curve to Bone and Curve Remap from ARKit curves (`jawOpen`,
  `eyeBlinkLeft`), with a curve-only slot (Basic role).
- Gloves or the suit's fingers: Digits and the curl curves from live fingers.

## 20. Recording results

- File each failure as an Open3DPuppet issue with the `desk` label: the step number, what you
  saw, and the Output Log lines.
- For part 3, record each system's real streamed bone names, even when mapping works; that data
  checks the default tables.
