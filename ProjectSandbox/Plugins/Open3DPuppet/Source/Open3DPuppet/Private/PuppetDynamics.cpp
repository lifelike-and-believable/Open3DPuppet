// Copyright 2026 Lifelike & Believable. All Rights Reserved.

#include "PuppetDynamics.h"

#include "PuppetModule.h"
#include "PuppetPerformerPose.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(PuppetDynamics)

namespace PuppetDynamics
{
	namespace
	{
		/** Point pushed to at least Distance from Center; straight up when it sits on the center. */
		FVector PushOut(const FVector& Point, const FVector& Center, double Distance)
		{
			const FVector Offset = Point - Center;
			const double Size = Offset.Size();
			if (Size >= Distance)
			{
				return Point;
			}
			return Center + (Size > UE_KINDA_SMALL_NUMBER ? Offset / Size : FVector::UpVector) * Distance;
		}
	}

	int32 FFixedStep::Advance(float DeltaTime)
	{
		if (StepTime <= 0.0f)
		{
			return 0;
		}
		Accumulated += FMath::Max(DeltaTime, 0.0f);
		const int32 Steps = FMath::FloorToInt32(Accumulated / StepTime);
		if (Steps > MaxSteps)
		{
			Accumulated = FMath::Fmod(Accumulated, StepTime);
			return MaxSteps;
		}
		Accumulated -= static_cast<float>(Steps) * StepTime;
		return Steps;
	}

	float FFixedStep::GetAlpha() const
	{
		return StepTime > 0.0f ? FMath::Clamp(Accumulated / StepTime, 0.0f, 1.0f) : 0.0f;
	}

	void FFixedStep::Reset()
	{
		Accumulated = 0.0f;
	}

	void FAngularSpring::Reset(const FQuat& To)
	{
		Rotation = To.GetNormalized();
		AngularVelocity = FVector::ZeroVector;
	}

	void FAngularSpring::Step(const FQuat& Target, float Frequency, float DampingRatio, float DeltaTime)
	{
		if (DeltaTime <= 0.0f)
		{
			return;
		}
		// The shortest way to the target, so the spring never swings the long way round.
		FQuat Error = (Target * Rotation.Inverse()).GetNormalized();
		if (Error.W < 0.0)
		{
			Error = -Error;
		}
		const double Omega = UE_TWO_PI * Frequency;
		AngularVelocity += (Error.ToRotationVector() * (Omega * Omega) - AngularVelocity * (2.0 * DampingRatio * Omega)) * DeltaTime;
		Rotation = (FQuat::MakeFromRotationVector(AngularVelocity * DeltaTime) * Rotation).GetNormalized();
	}

	FVector Collide(const FVector& Point, double Radius, const FCollisionShapes& Shapes)
	{
		FVector Result = Point;
		for (const FCollisionSphere& Sphere : Shapes.Spheres)
		{
			Result = PushOut(Result, Sphere.Center, Sphere.Radius + Radius);
		}
		for (const FCollisionCapsule& Capsule : Shapes.Capsules)
		{
			Result = PushOut(Result, FMath::ClosestPointOnSegment(Result, Capsule.Start, Capsule.End), Capsule.Radius + Radius);
		}
		if (Shapes.bGround)
		{
			Result.Z = FMath::Max(Result.Z, Shapes.GroundZ + Radius);
		}
		return Result;
	}

	void FDynamicChain::Reset(TConstArrayView<FVector> Pose)
	{
		Positions = Pose;
		PreviousPositions = Pose;
		Lengths.SetNumZeroed(Pose.Num());
		for (int32 Index = 1; Index < Pose.Num(); ++Index)
		{
			Lengths[Index] = FVector::Dist(Pose[Index - 1], Pose[Index]);
		}
	}

	void FDynamicChain::Step(TConstArrayView<FVector> Pose, const FVector& Gravity, float Stiffness, float Damping, float DeltaTime,
		const FCollisionShapes& Shapes, double Radius, int32 Iterations)
	{
		const int32 Num = Positions.Num();
		if (Num == 0 || Pose.Num() != Num)
		{
			return;
		}
		const double Keep = FMath::Clamp(1.0 - static_cast<double>(Damping) * DeltaTime, 0.0, 1.0);
		const FVector Fall = Gravity * (static_cast<double>(DeltaTime) * DeltaTime);
		const double Pull = FMath::Clamp(static_cast<double>(Stiffness), 0.0, 1.0);
		PreviousPositions[0] = Positions[0];
		Positions[0] = Pose[0];
		for (int32 Index = 1; Index < Num; ++Index)
		{
			const FVector Velocity = (Positions[Index] - PreviousPositions[Index]) * Keep;
			PreviousPositions[Index] = Positions[Index];
			Positions[Index] = FMath::Lerp(Positions[Index] + Velocity + Fall, Pose[Index], Pull);
		}
		for (int32 Iteration = 0; Iteration < Iterations; ++Iteration)
		{
			// From the root outwards, each particle back at its segment's length from its parent, then out of the shapes.
			for (int32 Index = 1; Index < Num; ++Index)
			{
				const FVector Segment = Positions[Index] - Positions[Index - 1];
				const double Length = Segment.Size();
				if (Length > UE_KINDA_SMALL_NUMBER)
				{
					Positions[Index] = Positions[Index - 1] + Segment * (Lengths[Index] / Length);
				}
				Positions[Index] = Collide(Positions[Index], Radius, Shapes);
			}
		}
	}

	void FDynamicChain::Transform(const FTransform& Motion)
	{
		for (int32 Index = 0; Index < Positions.Num(); ++Index)
		{
			Positions[Index] = Motion.TransformPosition(Positions[Index]);
			PreviousPositions[Index] = Motion.TransformPosition(PreviousPositions[Index]);
		}
	}

	void FDynamicChain::Interpolate(float Alpha, TArray<FVector>& OutPositions) const
	{
		OutPositions.SetNum(Positions.Num(), EAllowShrinking::No);
		for (int32 Index = 0; Index < Positions.Num(); ++Index)
		{
			OutPositions[Index] = FMath::Lerp(PreviousPositions[Index], Positions[Index], static_cast<double>(Alpha));
		}
	}

	FTransform ComponentMotion(const FTransform& PreviousComponentToWorld, const FTransform& ComponentToWorld)
	{
		return PreviousComponentToWorld.GetRelativeTransform(ComponentToWorld);
	}

	bool BindPhysicsWeight(const FPuppetPhysicsWeight& Weight, const FPuppetPerformerPose& Performer, int32& OutCurve, FString& OutError)
	{
		OutCurve = INDEX_NONE;
		if (Weight.Curve.IsNone())
		{
			return true;
		}
		OutCurve = Performer.FindCurve(Weight.Curve);
		if (OutCurve == INDEX_NONE)
		{
			OutError = FString::Printf(TEXT("the performer has no curve '%s' for the physics weight"), *Weight.Curve.ToString());
			return false;
		}
		return true;
	}

	float GetPhysicsWeight(const FPuppetPhysicsWeight& Weight, int32 Curve, const FPuppetSolveContext& Context)
	{
		return FMath::Clamp(Curve == INDEX_NONE ? Weight.Weight : Context.GetCurveDelta(Curve), 0.0f, 1.0f);
	}
}
