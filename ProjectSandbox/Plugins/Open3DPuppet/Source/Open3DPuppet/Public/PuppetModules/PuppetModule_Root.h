// Copyright 2026 Lifelike & Believable. All Rights Reserved.

#pragma once

#include "PuppetModule.h"

#include "PuppetModule_Root.generated.h"

/**
 * Drives the puppet's pelvis from the performer's pelvis: rotation by rotation delta, position by
 * the performer's movement since calibration, scaled by the puppet's size (design overview §4).
 * Runs first, on the root slot (ADR 0004).
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

	virtual TUniquePtr<FPuppetModuleSolver> CreateSolver() const override;
};
