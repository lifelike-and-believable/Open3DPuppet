// Copyright 2026 Lifelike & Believable. All Rights Reserved.

#pragma once

#include "PuppetModule.h"

#include "PuppetModule_Eyes.generated.h"

/** What the Eyes module reads. */
UENUM(BlueprintType)
enum class EPuppetEyeInput : uint8
{
	/** ARKit's eye-look curves (eyeLookInLeft, eyeLookOutLeft, eyeLookUpLeft, eyeLookDownLeft and the Right ones): 1 is the full Max Yaw or Max Pitch. */
	Curves,
	/** The performer's eye joints: their turn relative to the head, clamped to Max Yaw and Max Pitch. */
	Bones,
};

/**
 * Turns the puppet's eyes from the performer's gaze (ADR 0006), as a yaw and a pitch per eye with
 * range limits for the creature. Positive yaw looks to the puppet's left and positive pitch up,
 * about the eye bones' own Yaw Axis and Pitch Axis from their reference rotation.
 */
UCLASS(meta = (DisplayName = "Eyes"))
class OPEN3DPUPPET_API UPuppetModule_Eyes : public UPuppetModule
{
	GENERATED_BODY()

public:
	/** Read the eye-look curves of a face subject, or the eye joints of a performer that has them. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Performer")
	EPuppetEyeInput Input = EPuppetEyeInput::Curves;

	/** The performer's left eye joint, by canonical name (Bones input; map the source's name in the profile). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Performer", meta = (EditCondition = "Input == EPuppetEyeInput::Bones"))
	FName PerformerLeftEye = TEXT("eye_l");

	/** The performer's right eye joint (Bones input). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Performer", meta = (EditCondition = "Input == EPuppetEyeInput::Bones"))
	FName PerformerRightEye = TEXT("eye_r");

	/** The puppet's left eye bone. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puppet")
	FName PuppetLeftEye = TEXT("eye_l");

	/** The puppet's right eye bone. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puppet")
	FName PuppetRightEye = TEXT("eye_r");

	/** The eye bones' axis for looking left and right, in their own space. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puppet")
	FVector YawAxis = FVector::UpVector;

	/** The eye bones' axis for looking up and down, in their own space; positive turns look up. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puppet")
	FVector PitchAxis = FVector(0.0, -1.0, 0.0);

	/** The furthest the eyes turn left or right, in degrees. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Retarget", meta = (ClampMin = "0.0", ClampMax = "90.0", Units = "deg"))
	float MaxYaw = 30.0f;

	/** The furthest the eyes turn up or down, in degrees. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Retarget", meta = (ClampMin = "0.0", ClampMax = "90.0", Units = "deg"))
	float MaxPitch = 20.0f;

	/** How much the eyes share one gaze: 0 each follows its own performer eye, 1 both take the average of the two. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Retarget", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float Convergence = 0.0f;

	virtual TUniquePtr<FPuppetModuleSolver> CreateSolver() const override;
};
