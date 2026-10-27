# Desk test plan: performer clips, calibration and props in the editor

Step-by-step checks in the editor for what WP-3 (performer clips, #21), WP-2 (saved calibrations
#24, naming schemes #25, rest pose #26), WP-7 (a prop driving a chain) and WP-8 (bone pickers,
setup assistant, Puppet Session panel) added. Automated tests cover the solver
and helpers; these cover what only a running editor shows: the node's wiring, the buttons in the
details panel and real capture systems. The roadmap's [Next](../roadmap/roadmap.md#next) lists
the desk checks for other work packages.

Parts 1 to 3 need no performer: a recorded take stands in for one. The steps were written from
the code and docs, not in a running editor, so menu paths may differ slightly.

## 0. Setup (once)

1. Pull `develop`. Delete
   `ProjectSandbox/Intermediate/Build/Win64/x64/ProjectSandboxEditor/Development/Makefile.bin`,
   build `ProjectSandboxEditor` ([AGENTS.md](../../AGENTS.md#commands)), and open
   `ProjectSandbox.uproject`.
2. Bring in test content. Migrating from a Third Person template project is enough:
   - the UE5 mannequin (`SKM_Manny` and its skeleton);
   - two or three mannequin animations, at least one that travels (a walk or run) and one in
     place (an idle).
3. Create the puppet asset (Content Browser > Add > Puppet > **Puppet Asset**; the Puppet
   category also offers Puppet Performer Profile and Puppet Calibration Asset),
   named `PA_Test`.
   - Set Target Mesh to `SKM_Manny`.
   - Add modules: **Root**; **Chain** (`spine_01` to `spine_05`); **Chain** (`neck_01` to
     `head`); **Two-Bone Limb** for each arm and leg.
4. Create an Animation Blueprint for `SKM_Manny`, named `ABP_PuppetTest`.
   - Add **Puppet Solve** (in the Puppet category) and connect it to the Output Pose.
   - Set Puppet to `PA_Test` and add one cast entry with Slot `Body`.
5. Put `SKM_Manny` in a level with `ABP_PuppetTest` and open the Output Log.

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

1. Create a new Puppet Asset for `SKM_Manny` and press **Propose Modules**. **Expect:** Setup
   Result names the UE5 Mannequin scheme and lists Root, a spine Chain (`spine_01` to
   `spine_05`), a neck Chain, four Two-Bone Limbs (legs in Pelvis Relative), Digits for each
   hand, and no Dynamic Chain for the mannequin's IK, twist or corrective bones.
2. Press it again. **Expect:** nothing changes, and Setup Result says to remove the modules
   first.
3. A mesh with a tail or ears (or the Wolf once it exists). **Expect:** a Dynamic Chain for each.
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

## 9. Recording results

- File each failure as an Open3DPuppet issue with the `desk` label: the step number, what you
  saw, and the Output Log lines.
- For part 3, record each system's real streamed bone names, even when mapping works; that data
  checks the default tables.
