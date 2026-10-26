// Copyright 2026 Lifelike & Believable. All Rights Reserved.

#include "PuppetClipSampler.h"

#include "Animation/AnimSequence.h"
#include "PuppetPerformerCalibration.h"

bool FPuppetClipSampler::SetSequence(const UAnimSequence* InSequence, FString& OutError)
{
	Sequence = nullptr;
	return false;
}

bool FPuppetClipSampler::Sample(double Time, TArray<FTransform>& OutLocal) const
{
	return false;
}

bool FPuppetClipSampler::CaptureCalibration(bool bReferencePose, double Time, const FRotator& Alignment, FPuppetPerformerCalibration& OutCalibration) const
{
	return false;
}

double FPuppetClipSampler::AdvanceTime(double Time, double DeltaTime, float PlayRate, bool bLoop, double Length)
{
	return Time;
}

double FPuppetClipSampler::PlaceTime(double Time, bool bLoop, double Length)
{
	return Time;
}
