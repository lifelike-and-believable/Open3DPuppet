// Copyright 2026 Lifelike & Believable. All Rights Reserved.

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "PhysicsControlData.h"
#include "Templates/Function.h"

#include "PuppetPhysicsLibrary.generated.h"

class UAnimInstance;

/** Turns Physics Region curves into Rigid Body With Control parameters (ADR 0007, Tier 2). */
UCLASS()
class OPEN3DPUPPETPHYSICS_API UPuppetPhysicsLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * The control multipliers for each region, from the Physics Region curves the puppet's Puppet
	 * Solve published: connect the result to Rigid Body With Control's Control And Modifier
	 * Parameters pin. Each region's strength is multiplied by 1 - its physics weight and its damping
	 * ratio by 1 + its extra damping; a region with no curves keeps its authored values. The anim
	 * instance's curves are those of its last evaluation, so the node sees them a frame late.
	 *
	 * @param AnimInstance The animation instance that runs Puppet Solve (Self in its AnimGraph).
	 * @param Regions The regions, named as the node's limbs (Tail, LeftFrontLeg, Body).
	 */
	UFUNCTION(BlueprintPure, Category = "Open3DPuppet|Physics", meta = (BlueprintThreadSafe, DisplayName = "Make Physics Region Parameters"))
	static FPhysicsControlControlAndModifierParameters MakeRegionParameters(const UAnimInstance* AnimInstance, const TArray<FName>& Regions);

	/** MakeRegionParameters with the curves read through GetCurve, which returns 0 for a curve it lacks. */
	static FPhysicsControlControlAndModifierParameters MakeRegionParametersFromCurves(TConstArrayView<FName> Regions, TFunctionRef<float(FName)> GetCurve);
};
