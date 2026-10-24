// Copyright 2026 Lifelike & Believable. All Rights Reserved.

#pragma once

#include "PuppetEasing.h"
#include "PuppetModule.h"

#include "PuppetModule_Root.generated.h"

/**
 * Drives the puppet's pelvis from the performer's pelvis: rotation by rotation delta, position by
 * the performer's movement since calibration, scaled by the puppet's size (design overview §4).
 * Runs first, on the root slot (ADR 0004). On the root slot it also owns the hand-off offset
 * (ADR 0008): it captures it at a hand-off or re-centre, then converges it on the performer.
 */
UCLASS(meta = (DisplayName = "Root (Pelvis)"))
class OPEN3DPUPPET_API UPuppetModule_Root : public UPuppetModule
{
	GENERATED_BODY()

public:
	/** Follow the performer's pelvis rotation. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Root")
	bool bDriveRotation = true;

	/** Follow the performer's movement across and above the stage. Only the root slot may move the puppet. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Root")
	bool bDriveTranslation = true;

	/** Multiplies the root scale (puppet pelvis height over performer pelvis height) for movement across the stage. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Root", meta = (ClampMin = "0.0"))
	float HorizontalScale = 1.0f;

	/** Multiplies the root scale for vertical movement (crouches, jumps). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Root", meta = (ClampMin = "0.0"))
	float VerticalScale = 1.0f;

	/** Added to the pelvis height, in centimetres. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Root")
	float HeightOffset = 0.0f;

	/**
	 * After a hand-off or a re-centre, the time in seconds over which the puppet moves from where the
	 * animation left it to where the performer puts it (ADR 0008). 0 snaps to the performer at once.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hand-off", meta = (ClampMin = "0.0", Units = "s"))
	float ConvergenceTime = 2.0f;

	/** Converge only while the performer moves, so the correction hides in their motion and a standing puppet never slides. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hand-off")
	bool bConvergeOnlyWhileMoving = true;

	/** Performer speed (cm/s, in puppet space) at which convergence runs at its full rate. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hand-off", meta = (ClampMin = "1.0", EditCondition = "bConvergeOnlyWhileMoving"))
	float MovingSpeed = 50.0f;

	/** Performer turn rate (degrees per second) at which convergence runs at its full rate. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hand-off", meta = (ClampMin = "1.0", EditCondition = "bConvergeOnlyWhileMoving"))
	float MovingTurnRate = 45.0f;

	/** How the offset blends out over the convergence time. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hand-off")
	EPuppetEasing ConvergenceEasing = EPuppetEasing::EaseInOut;

	/** Keep the hand-off offset instead of converging: the puppet stays offset from the performer. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hand-off")
	bool bHoldOffset = false;

	virtual TUniquePtr<FPuppetModuleSolver> CreateSolver() const override;
};
