// Copyright 2026 Lifelike & Believable. All Rights Reserved.

#include "PuppetDynamics.h"

#include "PuppetModule.h"
#include "PuppetPerformerPose.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(PuppetDynamics)

namespace PuppetDynamics
{
	int32 FFixedStep::Advance(float DeltaTime)
	{
		return 0;
	}

	float FFixedStep::GetAlpha() const
	{
		return 0.0f;
	}

	void FFixedStep::Reset()
	{
		Accumulated = 0.0f;
	}

	void FAngularSpring::Reset(const FQuat& To)
	{
	}

	void FAngularSpring::Step(const FQuat& Target, float Frequency, float DampingRatio, float DeltaTime)
	{
	}

	FVector Collide(const FVector& Point, double Radius, const FCollisionShapes& Shapes)
	{
		return Point;
	}

	void FDynamicChain::Reset(TConstArrayView<FVector> Pose)
	{
		Positions = Pose;
		PreviousPositions = Pose;
		Lengths.SetNumZeroed(Pose.Num());
	}

	void FDynamicChain::Step(TConstArrayView<FVector> Pose, const FVector& Gravity, float Stiffness, float Damping, float DeltaTime,
		const FCollisionShapes& Shapes, double Radius, int32 Iterations)
	{
	}

	void FDynamicChain::Transform(const FTransform& Motion)
	{
	}

	void FDynamicChain::Interpolate(float Alpha, TArray<FVector>& OutPositions) const
	{
		OutPositions = Positions;
	}

	FTransform ComponentMotion(const FTransform& PreviousComponentToWorld, const FTransform& ComponentToWorld)
	{
		return FTransform::Identity;
	}

	bool BindPhysicsWeight(const FPuppetPhysicsWeight& Weight, const FPuppetPerformerPose& Performer, int32& OutCurve, FString& OutError)
	{
		OutCurve = INDEX_NONE;
		return true;
	}

	float GetPhysicsWeight(const FPuppetPhysicsWeight& Weight, int32 Curve, const FPuppetSolveContext& Context)
	{
		return 0.0f;
	}
}
