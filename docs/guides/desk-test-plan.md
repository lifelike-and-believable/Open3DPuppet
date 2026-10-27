# Desk test plan: performer clips and calibration in the editor

Step-by-step checks in the editor for what WP-3 (performer clips, #21) and WP-2 (saved
calibrations #24, naming schemes #25, rest pose #26) added. Automated tests cover the solver
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
3. Create the puppet asset (Content Browser > Miscellaneous > Data Asset > **Puppet Asset**),
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

## 5. Recording results

- File each failure as an Open3DPuppet issue with the `desk` label: the step number, what you
  saw, and the Output Log lines.
- For part 3, record each system's real streamed bone names, even when mapping works; that data
  checks the default tables.
