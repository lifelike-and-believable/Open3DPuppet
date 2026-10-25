// Copyright 2026 Lifelike & Believable. All Rights Reserved.

// PuppetDynamics: the building blocks of dynamic modules (ADR 0007, Tier 1). Step times are powers
// of two (1/128 s), so frame times add up exactly and runs at different frame rates can be compared.

#include "PuppetTestHarness.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Harness/PuppetTestFixtures.h"
#include "PuppetDynamics.h"
#include "PuppetModule.h"
#include "PuppetPerformerCalibration.h"
#include "PuppetPerformerPose.h"
#include "PuppetPose.h"

namespace PuppetDynamicsTests
{
	constexpr float Step = 1.0f / 128.0f;
	const FVector Gravity(0.0, 0.0, -980.0);

	/** A horizontal chain along X from (0, 0, Z): four particles 10 cm apart. */
	TArray<FVector> Horizontal(double Z)
	{
		return { FVector(0, 0, Z), FVector(10, 0, Z), FVector(20, 0, Z), FVector(30, 0, Z) };
	}

	/** Runs Steps steps of the spring towards Target and returns the largest turn about Z it reached, in degrees. */
	double RunSpring(PuppetDynamics::FAngularSpring& Spring, const FQuat& Target, float Frequency, float DampingRatio, int32 Steps)
	{
		double Largest = 0.0;
		for (int32 Index = 0; Index < Steps; ++Index)
		{
			Spring.Step(Target, Frequency, DampingRatio, Step);
			Largest = FMath::Max(Largest, FMath::RadiansToDegrees(Spring.Rotation.GetTwistAngle(FVector::ZAxisVector)));
		}
		return Largest;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetFixedStepTest, "Open3DPuppet.Dynamics.FixedStep.StepsAtItsOwnRate", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetFixedStepTest::RunTest(const FString& Parameters)
{
	PuppetDynamics::FFixedStep Stepper;
	Stepper.StepTime = PuppetDynamicsTests::Step;
	TestEqual(TEXT("A 1/32 s frame is four 1/128 s steps"), Stepper.Advance(1.0f / 32.0f), 4);
	TestNearlyEqual(TEXT("Nothing left over"), Stepper.GetAlpha(), 0.0f);
	TestEqual(TEXT("Half a step runs none"), Stepper.Advance(1.0f / 256.0f), 0);
	TestNearlyEqual(TEXT("Halfway to the next step"), Stepper.GetAlpha(), 0.5f);
	TestEqual(TEXT("A one-second hitch runs at most MaxSteps"), Stepper.Advance(1.0f), Stepper.MaxSteps);
	TestTrue(TEXT("And drops the rest"), Stepper.GetAlpha() < 1.0f);
	Stepper.Reset();
	TestNearlyEqual(TEXT("Reset"), Stepper.GetAlpha(), 0.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetSpringCriticalTest, "Open3DPuppet.Dynamics.AngularSpring.CriticalDampingSettlesWithoutOvershoot", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetSpringCriticalTest::RunTest(const FString& Parameters)
{
	// 2 Hz, critically damped, towards a 90 degree turn: after 3 s the error (1 + wt) e^-wt of 90
	// degrees, with w = 4 pi, is far below a tenth of a degree.
	PuppetDynamics::FAngularSpring Spring;
	Spring.Reset(FQuat::Identity);
	const FQuat Target = PuppetTests::Rotation(FVector::ZAxisVector, 90.0);
	const double Largest = PuppetDynamicsTests::RunSpring(Spring, Target, 2.0f, 1.0f, 3 * 128);
	TestTrue(TEXT("No overshoot"), Largest < 90.5);
	PuppetTests::TestRotation(*this, TEXT("Settled"), Spring.Rotation, Target, 1.e-3);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetSpringUnderdampedTest, "Open3DPuppet.Dynamics.AngularSpring.UnderdampingSwingsThenSettles", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetSpringUnderdampedTest::RunTest(const FString& Parameters)
{
	// Damping ratio 0.2 swings past the target (about 50% overshoot), then decays as e^-0.2 w t.
	PuppetDynamics::FAngularSpring Spring;
	Spring.Reset(FQuat::Identity);
	const FQuat Target = PuppetTests::Rotation(FVector::ZAxisVector, 90.0);
	const double Largest = PuppetDynamicsTests::RunSpring(Spring, Target, 2.0f, 0.2f, 10 * 128);
	TestTrue(TEXT("Swings past the target"), Largest > 100.0);
	PuppetTests::TestRotation(*this, TEXT("Settled after 10 s"), Spring.Rotation, Target, 1.e-3);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetSpringFrameRateTest, "Open3DPuppet.Dynamics.AngularSpring.SameMotionAtAnyFrameRate", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetSpringFrameRateTest::RunTest(const FString& Parameters)
{
	// One second at 32 frames per second and at 64, both stepping at 128 Hz: the same 128 steps.
	const FQuat Target = PuppetTests::Rotation(FVector::ZAxisVector, 90.0);
	FQuat Results[2];
	const float FrameTimes[2] = { 1.0f / 32.0f, 1.0f / 64.0f };
	for (int32 Run = 0; Run < 2; ++Run)
	{
		PuppetDynamics::FFixedStep Stepper;
		Stepper.StepTime = PuppetDynamicsTests::Step;
		PuppetDynamics::FAngularSpring Spring;
		Spring.Reset(FQuat::Identity);
		for (float Time = 0.0f; Time < 1.0f; Time += FrameTimes[Run])
		{
			for (int32 Steps = Stepper.Advance(FrameTimes[Run]); Steps > 0; --Steps)
			{
				Spring.Step(Target, 2.0f, 0.3f, Stepper.StepTime);
			}
		}
		Results[Run] = Spring.Rotation;
	}
	TestTrue(TEXT("Still moving after one second"), !Results[0].Equals(Target, 1.e-3));
	PuppetTests::TestRotation(*this, TEXT("32 and 64 frames per second agree"), Results[1], Results[0], 1.e-6);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetCollideTest, "Open3DPuppet.Dynamics.Collide.PushesOutOfShapes", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetCollideTest::RunTest(const FString& Parameters)
{
	PuppetDynamics::FCollisionShapes Sphere;
	Sphere.Spheres.Add({ FVector::ZeroVector, 10.0 });
	TestEqual(TEXT("Out of a sphere, by the particle's radius"), PuppetDynamics::Collide(FVector(1, 0, 0), 1.0, Sphere), FVector(11, 0, 0), 1.e-4f);
	TestEqual(TEXT("Outside it, untouched"), PuppetDynamics::Collide(FVector(20, 0, 0), 1.0, Sphere), FVector(20, 0, 0), 1.e-4f);

	PuppetDynamics::FCollisionShapes Capsule;
	Capsule.Capsules.Add({ FVector(0, 0, 0), FVector(0, 0, 10), 2.0 });
	TestEqual(TEXT("Out of a capsule, sideways"), PuppetDynamics::Collide(FVector(1, 0, 5), 0.0, Capsule), FVector(2, 0, 5), 1.e-4f);

	PuppetDynamics::FCollisionShapes Ground;
	Ground.bGround = true;
	TestEqual(TEXT("Above the ground"), PuppetDynamics::Collide(FVector(0, 0, -3), 1.0, Ground), FVector(0, 0, 1), 1.e-4f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetChainStiffTest, "Open3DPuppet.Dynamics.DynamicChain.FullStiffnessHoldsThePose", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetChainStiffTest::RunTest(const FString& Parameters)
{
	const TArray<FVector> Pose = PuppetDynamicsTests::Horizontal(100.0);
	PuppetDynamics::FDynamicChain Chain;
	Chain.Reset(Pose);
	for (int32 Index = 0; Index < 128; ++Index)
	{
		Chain.Step(Pose, PuppetDynamicsTests::Gravity, 1.0f, 0.0f, PuppetDynamicsTests::Step, {}, 0.0, 4);
	}
	TestEqual(TEXT("Tip held against gravity"), Chain.Positions[3], Pose[3], 1.e-3f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetChainHangTest, "Open3DPuppet.Dynamics.DynamicChain.HangsUnderGravityAndKeepsLengths", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetChainHangTest::RunTest(const FString& Parameters)
{
	// No stiffness: the horizontal chain swings down and, damped, hangs 30 cm straight below its root.
	const TArray<FVector> Pose = PuppetDynamicsTests::Horizontal(100.0);
	PuppetDynamics::FDynamicChain Chain;
	Chain.Reset(Pose);
	for (int32 Index = 0; Index < 10 * 128; ++Index)
	{
		Chain.Step(Pose, PuppetDynamicsTests::Gravity, 0.0f, 2.0f, PuppetDynamicsTests::Step, {}, 0.0, 4);
	}
	TestEqual(TEXT("Root pinned"), Chain.Positions[0], Pose[0], 1.e-3f);
	TestEqual(TEXT("Tip below the root"), Chain.Positions[3], FVector(0, 0, 70), 0.5f);
	for (int32 Index = 1; Index < 4; ++Index)
	{
		TestNearlyEqual(*FString::Printf(TEXT("Segment %d keeps its length"), Index), FVector::Dist(Chain.Positions[Index - 1], Chain.Positions[Index]), 10.0, 1.e-2);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetChainGroundTest, "Open3DPuppet.Dynamics.DynamicChain.StaysAboveTheGround", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetChainGroundTest::RunTest(const FString& Parameters)
{
	// Pinned 5 cm above the ground, a 30 cm chain cannot hang straight down: it lies on the ground.
	const TArray<FVector> Pose = PuppetDynamicsTests::Horizontal(5.0);
	PuppetDynamics::FCollisionShapes Shapes;
	Shapes.bGround = true;
	PuppetDynamics::FDynamicChain Chain;
	Chain.Reset(Pose);
	for (int32 Index = 0; Index < 5 * 128; ++Index)
	{
		Chain.Step(Pose, PuppetDynamicsTests::Gravity, 0.0f, 2.0f, PuppetDynamicsTests::Step, Shapes, 1.0, 4);
	}
	for (int32 Index = 1; Index < 4; ++Index)
	{
		TestTrue(*FString::Printf(TEXT("Particle %d above the ground"), Index), Chain.Positions[Index].Z >= 1.0 - 1.e-3);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetComponentMotionTest, "Open3DPuppet.Dynamics.ComponentMotion.KeepsTheWorldPosition", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetComponentMotionTest::RunTest(const FString& Parameters)
{
	// The puppet steps 100 cm along X: a particle that stays put in the world is 100 cm further
	// back in component space. The puppet turns 90 degrees left: a particle 10 cm ahead is now
	// 10 cm to its right (-Y).
	const FTransform Stepped = PuppetDynamics::ComponentMotion(FTransform::Identity, FTransform(FVector(100, 0, 0)));
	TestEqual(TEXT("Step"), Stepped.TransformPosition(FVector::ZeroVector), FVector(-100, 0, 0), 1.e-4f);
	const FTransform Turned = PuppetDynamics::ComponentMotion(FTransform::Identity, FTransform(PuppetTests::Rotation(FVector::ZAxisVector, 90.0)));
	TestEqual(TEXT("Turn"), Turned.TransformPosition(FVector(10, 0, 0)), FVector(0, -10, 0), 1.e-4f);

	PuppetDynamics::FDynamicChain Chain;
	Chain.Reset(PuppetDynamicsTests::Horizontal(0.0));
	Chain.Transform(Stepped);
	TestEqual(TEXT("Chain moved"), Chain.Positions[1], FVector(-90, 0, 0), 1.e-4f);
	TestEqual(TEXT("With its last step"), Chain.PreviousPositions[1], FVector(-90, 0, 0), 1.e-4f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetChainInterpolateTest, "Open3DPuppet.Dynamics.DynamicChain.InterpolatesBetweenSteps", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetChainInterpolateTest::RunTest(const FString& Parameters)
{
	const TArray<FVector> Pose = PuppetDynamicsTests::Horizontal(100.0);
	PuppetDynamics::FDynamicChain Chain;
	Chain.Reset(Pose);
	Chain.Step(Pose, PuppetDynamicsTests::Gravity, 0.0f, 0.0f, PuppetDynamicsTests::Step, {}, 0.0, 4);
	TArray<FVector> Halfway;
	Chain.Interpolate(0.5f, Halfway);
	TestTrue(TEXT("The tip fell"), Chain.Positions[3].Z < 100.0);
	TestEqual(TEXT("Halfway between the steps"), Halfway[3], FMath::Lerp(Chain.PreviousPositions[3], Chain.Positions[3], 0.5), 1.e-4f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetPhysicsWeightTest, "Open3DPuppet.Dynamics.PhysicsWeight.FixedOrFromACurve", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetPhysicsWeightTest::RunTest(const FString& Parameters)
{
	PuppetTests::FPerformer Face;
	Face.SetCurve(TEXT("jawOpen"), 1.5f);
	const FPuppetPerformerPose Performer = Face.ToPose();
	FPuppetPose Puppet;
	FPuppetPerformerCalibration Calibration;
	const FPuppetSolveContext Context{ Puppet, Performer, Calibration };

	FPuppetPhysicsWeight Weight;
	Weight.Weight = 0.3f;
	int32 Curve = 0;
	FString Error;
	TestTrue(TEXT("No curve binds"), PuppetDynamics::BindPhysicsWeight(Weight, Performer, Curve, Error));
	TestEqual(TEXT("To no curve"), Curve, INDEX_NONE);
	TestNearlyEqual(TEXT("The fixed weight"), PuppetDynamics::GetPhysicsWeight(Weight, Curve, Context), 0.3f);

	Weight.Curve = TEXT("jawOpen");
	TestTrue(TEXT("A curve the performer has binds"), PuppetDynamics::BindPhysicsWeight(Weight, Performer, Curve, Error));
	TestNearlyEqual(TEXT("The curve, clamped to 1"), PuppetDynamics::GetPhysicsWeight(Weight, Curve, Context), 1.0f);

	Weight.Curve = TEXT("browInnerUp");
	TestFalse(TEXT("A curve the performer lacks"), PuppetDynamics::BindPhysicsWeight(Weight, Performer, Curve, Error));
	TestTrue(TEXT("Says which"), Error.Contains(TEXT("browInnerUp")));
	return true;
}

#endif
