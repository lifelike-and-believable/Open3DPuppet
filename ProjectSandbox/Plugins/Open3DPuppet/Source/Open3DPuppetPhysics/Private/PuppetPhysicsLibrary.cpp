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
	FPhysicsControlControlAndModifierParameters Parameters;
	for (const FName Region : Regions)
	{
		const float Strength = 1.0f - FMath::Clamp(GetCurve(PuppetPhysics::WeightCurve(Region)), 0.0f, 1.0f);
		const float Damping = 1.0f + FMath::Max(GetCurve(PuppetPhysics::DampingCurve(Region)), 0.0f);

		// Only strength and damping ratio are set; the other multipliers keep what the node has.
		FPhysicsControlSparseMultiplier Multiplier;
		Multiplier.LinearStrengthMultiplier = FVector(Strength);
		Multiplier.AngularStrengthMultiplier = Strength;
		Multiplier.LinearDampingRatioMultiplier = FVector(Damping);
		Multiplier.AngularDampingRatioMultiplier = Damping;
		Multiplier.bEnableLinearExtraDampingMultiplier = false;
		Multiplier.bEnableMaxForceMultiplier = false;
		Multiplier.bEnableAngularExtraDampingMultiplier = false;
		Multiplier.bEnableMaxTorqueMultiplier = false;
		Parameters.Add(FPhysicsControlNamedControlMultiplierParameters(Region, Multiplier));
	}
	return Parameters;
}
