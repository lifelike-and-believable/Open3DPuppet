// Copyright 2026 Lifelike & Believable. All Rights Reserved.

#pragma once

#include "Containers/UnrealString.h"
#include "Math/Quat.h"
#include "Math/Transform.h"
#include "Templates/UniquePtr.h"
#include "UObject/Object.h"

#include "PuppetModule.generated.h"

class FPuppetPerformerPose;
class FPuppetPose;
struct FPuppetPerformerCalibration;

/** What a solver may look at when it binds (ADR 0002). Valid only during the Bind call. */
struct FPuppetBindContext
{
	/** The puppet's skeleton and reference pose. */
	const FPuppetPose& Puppet;
	/** The performer's skeleton, with a pose. */
	const FPuppetPerformerPose& Performer;
	/** The performer's calibration; it matches Performer's skeleton. */
	const FPuppetPerformerCalibration& Calibration;
	/** The puppet's pelvis (UPuppetAsset::PelvisBone), or INDEX_NONE. */
	int32 PuppetPelvis = INDEX_NONE;
	/** The performer's "pelvis" joint, or INDEX_NONE. */
	int32 PerformerPelvis = INDEX_NONE;
	/** Puppet pelvis height over performer pelvis height at rest; 1 when either is unknown. */
	double RootScale = 1.0;
	/** True when the module's slot is the puppet's root slot (ADR 0004). */
	bool bOwnsRoot = false;
};

/** What a solver reads and writes each frame. Valid only during the Solve call. */
struct OPEN3DPUPPET_API FPuppetSolveContext
{
	/** The pose being solved. Earlier modules have already written their bones. */
	FPuppetPose& Puppet;
	const FPuppetPerformerPose& Performer;
	const FPuppetPerformerCalibration& Calibration;
	/** Calibration.Alignment as a quaternion. */
	FQuat Alignment = FQuat::Identity;
	int32 PuppetPelvis = INDEX_NONE;
	int32 PerformerPelvis = INDEX_NONE;
	double RootScale = 1.0;
	float DeltaTime = 0.0f;
	/** Each performer curve's value at rest (neutral), indexed like the performer's curves; 0 where the calibration had none. */
	TConstArrayView<float> CurveNeutral;
	/**
	 * How much of the module's result to blend over the pose it received: the module's Weight times
	 * its slot's control blend (ADR 0008). Use this, not the module's own Weight.
	 */
	float Weight = 1.0f;
	/**
	 * True on the frame a hand-off to the performer starts or a re-centre is asked for (ADR 0008).
	 * The Root module on the root slot captures HandoffOffset then.
	 */
	bool bRecentre = false;
	/**
	 * The hand-off offset (ADR 0008): a turn about the vertical axis and a horizontal move that keep
	 * the puppet where an animation left it. The helpers below apply it, so every slot's modules turn
	 * with the puppet. The Root module on the root slot owns it: it captures it on bRecentre and
	 * converges it to identity; other modules only read it.
	 */
	FTransform HandoffOffset = FTransform::Identity;
	/** The puppet component's world transform this frame (ADR 0007), so dynamic modules feel it move and turn. */
	FTransform ComponentToWorld = FTransform::Identity;
	/** The component's world transform last frame; equal to ComponentToWorld when bTeleport is set. */
	FTransform PreviousComponentToWorld = FTransform::Identity;
	/**
	 * True on the first frame and when the component jumped further than the asset's Teleport
	 * Distance since the last: dynamic modules start their simulation again from the pose.
	 */
	bool bTeleport = false;

	/** A performer joint's rotation change since calibration, in puppet component space (PuppetRetarget::RotationDelta), turned by the hand-off offset. */
	FQuat GetJointDelta(int32 Joint) const;

	/** A performer joint's rotation change relative to its parent (PuppetRetarget::RotationDeltaInParent); GetJointDelta for a root joint. */
	FQuat GetJointDeltaInParent(int32 Joint) const;

	/** A performer curve's current value (index from FPuppetPerformerPose::FindCurve). */
	float GetCurve(int32 Curve) const;

	/** A performer curve's change from its neutral value. */
	float GetCurveDelta(int32 Curve) const;

	/** A performer joint's current component-space position, in puppet orientation, with the hand-off offset applied. */
	FVector GetJointPosition(int32 Joint) const;

	/** A performer joint's position change since calibration, in puppet orientation, unscaled, turned by the hand-off offset. */
	FVector GetJointTranslation(int32 Joint) const;
};

/**
 * The per-instance half of a puppet module: created once per Puppet Solve node instance by
 * UPuppetModule::CreateSolver. It caches bone indices and any state between frames.
 */
class OPEN3DPUPPET_API FPuppetModuleSolver
{
public:
	virtual ~FPuppetModuleSolver() = default;

	/**
	 * Resolves bone and joint names. Called when the puppet's skeleton, the performer's skeleton,
	 * the calibration or the module's settings change. Return false with OutError set when the
	 * module cannot run (a missing bone, for example); the module is then skipped until the next
	 * bind. May allocate.
	 */
	virtual bool Bind(const FPuppetBindContext& Context, FString& OutError) = 0;

	/** Writes the module's bones. Runs on an animation worker thread every frame; must not allocate. */
	virtual void Solve(FPuppetSolveContext& Context) = 0;
};

/**
 * A solver that keeps a reference to its module's settings and reads them each frame, so edits in
 * the details panel are live. The Puppet Solve node keeps the module alive while the solver exists.
 */
template <typename TModule>
class TPuppetModuleSolver : public FPuppetModuleSolver
{
public:
	explicit TPuppetModuleSolver(const TModule& InSettings) : Settings(InSettings) {}

protected:
	const TModule& Settings;
};

/**
 * A puppet module's settings: one region of the puppet, the performer slot that drives it and its
 * retargeting controls (ADR 0002). Instanced inside a UPuppetAsset and shared by every actor using
 * that puppet, so it holds no per-frame state; CreateSolver makes the object that does.
 *
 * To add a module to the library, follow docs/guides/authoring-puppet-modules.md.
 */
UCLASS(Abstract, EditInlineNew, DefaultToInstanced, CollapseCategories)
class OPEN3DPUPPET_API UPuppetModule : public UObject
{
	GENERATED_BODY()

public:
	/** When off, the module leaves its bones as the modules before it left them. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Module")
	bool bEnabled = true;

	/** The performer slot (UPuppetAsset::PerformerSlots) whose performer drives this module. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Module", meta = (GetOptions = "GetPerformerSlotOptions"))
	FName PerformerSlot = TEXT("Body");

	/** Blends the module's result over the pose it received: 0 leaves the pose, 1 is full performer control. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Module", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float Weight = 1.0f;

	/** Creates this module's solver. */
	virtual TUniquePtr<FPuppetModuleSolver> CreateSolver() const PURE_VIRTUAL(UPuppetModule::CreateSolver, return nullptr;);

	/** The owning puppet asset's mesh bones, for the bone pickers (UPuppetAsset::GetPuppetBoneOptions). */
	UFUNCTION()
	TArray<FName> GetPuppetBoneOptions() const;

	/** The canonical joint names (ADR 0003), for the performer joint pickers. */
	UFUNCTION()
	TArray<FName> GetPerformerJointOptions() const;

	/** The owning puppet asset's performer slots, for Performer Slot. */
	UFUNCTION()
	TArray<FName> GetPerformerSlotOptions() const;

#if WITH_EDITOR
	/** Tells the owning puppet asset that it changed, so running Puppet Solve nodes bind again. */
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
};
