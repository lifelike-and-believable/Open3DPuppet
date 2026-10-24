// Copyright 2026 Lifelike & Believable. All Rights Reserved.

#pragma once

#include "UObject/ObjectMacros.h"

#include "PuppetEasing.generated.h"

/** The shape of a blend over its duration (ADR 0008). */
UENUM(BlueprintType)
enum class EPuppetEasing : uint8
{
	Linear,
	EaseIn UMETA(DisplayName = "Ease In"),
	EaseOut UMETA(DisplayName = "Ease Out"),
	EaseInOut UMETA(DisplayName = "Ease In Out"),
};

namespace PuppetEasing
{
	/** Alpha (clamped to 0..1) shaped by Easing; 0 and 1 map to themselves. The eases are quadratic. */
	OPEN3DPUPPET_API float Apply(EPuppetEasing Easing, float Alpha);
}
