// Copyright 2026 Lifelike & Believable. All Rights Reserved.

#pragma once

#include "Containers/Array.h"
#include "Containers/ArrayView.h"
#include "Containers/UnrealString.h"
#include "Math/Quat.h"
#include "Math/Transform.h"
#include "Math/Vector.h"
#include "UObject/NameTypes.h"
#include "UObject/ObjectMacros.h"

#include "PuppetDynamics.generated.h"

class FPuppetPerformerPose;
struct FPuppetSolveContext;

/**
 * How much a dynamic module simulates (ADR 0007): 0 follows the retargeted pose, 1 is fully
 * simulated. A performer curve can set it instead of the fixed value, so a puppeteer plays it live.
 */
USTRUCT(BlueprintType)
struct OPEN3DPUPPET_API FPuppetPhysicsWeight
{
	GENERATED_BODY()

	/** The weight when no curve is named: 0 follows the retargeted pose, 1 is fully simulated. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dynamics", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float Weight = 1.0f;

	/** A performer curve, by canonical name, whose change from neutral (clamped to 0 to 1) sets the weight instead. None uses Weight. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dynamics")
	FName Curve;
};

/**
 * The building blocks of dynamic puppet modules (ADR 0007, Tier 1), as PuppetRetarget is for
 * retargeting: deterministic, so a re-solved take moves the same way. Positions are in the
 * puppet's component space, times in seconds, lengths in centimetres.
 */
namespace PuppetDynamics
{
	/**
	 * Runs a simulation at a fixed rate whatever the frame rate. Each frame, Advance says how many
	 * steps to run; GetAlpha says how far between the last two steps the frame falls, for
	 * interpolating the output.
	 */
	struct OPEN3DPUPPET_API FFixedStep
	{
		/** Seconds per step. */
		float StepTime = 1.0f / 120.0f;
		/** The most steps one frame runs; time beyond them is dropped, so a long hitch does not stall the frame. */
		int32 MaxSteps = 8;

		/** Adds the frame's time and returns the number of steps to run now. */
		int32 Advance(float DeltaTime);

		/** Time left over after the steps, as a fraction of a step (0 to 1). */
		float GetAlpha() const;

		void Reset();

	private:
		float Accumulated = 0.0f;
	};

	/**
	 * A rotation that follows a target like a spring with a damper: Frequency (Hz) sets the
	 * stiffness, DampingRatio the damping (1 settles without overshoot, below 1 swings). Semi-implicit
	 * Euler, stable while 2 pi Frequency DeltaTime stays below 2.
	 */
	struct OPEN3DPUPPET_API FAngularSpring
	{
		FQuat Rotation = FQuat::Identity;
		/** Radians per second, as a rotation vector. */
		FVector AngularVelocity = FVector::ZeroVector;

		/** Puts the spring at rest at To. */
		void Reset(const FQuat& To);

		/** Advances one step of DeltaTime towards Target. */
		void Step(const FQuat& Target, float Frequency, float DampingRatio, float DeltaTime);
	};

	/** A sphere in component space. */
	struct FCollisionSphere
	{
		FVector Center = FVector::ZeroVector;
		double Radius = 0.0;
	};

	/** A capsule (a segment with a radius) in component space. */
	struct FCollisionCapsule
	{
		FVector Start = FVector::ZeroVector;
		FVector End = FVector::ZeroVector;
		double Radius = 0.0;
	};

	/** What dynamic chains keep out of: spheres and capsules on the puppet's bones, and the ground. */
	struct FCollisionShapes
	{
		TArray<FCollisionSphere> Spheres;
		TArray<FCollisionCapsule> Capsules;
		bool bGround = false;
		/** The ground's height in component space. */
		double GroundZ = 0.0;
	};

	/** Point pushed out of every shape, so a particle of Radius only touches them. */
	OPEN3DPUPPET_API FVector Collide(const FVector& Point, double Radius, const FCollisionShapes& Shapes);

	/**
	 * A chain of particles with fixed segment lengths, its first particle pinned to a bone (Verlet
	 * integration). Ears, a mane, feathers and tails are such chains.
	 */
	struct OPEN3DPUPPET_API FDynamicChain
	{
		/** Particle positions now, and one step ago. */
		TArray<FVector> Positions;
		TArray<FVector> PreviousPositions;
		/** Rest length of the segment ending at each particle; Lengths[0] is unused. */
		TArray<double> Lengths;

		/** Sizes the chain for Pose (component space) and puts it there, at rest; segment lengths come from Pose. Allocates. */
		void Reset(TConstArrayView<FVector> Pose);

		/**
		 * One step of DeltaTime. The first particle moves to Pose[0]; every other particle keeps its
		 * velocity, less Damping (0 to 1, per second), accelerates by Gravity and is pulled towards
		 * Pose by Stiffness (0 to 1: 1 holds the pose). Then, Iterations times, segment lengths are
		 * restored from the root outwards and particles of Radius are pushed out of Shapes.
		 */
		void Step(TConstArrayView<FVector> Pose, const FVector& Gravity, float Stiffness, float Damping, float DeltaTime,
			const FCollisionShapes& Shapes, double Radius, int32 Iterations);

		/** Moves the whole state by Motion, for example ComponentMotion, so it keeps its world position as the puppet moves. */
		void Transform(const FTransform& Motion);

		/** The particle positions between the last two steps, at Alpha (FFixedStep::GetAlpha). */
		void Interpolate(float Alpha, TArray<FVector>& OutPositions) const;
	};

	/**
	 * Maps a point in last frame's component space to this frame's, from the component's world
	 * transforms (FPuppetSolveContext). Applied to a dynamic state, it keeps the state where it was
	 * in the world, so the simulation feels the puppet move and turn.
	 */
	OPEN3DPUPPET_API FTransform ComponentMotion(const FTransform& PreviousComponentToWorld, const FTransform& ComponentToWorld);

	/** Resolves Weight's curve; INDEX_NONE when it names none. False, with OutError, when the performer lacks the curve. */
	OPEN3DPUPPET_API bool BindPhysicsWeight(const FPuppetPhysicsWeight& Weight, const FPuppetPerformerPose& Performer, int32& OutCurve, FString& OutError);

	/** This frame's physics weight: the curve's change from neutral clamped to 0 to 1, or the fixed weight. */
	OPEN3DPUPPET_API float GetPhysicsWeight(const FPuppetPhysicsWeight& Weight, int32 Curve, const FPuppetSolveContext& Context);
}
