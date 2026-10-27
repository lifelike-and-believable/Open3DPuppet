// Copyright 2026 Lifelike & Believable. All Rights Reserved.

#include "PuppetCalibrationAsset.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(PuppetCalibrationAsset)

void UPuppetCalibrationAsset::Capture()
{
}

void UPuppetCalibrationAsset::Apply()
{
}

void UPuppetCalibrationAsset::SetCalibration(const FPuppetPerformerCalibration& InCalibration)
{
}

bool UPuppetCalibrationAsset::HasCalibration() const
{
	return false;
}

#if WITH_EDITOR
void UPuppetCalibrationAsset::PostEditUndo()
{
	Super::PostEditUndo();
}
#endif
