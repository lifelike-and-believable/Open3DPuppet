// Copyright 2026 Lifelike & Believable. All Rights Reserved.

#include "PuppetPhysicsLibrary.h"

#include "Animation/AnimInstance.h"
#include "PuppetModule_PhysicsRegion.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(PuppetPhysicsLibrary)

FPhysicsControlControlAndModifierParameters UPuppetPhysicsLibrary::MakeRegionParameters(const UAnimInstance* AnimInstance, const TArray<FName>& Regions)
{
	return MakeRegionParametersFromCurves(Regions, [AnimInstance](FName Curve)
	{
		float Value = 0.0f;
		if (AnimInstance)
		{
			AnimInstance->GetCurveValueWithDefault(Curve, 0.0f, Value);
		}
		return Value;
	});
}

FPhysicsControlControlAndModifierParameters UPuppetPhysicsLibrary::MakeRegionParametersFromCurves(TConstArrayView<FName> Regions, TFunctionRef<float(FName)> GetCurve)
{
	return {};
}
