// Copyright 2026 Lifelike & Believable. All Rights Reserved.

#pragma once

#include "Engine/DataAsset.h"
#include "LiveLinkTypes.h"
#include "PuppetPerformerCalibration.h"

#include "PuppetCalibrationAsset.generated.h"

/**
 * A performer's calibration saved in an asset (ADR 0003), so a show can start without calibrating
 * again. Capture records a Live Link subject's current pose; Apply makes the saved calibration the
 * subject's current one. A Puppet Solve cast entry can name the asset as its saved calibration,
 * used while the subject has no calibration in the Puppet Calibration subsystem.
 */
UCLASS(BlueprintType)
class OPEN3DPUPPET_API UPuppetCalibrationAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	/** The Live Link subject Capture reads and Apply calibrates. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Calibration")
	FLiveLinkSubjectName Subject;

	/** Rotation from the performer's facing to the puppet's, recorded by Capture (for example a yaw when the performer faces another way). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Calibration")
	FRotator Alignment = FRotator::ZeroRotator;

	/**
	 * Captures Subject's current pose as its rest pose, with Alignment, saves it in this asset and
	 * makes it the subject's calibration. Have the performer stand in the reference pose (T-pose)
	 * first. When the subject has no frame, logs why and keeps the saved calibration.
	 */
	UFUNCTION(CallInEditor, BlueprintCallable, Category = "Calibration")
	void Capture();

	/** Makes the saved calibration Subject's calibration in the Puppet Calibration subsystem. Does nothing when nothing is saved. */
	UFUNCTION(CallInEditor, BlueprintCallable, Category = "Calibration")
	void Apply();

	/** The saved calibration. */
	UFUNCTION(BlueprintPure, Category = "Calibration")
	const FPuppetPerformerCalibration& GetCalibration() const { return Calibration; }

	/** Replaces the saved calibration. */
	UFUNCTION(BlueprintCallable, Category = "Calibration")
	void SetCalibration(const FPuppetPerformerCalibration& InCalibration);

	/** True when a calibration is saved (bones or curves). */
	UFUNCTION(BlueprintPure, Category = "Calibration")
	bool HasCalibration() const;

	/** A number that changes whenever the saved calibration changes, so Puppet Solve nodes notice. */
	int32 GetRevision() const { return Revision; }

#if WITH_EDITOR
	/** Undoing a capture changes the saved calibration too. */
	virtual void PostEditUndo() override;
#endif

private:
	/** Written by Capture and SetCalibration; edit it there rather than by hand. */
	UPROPERTY(VisibleAnywhere, Category = "Calibration")
	FPuppetPerformerCalibration Calibration;

	int32 Revision = 0;
};
