// Copyright 2026 Lifelike & Believable. All Rights Reserved.

#include "PuppetEasing.h"

#include "Math/UnrealMathUtility.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(PuppetEasing)

float PuppetEasing::Apply(EPuppetEasing Easing, float Alpha)
{
	const float Clamped = FMath::Clamp(Alpha, 0.0f, 1.0f);
	switch (Easing)
	{
	case EPuppetEasing::EaseIn: return FMath::InterpEaseIn(0.0f, 1.0f, Clamped, 2.0f);
	case EPuppetEasing::EaseOut: return FMath::InterpEaseOut(0.0f, 1.0f, Clamped, 2.0f);
	case EPuppetEasing::EaseInOut: return FMath::InterpEaseInOut(0.0f, 1.0f, Clamped, 2.0f);
	default: return Clamped;
	}
}
