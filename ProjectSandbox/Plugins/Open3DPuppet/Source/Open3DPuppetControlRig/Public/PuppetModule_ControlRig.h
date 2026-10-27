// Copyright 2026 Lifelike & Believable. All Rights Reserved.

#pragma once

#include "ControlRigAssetReference.h"
#include "PuppetModule.h"

#include "PuppetModule_ControlRig.generated.h"

/** What a Control Rig input feeds its control (ADR 0010). */
UENUM(BlueprintType)
enum class EPuppetControlRigInput : uint8
{
	/** The control's rest rotation turned by the performer joint's rotation since calibration. Transform, Euler transform or rotator controls. */
	JointRotation UMETA(DisplayName = "Joint Rotation"),
	/** The joint's rotation, and its move since calibration times the puppet's root scale and Scale. Transform or Euler transform controls. */
	JointTransform UMETA(DisplayName = "Joint Transform"),
	/** The performer curve's change from neutral, times Scale. Float controls. */
	Curve,
};

/** One rig control driven by the performer (ADR 0010). */
USTRUCT(BlueprintType)
struct OPEN3DPUPPETCONTROLRIG_API FPuppetControlRigInput
{
	GENERATED_BODY()

	/** The rig control, by name. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rig")
	FName Control;

	/** What drives it. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rig")
	EPuppetControlRigInput Input = EPuppetControlRigInput::JointRotation;

	/** The performer joint, by canonical name, for the joint inputs. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Performer", meta = (GetOptions = "GetPerformerJointOptions",
		EditCondition = "Input != EPuppetControlRigInput::Curve", EditConditionHides))
	FName PerformerJoint = TEXT("head");

	/** The performer curve, by canonical name, for the Curve input. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Performer", meta = (EditCondition = "Input == EPuppetControlRigInput::Curve", EditConditionHides))
	FName PerformerCurve = TEXT("jawOpen");

	/** Multiplies the joint's move (Joint Transform) or the curve's value (Curve). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Retarget", meta = (EditCondition = "Input != EPuppetControlRigInput::JointRotation", EditConditionHides))
	float Scale = 1.0f;
};

/**
 * Drives part of the puppet with a Control Rig (ADR 0010): the rig starts from the pose the
 * modules before it left, the performer drives the rig's controls, and the rig's forward solve
 * poses the listed bones. The route for technical artists who would rather build a module in the
 * Control Rig editor than in C++. The rig is made on the puppet's mesh, so its bones have the
 * puppet's names; each puppet instance runs its own copy.
 */
UCLASS(meta = (DisplayName = "Control Rig"))
class OPEN3DPUPPETCONTROLRIG_API UPuppetModule_ControlRig : public UPuppetModule
{
	GENERATED_BODY()

public:
	/** The rig. Its controls are set from the performer before each forward solve. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rig", meta = (DisallowCreateNew))
	FControlRigAssetStrongReference Rig;

	/** The rig controls the performer drives. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rig")
	TArray<FPuppetControlRigInput> Inputs;

	/** The puppet bones the rig poses; every other bone is left as the modules before it left it. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puppet", meta = (GetOptions = "GetPuppetBoneOptions"))
	TArray<FName> Bones;

	virtual TUniquePtr<FPuppetModuleSolver> CreateSolver() const override;
};
