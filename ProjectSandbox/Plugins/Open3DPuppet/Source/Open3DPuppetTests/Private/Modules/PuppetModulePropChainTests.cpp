// Copyright 2026 Lifelike & Believable. All Rights Reserved.

// UPuppetModule_PropChain (WP-7): a chain such as a tail driven by a tracked prop, read as a
// performer with one joint, `prop` (design overview §6).

#include "PuppetTestHarness.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Harness/PuppetTestFixtures.h"
#include "PuppetModules/PuppetModule_PropChain.h"

namespace PuppetPropChainTests
{
	const FName Tail(TEXT("Tail"));

	/** A tilt of the prop about X, which turns the upright tail towards -Y. */
	FQuat Tilt(double Degrees)
	{
		return PuppetTests::Rotation(FVector::XAxisVector, Degrees);
	}

	/** The tail fixture (tail_01 to tail_05 upright above the pelvis) with a Prop Chain on a Tail slot, driven by a one-joint prop. */
	struct FRig
	{
		FRig()
			: Puppet(PuppetTests::Tail())
			, Prop({ { TEXT("prop"), TEXT(""), FVector::ZeroVector } })
			, Module(Puppet.Add<UPuppetModule_PropChain>())
		{
			Puppet.Asset->PerformerSlots = { TEXT("Body"), Tail };
			Puppet.Asset->RootSlot = TEXT("Body");
			Module.PerformerSlot = Tail;
		}

		/** Builds, calibrates the prop at rest and solves once there. */
		void Start()
		{
			Puppet.Build();
			Puppet.Drive(1, Prop);
			Puppet.Calibrate(1, Prop);
			Puppet.Solve();
		}

		/** Turns the prop to Rotation and solves one frame from Input. */
		void Step(const FQuat& Rotation, TConstArrayView<FTransform> Input, float DeltaTime = 1.0f / 30.0f)
		{
			Prop.SetLocalRotation(TEXT("prop"), Rotation);
			Puppet.Drive(1, Prop);
			Puppet.Solve(Input, DeltaTime);
		}

		void Step(const FQuat& Rotation) { Step(Rotation, Puppet.RefSkeleton.GetRefBonePose()); }

		FQuat Bone(const TCHAR* Name) const { return Puppet.Component(Name).GetRotation(); }

		/** The input pose with the pelvis turned by Yaw degrees about Z: the body turning. */
		TArray<FTransform> BodyTurned(double Yaw) const
		{
			return Puppet.InputWith(TEXT("pelvis"), FTransform(PuppetTests::Rotation(FVector::ZAxisVector, Yaw), FVector(0, 0, 100)));
		}

		PuppetTests::FPuppet Puppet;
		PuppetTests::FPerformer Prop;
		UPuppetModule_PropChain& Module;
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetPropChainBendTest, "Open3DPuppet.Modules.PropChain.BendSpreadsTheTiltAlongTheChain", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetPropChainBendTest::RunTest(const FString& Parameters)
{
	PuppetPropChainTests::FRig Rig;
	Rig.Start();
	PuppetTests::TestRotation(*this, TEXT("At rest the tail keeps its rest"), Rig.Bone(TEXT("tail_05")), FQuat::Identity);
	Rig.Step(PuppetPropChainTests::Tilt(40.0));
	PuppetTests::TestRotation(*this, TEXT("The base keeps its rest"), Rig.Bone(TEXT("tail_01")), FQuat::Identity);
	PuppetTests::TestRotation(*this, TEXT("Halfway along, half the tilt"), Rig.Bone(TEXT("tail_03")), PuppetPropChainTests::Tilt(20.0));
	PuppetTests::TestRotation(*this, TEXT("The tip turns with the prop"), Rig.Bone(TEXT("tail_05")), PuppetPropChainTests::Tilt(40.0));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetPropChainRigidTest, "Open3DPuppet.Modules.PropChain.RigidTurnsTheWholeChain", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetPropChainRigidTest::RunTest(const FString& Parameters)
{
	PuppetPropChainTests::FRig Rig;
	Rig.Module.Shape = EPuppetPropShape::Rigid;
	Rig.Start();
	Rig.Step(PuppetPropChainTests::Tilt(40.0));
	PuppetTests::TestRotation(*this, TEXT("The base"), Rig.Bone(TEXT("tail_01")), PuppetPropChainTests::Tilt(40.0));
	PuppetTests::TestRotation(*this, TEXT("The tip"), Rig.Bone(TEXT("tail_05")), PuppetPropChainTests::Tilt(40.0));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetPropChainSpaceTest, "Open3DPuppet.Modules.PropChain.ParentAndStageSpaces", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetPropChainSpaceTest::RunTest(const FString& Parameters)
{
	const FQuat Turn = PuppetTests::Rotation(FVector::ZAxisVector, 90.0);
	// Parent: the body turns and the prop stays still, so the tail keeps its rest relative to the body.
	{
		PuppetPropChainTests::FRig Rig;
		Rig.Start();
		Rig.Step(FQuat::Identity, Rig.BodyTurned(90.0));
		PuppetTests::TestRotation(*this, TEXT("Parent: the base turns with the body"), Rig.Bone(TEXT("tail_01")), Turn);
		PuppetTests::TestRotation(*this, TEXT("Parent: so does the tip"), Rig.Bone(TEXT("tail_05")), Turn);
		Rig.Step(PuppetPropChainTests::Tilt(40.0), Rig.BodyTurned(90.0));
		PuppetTests::TestRotation(*this, TEXT("Parent: a tilt adds to the body's turn"), Rig.Bone(TEXT("tail_05")), Turn * PuppetPropChainTests::Tilt(40.0));
	}
	// Stage: the tip keeps the prop's direction on the stage while the base follows the body.
	{
		PuppetPropChainTests::FRig Rig;
		Rig.Module.Space = EPuppetPropSpace::Stage;
		Rig.Start();
		Rig.Step(FQuat::Identity, Rig.BodyTurned(90.0));
		PuppetTests::TestRotation(*this, TEXT("Stage: the base turns with the body"), Rig.Bone(TEXT("tail_01")), Turn);
		PuppetTests::TestRotation(*this, TEXT("Stage: the tip keeps the prop's direction"), Rig.Bone(TEXT("tail_05")), FQuat::Identity);
		Rig.Step(PuppetPropChainTests::Tilt(40.0), Rig.BodyTurned(90.0));
		PuppetTests::TestRotation(*this, TEXT("Stage: the tip points where the prop points"), Rig.Bone(TEXT("tail_05")), PuppetPropChainTests::Tilt(40.0));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetPropChainAmplificationTest, "Open3DPuppet.Modules.PropChain.AmplificationScalesTheTilt", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetPropChainAmplificationTest::RunTest(const FString& Parameters)
{
	PuppetPropChainTests::FRig Rig;
	Rig.Module.Amplification = 2.0f;
	Rig.Start();
	Rig.Step(PuppetPropChainTests::Tilt(20.0));
	PuppetTests::TestRotation(*this, TEXT("Twice the tilt at the tip"), Rig.Bone(TEXT("tail_05")), PuppetPropChainTests::Tilt(40.0));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetPropChainSmoothingTest, "Open3DPuppet.Modules.PropChain.SmoothingIsFrameRateIndependent", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetPropChainSmoothingTest::RunTest(const FString& Parameters)
{
	// A 40-degree step with a 0.1 s smoothing time: after 0.2 s the tip has turned
	// 40 * (1 - e^-2) degrees, at 30 or at 60 frames a second.
	const double Expected = 40.0 * (1.0 - FMath::Exp(-2.0));
	for (const int32 Rate : { 30, 60 })
	{
		PuppetPropChainTests::FRig Rig;
		Rig.Module.Smoothing = 0.1f;
		Rig.Start();
		Rig.Step(PuppetPropChainTests::Tilt(40.0), Rig.Puppet.RefSkeleton.GetRefBonePose(), 1.0f / Rate);
		const double FirstFrame = FMath::RadiansToDegrees(Rig.Bone(TEXT("tail_05")).GetAngle());
		TestTrue(FString::Printf(TEXT("At %d fps the first frame does not jump"), Rate), FirstFrame < 20.0);
		for (int32 Frame = 1; Frame < Rate / 5; ++Frame)
		{
			Rig.Step(PuppetPropChainTests::Tilt(40.0), Rig.Puppet.RefSkeleton.GetRefBonePose(), 1.0f / Rate);
		}
		TestNearlyEqual(FString::Printf(TEXT("At %d fps after 0.2 s"), Rate), FMath::RadiansToDegrees(Rig.Bone(TEXT("tail_05")).GetAngle()), Expected, 0.01);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetPropChainMissingTest, "Open3DPuppet.Modules.PropChain.WithoutTheJointItDoesNotRun", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetPropChainMissingTest::RunTest(const FString& Parameters)
{
	AddExpectedMessagePlain(TEXT("the performer has no joint 'prop'"), ELogVerbosity::Warning);
	PuppetPropChainTests::FRig Rig;
	Rig.Prop = PuppetTests::FPerformer({ { TEXT("stick"), TEXT(""), FVector::ZeroVector } });
	Rig.Start();
	Rig.Prop.SetLocalRotation(TEXT("stick"), PuppetPropChainTests::Tilt(40.0));
	Rig.Puppet.Drive(1, Rig.Prop);
	Rig.Puppet.Solve();
	PuppetTests::TestRotation(*this, TEXT("The tail stays as the input pose left it"), Rig.Bone(TEXT("tail_05")), FQuat::Identity);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
