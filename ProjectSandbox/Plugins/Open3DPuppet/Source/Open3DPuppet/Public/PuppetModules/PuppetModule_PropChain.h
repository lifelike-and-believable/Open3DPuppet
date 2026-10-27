// Copyright 2026 Lifelike & Believable. All Rights Reserved.

#pragma once

#include "PuppetModule.h"

#include "PuppetModule_PropChain.generated.h"

/** How a prop's rotation spreads along the chain. */
UENUM(BlueprintType)
enum class EPuppetPropShape : uint8
{
	/** The chain bends: its first bone keeps its rest, each bone further along turns more, and the last turns fully with the prop. */
	Bend,
	/** The whole chain turns with the prop, as one piece. */
	Rigid,
};

/** What the prop's rotation is relative to. */
UENUM(BlueprintType)
enum class EPuppetPropSpace : uint8
{
	/** On top of the chain's parent bone: the chain moves with the body, and the prop adds to it. */
	Parent,
	/** The stage: the end of the chain points where the prop points, whatever the body does; its first bone stays attached to the body. */
	Stage,
};

/**
 * Drives a chain of puppet bones (a tail, a trunk, a tentacle) from a tracked prop, such as a rod a
 * puppeteer holds (design overview §6). The prop is a Live Link Transform-role subject cast into
 * the module's slot, which reads it as a performer with one joint, `prop`; any performer joint can
 * drive the chain instead (a hand). The prop's rotation change since calibration is spread along
 * the chain, by each bone's position along it (0 at the first bone, 1 at the last):
 *
 * - Parent space: bone = ParentMotion * Slerp(I, PropDelta, t) * Rest, where ParentMotion is the
 *   chain parent's rotation change from its rest.
 * - Stage space: bone = Slerp(ParentMotion, PropDelta, t) * Rest.
 *
 * Rigid uses t = 1 for every bone. Amplification and Smoothing apply to the prop's own motion.
 */
UCLASS(meta = (DisplayName = "Prop Chain"))
class OPEN3DPUPPET_API UPuppetModule_PropChain : public UPuppetModule
{
	GENERATED_BODY()

public:
	/** The performer joint that drives the chain: `prop` for a Live Link Transform-role subject, or any canonical joint (hand_l). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Performer", meta = (GetOptions = "GetPerformerJointOptions"))
	FName PerformerJoint = TEXT("prop");

	/** First puppet bone of the chain (its base). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puppet", meta = (GetOptions = "GetPuppetBoneOptions"))
	FName PuppetStart = TEXT("tail_01");

	/** Last puppet bone of the chain (its tip); a descendant of PuppetStart. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puppet", meta = (GetOptions = "GetPuppetBoneOptions"))
	FName PuppetEnd = TEXT("tail_05");

	/** Bend the chain along its length, or turn it as one piece. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Retarget")
	EPuppetPropShape Shape = EPuppetPropShape::Bend;

	/** Whether the prop's motion adds to the body's (Parent) or sets the chain's direction on the stage (Stage). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Retarget")
	EPuppetPropSpace Space = EPuppetPropSpace::Parent;

	/** Scales the prop's rotation: below 1 damps it, above 1 exaggerates it. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Retarget", meta = (ClampMin = "0.0", UIMax = "3.0"))
	float Amplification = 1.0f;

	/** Smooths a noisy track: the time the chain takes to cover about two thirds of a change. 0 follows the prop at once. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Filtering", meta = (ClampMin = "0.0", UIMax = "1.0", Units = "s"))
	float Smoothing = 0.0f;

	virtual TUniquePtr<FPuppetModuleSolver> CreateSolver() const override;
};
