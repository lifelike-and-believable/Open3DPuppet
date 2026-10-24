// Copyright 2026 Lifelike & Believable. All Rights Reserved.

#pragma once

#include "Containers/Array.h"
#include "Math/Quat.h"
#include "Math/Vector.h"

/**
 * The retargeting operations every puppet module builds on (design overview §4). Modules compose
 * these instead of reimplementing them, so a fix here reaches the whole library.
 */
namespace PuppetRetarget
{
	/**
	 * A performer joint's component-space rotation change since calibration, expressed in the
	 * puppet's component space: Alignment * (Current * Rest^-1) * Alignment^-1. Applied to a puppet
	 * bone's rest rotation (Delta * PuppetRest), it reproduces the performer's motion whatever
	 * either skeleton's bone axes are.
	 */
	OPEN3DPUPPET_API FQuat RotationDelta(const FQuat& Current, const FQuat& Rest, const FQuat& Alignment);

	/** Scales a rotation's angle about its own axis: 0 is identity, 1 is Delta, 2 doubles it. */
	OPEN3DPUPPET_API FQuat ScaleRotation(const FQuat& Delta, double Amount);

	/**
	 * Normalized position of each joint along a chain, by distance along the chain: the first
	 * joint is 0 and the last 1. A chain of zero length is spaced evenly by joint count.
	 */
	OPEN3DPUPPET_API void ChainPositions(TConstArrayView<FVector> JointPositions, TArray<float>& OutPositions);

	/** Total length of the segments between consecutive joints. */
	OPEN3DPUPPET_API double ChainLength(TConstArrayView<FVector> JointPositions);

	/**
	 * The rotation at normalized position T along a chain whose joints sit at Positions (from
	 * ChainPositions) and carry Rotations, interpolated (slerp) between the neighbouring joints and
	 * clamped to the ends.
	 */
	OPEN3DPUPPET_API FQuat SampleChain(TConstArrayView<FQuat> Rotations, TConstArrayView<float> Positions, float T);

	/**
	 * Fills OutChain with the bones from Start down to End, Start first, following Parents (one
	 * entry per bone, INDEX_NONE for a root). False and OutChain empty when End is not Start or one
	 * of its descendants.
	 */
	OPEN3DPUPPET_API bool CollectChain(TConstArrayView<int32> Parents, int32 Start, int32 End, TArray<int32>& OutChain);
}
