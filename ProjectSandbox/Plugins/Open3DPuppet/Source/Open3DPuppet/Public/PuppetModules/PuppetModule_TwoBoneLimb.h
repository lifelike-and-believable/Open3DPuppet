// Copyright 2026 Lifelike & Believable. All Rights Reserved.

#pragma once

#include "PuppetModule.h"

#include "PuppetModule_TwoBoneLimb.generated.h"

/** Where a limb's end effector (hand, foot, paw) goes (design overview §4). */
UENUM(BlueprintType)
enum class EPuppetEffectorMode : uint8
{
	/** Rotations only: the effector lands wherever the puppet's own limb lengths put it. */
	ForwardKinematics UMETA(DisplayName = "Forward Kinematics"),
	/** Relative to the limb's root, scaled by puppet limb length over performer limb length. Keeps reach poses (arm fully out) intact. */
	ScaledReach UMETA(DisplayName = "Scaled Reach"),
	/** Relative to the pelvis, scaled by the root scale. Keeps feet planted when the puppet is taller or shorter than the performer. */
	PelvisRelative UMETA(DisplayName = "Pelvis Relative"),
	/** The performer's own effector position. For contact with props and the stage. */
	Absolute UMETA(DisplayName = "Absolute"),
};

/**
 * Drives a two-bone limb (arm or leg) from the performer's limb: rotation deltas for all three
 * bones, then optionally two-bone IK to place the end effector, with the elbow or knee pointing
 * where the performer's does.
 */
UCLASS(meta = (DisplayName = "Two-Bone Limb"))
class OPEN3DPUPPET_API UPuppetModule_TwoBoneLimb : public UPuppetModule
{
	GENERATED_BODY()

public:
	/** Performer upper limb joint, by canonical name (upperarm_l, thigh_l, ...). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Performer", meta = (GetOptions = "GetPerformerJointOptions"))
	FName PerformerUpper = TEXT("upperarm_l");

	/** Performer lower limb joint (lowerarm_l, calf_l, ...). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Performer", meta = (GetOptions = "GetPerformerJointOptions"))
	FName PerformerLower = TEXT("lowerarm_l");

	/** Performer end joint (hand_l, foot_l, ...). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Performer", meta = (GetOptions = "GetPerformerJointOptions"))
	FName PerformerEnd = TEXT("hand_l");

	/** Puppet upper limb bone. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puppet", meta = (GetOptions = "GetPuppetBoneOptions"))
	FName PuppetUpper = TEXT("upperarm_l");

	/** Puppet lower limb bone; a descendant of PuppetUpper. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puppet", meta = (GetOptions = "GetPuppetBoneOptions"))
	FName PuppetLower = TEXT("lowerarm_l");

	/** Puppet end bone; a descendant of PuppetLower. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puppet", meta = (GetOptions = "GetPuppetBoneOptions"))
	FName PuppetEnd = TEXT("hand_l");

	/** Where the end effector goes. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Retarget")
	EPuppetEffectorMode EffectorMode = EPuppetEffectorMode::ScaledReach;

	/**
	 * Which way the elbow or knee points in the puppet's reference pose, in component space. Only
	 * needed when the limb is straight in the reference pose (arms in a T-pose); a bent reference
	 * limb gives the direction itself.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Retarget", meta = (EditCondition = "EffectorMode != EPuppetEffectorMode::ForwardKinematics"))
	FVector PoleDirection = FVector::ZeroVector;

	/** Let the limb stretch towards a target it cannot reach. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Retarget", meta = (EditCondition = "EffectorMode != EPuppetEffectorMode::ForwardKinematics"))
	bool bAllowStretching = false;

	/** The longest the limb may stretch, as a multiple of its length. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Retarget", meta = (ClampMin = "1.0", EditCondition = "bAllowStretching"))
	float MaxStretchScale = 1.2f;

	virtual TUniquePtr<FPuppetModuleSolver> CreateSolver() const override;
};
