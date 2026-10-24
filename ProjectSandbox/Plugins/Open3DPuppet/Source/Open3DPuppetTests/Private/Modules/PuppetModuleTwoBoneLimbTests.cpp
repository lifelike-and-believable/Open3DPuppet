// Copyright 2026 Lifelike & Believable. All Rights Reserved.

// UPuppetModule_TwoBoneLimb: FK, then two-bone IK to an effector target (design overview §4).

#include "PuppetTestHarness.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Harness/PuppetTestFixtures.h"
#include "PuppetModules/PuppetModule_Root.h"
#include "PuppetModules/PuppetModule_TwoBoneLimb.h"

namespace PuppetLimbTests
{
	/** The biped with an upper arm twice as long (60 cm): the forearm stays 25 cm. */
	TArray<PuppetTests::FBone> LongUpperArm()
	{
		return PuppetTests::WithOffset(PuppetTests::Biped(), TEXT("lowerarm_l"), FVector(0, 60, 0));
	}

	/**
	 * A left-arm module. The fixture's arm is straight in its reference pose, so the elbow's
	 * direction is set: +X, the side it bends out to when the forearm turns towards -X.
	 */
	UPuppetModule_TwoBoneLimb& AddArm(PuppetTests::FPuppet& Puppet, EPuppetEffectorMode Mode)
	{
		UPuppetModule_TwoBoneLimb& Arm = Puppet.Add<UPuppetModule_TwoBoneLimb>();
		Arm.EffectorMode = Mode;
		Arm.PoleDirection = FVector(1, 0, 0);
		return Arm;
	}

	/**
	 * Calibrates slot 0 at rest, then bends the performer's elbow 90 degrees about Z. The
	 * performer's hand is then at (-25, 45, 130), 25 cm back and 30 cm out from the shoulder at
	 * (0, 15, 130).
	 */
	void SolveWithElbowBent(PuppetTests::FPuppet& Puppet)
	{
		PuppetTests::FPerformer Performer(PuppetTests::Biped());
		Puppet.Build();
		Puppet.Drive(0, Performer);
		Puppet.Calibrate(0, Performer);
		Performer.SetLocalRotation(TEXT("lowerarm_l"), PuppetTests::Rotation(FVector::ZAxisVector, 90.0));
		Puppet.Drive(0, Performer);
		Puppet.Solve();
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetLimbRestTest, "Open3DPuppet.Modules.TwoBoneLimb.RestMapsToReference", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetLimbRestTest::RunTest(const FString& Parameters)
{
	PuppetTests::FPuppet Puppet(PuppetLimbTests::LongUpperArm());
	PuppetLimbTests::AddArm(Puppet, EPuppetEffectorMode::ScaledReach);
	PuppetTests::FPerformer Performer(PuppetTests::Biped());
	Puppet.Build();
	Puppet.Drive(0, Performer);
	Puppet.Calibrate(0, Performer);
	Puppet.Solve();
	TestEqual(TEXT("Hand at its reference"), Puppet.Component(TEXT("hand_l")), Puppet.Reference(TEXT("hand_l")), 1.e-2f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetLimbForwardKinematicsTest, "Open3DPuppet.Modules.TwoBoneLimb.ForwardKinematicsKeepsPuppetLengths", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetLimbForwardKinematicsTest::RunTest(const FString& Parameters)
{
	PuppetTests::FPuppet Puppet(PuppetLimbTests::LongUpperArm());
	PuppetLimbTests::AddArm(Puppet, EPuppetEffectorMode::ForwardKinematics);
	PuppetLimbTests::SolveWithElbowBent(Puppet);
	// Shoulder (0, 15, 130), the puppet's 60 cm upper arm along Y, then its 25 cm forearm turned to -X.
	TestEqual(TEXT("Hand"), Puppet.Component(TEXT("hand_l")).GetTranslation(), FVector(-25, 75, 130), 1.e-2f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetLimbScaledReachTest, "Open3DPuppet.Modules.TwoBoneLimb.ScaledReachScalesByLimbLength", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetLimbScaledReachTest::RunTest(const FString& Parameters)
{
	PuppetTests::FPuppet Puppet(PuppetLimbTests::LongUpperArm());
	PuppetLimbTests::AddArm(Puppet, EPuppetEffectorMode::ScaledReach);
	PuppetLimbTests::SolveWithElbowBent(Puppet);
	// The performer's reach from the shoulder, scaled by limb length: puppet 85 cm, performer 55 cm.
	const FVector Expected = FVector(0, 15, 130) + FVector(-25, 30, 0) * (85.0 / 55.0);
	TestEqual(TEXT("Hand at the scaled reach"), Puppet.Component(TEXT("hand_l")).GetTranslation(), Expected, 1.e-2f);
	TestEqual(TEXT("Upper arm keeps its length"), FVector::Dist(Puppet.Component(TEXT("upperarm_l")).GetTranslation(), Puppet.Component(TEXT("lowerarm_l")).GetTranslation()), 60.0, 1.e-2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetLimbAbsoluteTest, "Open3DPuppet.Modules.TwoBoneLimb.AbsoluteReachesThePerformersHand", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetLimbAbsoluteTest::RunTest(const FString& Parameters)
{
	PuppetTests::FPuppet Puppet(PuppetLimbTests::LongUpperArm());
	PuppetLimbTests::AddArm(Puppet, EPuppetEffectorMode::Absolute);
	PuppetLimbTests::SolveWithElbowBent(Puppet);
	TestEqual(TEXT("Hand where the performer's is"), Puppet.Component(TEXT("hand_l")).GetTranslation(), FVector(-25, 45, 130), 1.e-2f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetLimbPelvisRelativeTest, "Open3DPuppet.Modules.TwoBoneLimb.PelvisRelativeFollowsTheScaledPelvis", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetLimbPelvisRelativeTest::RunTest(const FString& Parameters)
{
	// A puppet twice the performer's size: the performer steps 10 cm along X, the Root module moves
	// the pelvis 20 cm, and the foot keeps its place relative to the pelvis.
	PuppetTests::FPuppet Puppet(PuppetTests::Scaled(PuppetTests::Biped(), 2.0));
	Puppet.Add<UPuppetModule_Root>();
	UPuppetModule_TwoBoneLimb& Leg = Puppet.Add<UPuppetModule_TwoBoneLimb>();
	Leg.PerformerUpper = Leg.PuppetUpper = TEXT("thigh_l");
	Leg.PerformerLower = Leg.PuppetLower = TEXT("calf_l");
	Leg.PerformerEnd = Leg.PuppetEnd = TEXT("foot_l");
	Leg.EffectorMode = EPuppetEffectorMode::PelvisRelative;

	PuppetTests::FPerformer Performer(PuppetTests::Biped());
	Puppet.Build();
	Puppet.Drive(0, Performer);
	Puppet.Calibrate(0, Performer);
	Performer.SetLocalTranslation(TEXT("pelvis"), FVector(10, 0, 100));
	Puppet.Drive(0, Performer);
	Puppet.Solve();

	TestEqual(TEXT("Foot"), Puppet.Component(TEXT("foot_l")).GetTranslation(), Puppet.Reference(TEXT("foot_l")).GetTranslation() + FVector(20, 0, 0), 1.e-2f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetLimbMissingJointTest, "Open3DPuppet.Modules.TwoBoneLimb.MissingJointSkipsTheModule", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetLimbMissingJointTest::RunTest(const FString& Parameters)
{
	AddExpectedMessagePlain(TEXT("the performer has no joint 'hand_r'"), ELogVerbosity::Warning);
	PuppetTests::FPuppet Puppet(PuppetTests::Biped());
	PuppetLimbTests::AddArm(Puppet, EPuppetEffectorMode::ScaledReach).PerformerEnd = TEXT("hand_r");
	PuppetLimbTests::SolveWithElbowBent(Puppet);
	TestEqual(TEXT("Hand left as the input"), Puppet.Component(TEXT("hand_l")), Puppet.Reference(TEXT("hand_l")), 1.e-3f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetLimbStraightReferenceTest, "Open3DPuppet.Modules.TwoBoneLimb.StraightReferenceNeedsPoleDirection", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetLimbStraightReferenceTest::RunTest(const FString& Parameters)
{
	// Without a bend in the reference pose or a Pole Direction, the elbow could point anywhere.
	AddExpectedMessagePlain(TEXT("set Pole Direction"), ELogVerbosity::Warning);
	PuppetTests::FPuppet Puppet(PuppetTests::Biped());
	PuppetLimbTests::AddArm(Puppet, EPuppetEffectorMode::ScaledReach).PoleDirection = FVector::ZeroVector;
	PuppetLimbTests::SolveWithElbowBent(Puppet);
	TestEqual(TEXT("Hand left as the input"), Puppet.Component(TEXT("hand_l")), Puppet.Reference(TEXT("hand_l")), 1.e-3f);
	return true;
}

#endif
