// Copyright 2026 Lifelike & Believable. All Rights Reserved.

#include "PuppetCalibrationAsset.h"

#include "Engine/Engine.h"
#include "PuppetCalibrationSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(PuppetCalibrationAsset)

namespace PuppetCalibrationAsset
{
	UPuppetCalibrationSubsystem* Subsystem()
	{
		return GEngine ? GEngine->GetEngineSubsystem<UPuppetCalibrationSubsystem>() : nullptr;
	}
}

void UPuppetCalibrationAsset::Capture()
{
	UPuppetCalibrationSubsystem* Calibrations = PuppetCalibrationAsset::Subsystem();
	FPuppetPerformerCalibration Captured;
	// CalibratePerformer logs why when the subject has no frame.
	if (Calibrations && Calibrations->CalibratePerformer(Subject, Alignment) && Calibrations->GetCalibration(Subject, Captured))
	{
		Modify();
		SetCalibration(Captured);
	}
}

void UPuppetCalibrationAsset::Apply()
{
	UPuppetCalibrationSubsystem* Calibrations = PuppetCalibrationAsset::Subsystem();
	if (Calibrations && HasCalibration())
	{
		Calibrations->SetCalibration(Subject, Calibration);
	}
}

void UPuppetCalibrationAsset::SetCalibration(const FPuppetPerformerCalibration& InCalibration)
{
	Calibration = InCalibration;
	++Revision;
}

bool UPuppetCalibrationAsset::HasCalibration() const
{
	return Calibration.BoneNames.Num() > 0 || Calibration.CurveNames.Num() > 0;
}

#if WITH_EDITOR
void UPuppetCalibrationAsset::PostEditUndo()
{
	Super::PostEditUndo();
	++Revision;
}
#endif
