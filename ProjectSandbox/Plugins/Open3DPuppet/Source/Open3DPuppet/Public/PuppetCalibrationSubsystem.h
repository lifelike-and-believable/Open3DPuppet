// Copyright 2026 Lifelike & Believable. All Rights Reserved.

#pragma once

#include "LiveLinkTypes.h"
#include "PuppetPerformerCalibration.h"
#include "Subsystems/EngineSubsystem.h"

#include "PuppetCalibrationSubsystem.generated.h"

/**
 * Keeps each performer's calibration, by Live Link subject name (ADR 0003). An engine subsystem,
 * so calibrating works in the editor viewport as well as in game. Game thread only; Puppet Solve
 * nodes copy what they need in PreUpdate.
 */
UCLASS()
class OPEN3DPUPPET_API UPuppetCalibrationSubsystem : public UEngineSubsystem
{
	GENERATED_BODY()

public:
	/**
	 * Captures the subject's current pose as its rest pose. Have the performer stand in the
	 * reference pose (T-pose) first. Alignment turns the performer's facing into the puppet's.
	 * Returns false (and logs why) when the subject has no Animation-role frame.
	 */
	UFUNCTION(BlueprintCallable, Category = "Puppet|Calibration")
	bool CalibratePerformer(FLiveLinkSubjectName Subject, FRotator Alignment);

	/** Sets a subject's calibration, for example one saved earlier. */
	UFUNCTION(BlueprintCallable, Category = "Puppet|Calibration")
	void SetCalibration(FLiveLinkSubjectName Subject, const FPuppetPerformerCalibration& Calibration);

	/** The subject's calibration; false when it has none. */
	UFUNCTION(BlueprintCallable, Category = "Puppet|Calibration")
	bool GetCalibration(FLiveLinkSubjectName Subject, FPuppetPerformerCalibration& OutCalibration) const;

	/** Forgets the subject's calibration; puppets it drives stop following it. */
	UFUNCTION(BlueprintCallable, Category = "Puppet|Calibration")
	void ClearCalibration(FLiveLinkSubjectName Subject);

	/** The subjects that have a calibration, for the session panel. */
	TArray<FName> GetCalibratedSubjects() const;

	/** The subject's calibration or null, and a number that changes whenever it changes (0 when there is none). */
	const FPuppetPerformerCalibration* FindCalibration(FName Subject, int32& OutRevision) const;

private:
	struct FEntry
	{
		FPuppetPerformerCalibration Calibration;
		int32 Revision = 0;
	};

	TMap<FName, FEntry> Calibrations;
	int32 LastRevision = 0;
};
