// Copyright 2026 Lifelike & Believable. All Rights Reserved.

// UPuppetModule_Wing: flap from the upper arm, fold from the elbow, feathers fanned by the fold
// (design overview §10). The Wing fixture's left arm lies along +Y at identity rest rotations,
// with three feathers on the hand; the performer is the plain biped.

#include "PuppetTestHarness.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Harness/PuppetTestFixtures.h"
#include "PuppetModules/PuppetModule_Wing.h"
#include "Templates/Function.h"

namespace PuppetWingTests
{
	/** A wing that folds over elbow bends of 0 to 90 degrees, with no fold poses unless a test sets them. */
	UPuppetModule_Wing& AddWing(PuppetTests::FPuppet& Puppet)
	{
		UPuppetModule_Wing& Wing = Puppet.Add<UPuppetModule_Wing>();
		Wing.Feathers = { TEXT("feather_01"), TEXT("feather_02"), TEXT("feather_03") };
		Wing.FoldStartAngle = 0.0f;
		Wing.FoldEndAngle = 90.0f;
		Wing.ForearmFold = FRotator::ZeroRotator;
		Wing.HandFold = FRotator::ZeroRotator;
		Wing.FeatherFold = FRotator::ZeroRotator;
		return Wing;
	}

	/** Calibrates the performer standing, then poses them with Pose and solves one frame. */
	void Solve(PuppetTests::FPuppet& Puppet, TFunctionRef<void(PuppetTests::FPerformer&)> Pose)
	{
		PuppetTests::FPerformer Performer(PuppetTests::Biped());
		Puppet.Build();
		Puppet.Drive(0, Performer);
		Puppet.Calibrate(0, Performer);
		Pose(Performer);
		Puppet.Drive(0, Performer);
		Puppet.Solve();
	}

	FQuat Yaw(double Degrees)
	{
		return PuppetTests::Rotation(FVector::ZAxisVector, Degrees);
	}

	FQuat Lift(double Degrees)
	{
		return PuppetTests::Rotation(FVector::XAxisVector, Degrees);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetWingRestTest, "Open3DPuppet.Modules.Wing.RestMapsToReference", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetWingRestTest::RunTest(const FString& Parameters)
{
	PuppetTests::FPuppet Puppet(PuppetTests::Wing());
	UPuppetModule_Wing& Wing = Puppet.Add<UPuppetModule_Wing>();
	Wing.Feathers = { TEXT("feather_01"), TEXT("feather_02"), TEXT("feather_03") };
	PuppetWingTests::Solve(Puppet, [](PuppetTests::FPerformer&) {});
	TestEqual(TEXT("hand_l at its reference"), Puppet.Component(TEXT("hand_l")), Puppet.Reference(TEXT("hand_l")), 1.e-3f);
	TestEqual(TEXT("feather_03 at its reference"), Puppet.Component(TEXT("feather_03")), Puppet.Reference(TEXT("feather_03")), 1.e-3f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetWingFlapTest, "Open3DPuppet.Modules.Wing.FlapFollowsTheUpperArm", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetWingFlapTest::RunTest(const FString& Parameters)
{
	// The performer raises the arm 30 degrees about X: the wing root rises 30.
	PuppetTests::FPuppet Puppet(PuppetTests::Wing());
	PuppetWingTests::AddWing(Puppet);
	PuppetWingTests::Solve(Puppet, [](PuppetTests::FPerformer& Performer)
	{
		Performer.SetLocalRotation(TEXT("upperarm_l"), PuppetWingTests::Lift(30.0));
	});
	PuppetTests::TestRotation(*this, TEXT("upperarm_l"), Puppet.Component(TEXT("upperarm_l")).GetRotation(), PuppetWingTests::Lift(30.0));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetWingAmplificationTest, "Open3DPuppet.Modules.Wing.FlapAmplificationScalesTheFlap", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetWingAmplificationTest::RunTest(const FString& Parameters)
{
	PuppetTests::FPuppet Puppet(PuppetTests::Wing());
	PuppetWingTests::AddWing(Puppet).FlapAmplification = 2.0f;
	PuppetWingTests::Solve(Puppet, [](PuppetTests::FPerformer& Performer)
	{
		Performer.SetLocalRotation(TEXT("upperarm_l"), PuppetWingTests::Lift(30.0));
	});
	PuppetTests::TestRotation(*this, TEXT("Twice the flap"), Puppet.Component(TEXT("upperarm_l")).GetRotation(), PuppetWingTests::Lift(60.0));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetWingRelativeTest, "Open3DPuppet.Modules.Wing.FlapIsRelativeToTheChest", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetWingRelativeTest::RunTest(const FString& Parameters)
{
	// The performer's chest turns 90 degrees and the arm rises 30 in it. No module turns the
	// puppet's chest, so the wing takes the rise alone.
	PuppetTests::FPuppet Puppet(PuppetTests::Wing());
	PuppetWingTests::AddWing(Puppet);
	PuppetWingTests::Solve(Puppet, [](PuppetTests::FPerformer& Performer)
	{
		Performer.SetLocalRotation(TEXT("spine_03"), PuppetWingTests::Yaw(90.0));
		Performer.SetLocalRotation(TEXT("upperarm_l"), PuppetWingTests::Lift(30.0));
	});
	PuppetTests::TestRotation(*this, TEXT("upperarm_l"), Puppet.Component(TEXT("upperarm_l")).GetRotation(), PuppetWingTests::Lift(30.0));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetWingFoldTest, "Open3DPuppet.Modules.Wing.ElbowBendFoldsTheWing", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetWingFoldTest::RunTest(const FString& Parameters)
{
	// An elbow bend of 45 degrees, halfway through the 0 to 90 fold range: the forearm takes half of
	// its 90 degree fold and the hand half of its -90, so the hand ends square with the arm.
	PuppetTests::FPuppet Puppet(PuppetTests::Wing());
	UPuppetModule_Wing& Wing = PuppetWingTests::AddWing(Puppet);
	Wing.ForearmFold = FRotator(0.0, 90.0, 0.0);
	Wing.HandFold = FRotator(0.0, -90.0, 0.0);
	PuppetWingTests::Solve(Puppet, [](PuppetTests::FPerformer& Performer)
	{
		Performer.SetLocalRotation(TEXT("lowerarm_l"), PuppetWingTests::Yaw(45.0));
	});
	PuppetTests::TestRotation(*this, TEXT("lowerarm_l"), Puppet.Component(TEXT("lowerarm_l")).GetRotation(), PuppetWingTests::Yaw(45.0));
	PuppetTests::TestRotation(*this, TEXT("hand_l"), Puppet.Component(TEXT("hand_l")).GetRotation(), FQuat::Identity);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetWingFeathersTest, "Open3DPuppet.Modules.Wing.FoldClosesTheFeatherFan", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetWingFeathersTest::RunTest(const FString& Parameters)
{
	// Fully folded (elbow 90): the first feather stays, the second turns half the Feather Fold (15
	// of 30), the last all of it.
	PuppetTests::FPuppet Puppet(PuppetTests::Wing());
	PuppetWingTests::AddWing(Puppet).FeatherFold = FRotator(0.0, 30.0, 0.0);
	PuppetWingTests::Solve(Puppet, [](PuppetTests::FPerformer& Performer)
	{
		Performer.SetLocalRotation(TEXT("lowerarm_l"), PuppetWingTests::Yaw(90.0));
	});
	PuppetTests::TestRotation(*this, TEXT("feather_01"), Puppet.Component(TEXT("feather_01")).GetRotation(), FQuat::Identity);
	PuppetTests::TestRotation(*this, TEXT("feather_02"), Puppet.Component(TEXT("feather_02")).GetRotation(), PuppetWingTests::Yaw(15.0));
	PuppetTests::TestRotation(*this, TEXT("feather_03"), Puppet.Component(TEXT("feather_03")).GetRotation(), PuppetWingTests::Yaw(30.0));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetWingWeightTest, "Open3DPuppet.Modules.Wing.WeightBlendsOverInput", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetWingWeightTest::RunTest(const FString& Parameters)
{
	PuppetTests::FPuppet Puppet(PuppetTests::Wing());
	PuppetWingTests::AddWing(Puppet).Weight = 0.5f;
	PuppetWingTests::Solve(Puppet, [](PuppetTests::FPerformer& Performer)
	{
		Performer.SetLocalRotation(TEXT("upperarm_l"), PuppetWingTests::Lift(30.0));
	});
	PuppetTests::TestRotation(*this, TEXT("Half the flap"), Puppet.Component(TEXT("upperarm_l")).GetRotation(), PuppetWingTests::Lift(15.0));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetWingMissingTest, "Open3DPuppet.Modules.Wing.MissingBoneSkipsTheModule", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetWingMissingTest::RunTest(const FString& Parameters)
{
	AddExpectedMessagePlain(TEXT("the puppet has no feather bone 'feather_09'"), ELogVerbosity::Warning);
	PuppetTests::FPuppet Puppet(PuppetTests::Wing());
	PuppetWingTests::AddWing(Puppet).Feathers.Add(TEXT("feather_09"));
	PuppetWingTests::Solve(Puppet, [](PuppetTests::FPerformer& Performer)
	{
		Performer.SetLocalRotation(TEXT("upperarm_l"), PuppetWingTests::Lift(30.0));
	});
	TestEqual(TEXT("upperarm_l left as the input"), Puppet.Component(TEXT("upperarm_l")), Puppet.Reference(TEXT("upperarm_l")), 1.e-3f);
	return true;
}

#endif
