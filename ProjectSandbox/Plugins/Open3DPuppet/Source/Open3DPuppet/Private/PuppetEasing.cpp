// Copyright 2026 Lifelike & Believable. All Rights Reserved.

#include "PuppetEasing.h"

#include "Math/UnrealMathUtility.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(PuppetEasing)

float PuppetEasing::Apply(EPuppetEasing Easing, float Alpha)
{
	return FMath::Clamp(Alpha, 0.0f, 1.0f);
}
