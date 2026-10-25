// Copyright 2026 Lifelike & Believable. All Rights Reserved.

// UPuppetModule_CurveToBone: one performer curve turns and moves one puppet bone (ADR 0006). The
// Face fixture's jaw sits under the head at an identity rest rotation, and nothing moves the head,
// so the jaw's component rotation is the module's turn.

#include "PuppetTestHarness.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Harness/PuppetTestFixtures.h"
#include "PuppetModules/PuppetModule_CurveToBone.h"

namespace PuppetCurveToBoneTests
{
	/** A jaw that turns 30 degrees of yaw (about Z) at jawOpen 1, so values are easy to read. */
	UPuppetModule_CurveToBone& AddJaw(PuppetTests::FPuppet& Puppet)
	{
		UPuppetModule_CurveToBone& Jaw = Puppet.Add<UPuppetModule_CurveToBone>();
		Jaw.Rotation = FRotator(0.0, 30.0, 0.0);
		return Jaw;
	}

	/** Calibrates a face at jawOpen 0, then solves one frame with jawOpen at Value. */
	void SolveWithJaw(PuppetTests::FPuppet& Puppet, PuppetTests::FPerformer& Face, float Value)
	{
		Face.SetCurve(TEXT("jawOpen"), 0.0f);
		Puppet.Build();
		Puppet.Drive(0, Face);
		Puppet.Calibrate(0, Face);
		Face.SetCurve(TEXT("jawOpen"), Value);
		Puppet.Drive(0, Face);
		Puppet.Solve();
	}

	FQuat Yaw(double Degrees)
	{
		return PuppetTests::Rotation(FVector::ZAxisVector, Degrees);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetCurveToBoneNeutralTest, "Open3DPuppet.Modules.CurveToBone.NeutralLeavesTheReference", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetCurveToBoneNeutralTest::RunTest(const FString& Parameters)
{
	PuppetTests::FPuppet Puppet(PuppetTests::Face());
	PuppetCurveToBoneTests::AddJaw(Puppet);
	PuppetTests::FPerformer Face;
	PuppetCurveToBoneTests::SolveWithJaw(Puppet, Face, 0.0f);
	TestEqual(TEXT("jaw at its reference"), Puppet.Component(TEXT("jaw")), Puppet.Reference(TEXT("jaw")), 1.e-3f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetCurveToBoneRotationTest, "Open3DPuppet.Modules.CurveToBone.CurveTurnsTheBone", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetCurveToBoneRotationTest::RunTest(const FString& Parameters)
{
	PuppetTests::FPuppet Puppet(PuppetTests::Face());
	PuppetCurveToBoneTests::AddJaw(Puppet);
	PuppetTests::FPerformer Face;
	PuppetCurveToBoneTests::SolveWithJaw(Puppet, Face, 1.0f);
	PuppetTests::TestRotation(*this, TEXT("Full turn at 1"), Puppet.Component(TEXT("jaw")).GetRotation(), PuppetCurveToBoneTests::Yaw(30.0));

	Face.SetCurve(TEXT("jawOpen"), 0.5f);
	Puppet.Drive(0, Face);
	Puppet.Solve();
	PuppetTests::TestRotation(*this, TEXT("Half the turn at 0.5"), Puppet.Component(TEXT("jaw")).GetRotation(), PuppetCurveToBoneTests::Yaw(15.0));
	TestEqual(TEXT("The jaw keeps its place"), Puppet.Component(TEXT("jaw")).GetTranslation(), Puppet.Reference(TEXT("jaw")).GetTranslation(), 1.e-3f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetCurveToBoneRangeTest, "Open3DPuppet.Modules.CurveToBone.InputRangeAndDeadZone", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetCurveToBoneRangeTest::RunTest(const FString& Parameters)
{
	// Input 0 to 0.5 with half of it dead: 0.25 is still neutral, 0.375 is halfway past the dead zone.
	PuppetTests::FPuppet Puppet(PuppetTests::Face());
	UPuppetModule_CurveToBone& Jaw = PuppetCurveToBoneTests::AddJaw(Puppet);
	Jaw.InputMax = 0.5f;
	Jaw.DeadZone = 0.5f;
	PuppetTests::FPerformer Face;
	PuppetCurveToBoneTests::SolveWithJaw(Puppet, Face, 0.25f);
	PuppetTests::TestRotation(*this, TEXT("In the dead zone"), Puppet.Component(TEXT("jaw")).GetRotation(), FQuat::Identity);

	Face.SetCurve(TEXT("jawOpen"), 0.375f);
	Puppet.Drive(0, Face);
	Puppet.Solve();
	PuppetTests::TestRotation(*this, TEXT("Halfway past it"), Puppet.Component(TEXT("jaw")).GetRotation(), PuppetCurveToBoneTests::Yaw(15.0));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetCurveToBoneTranslationTest, "Open3DPuppet.Modules.CurveToBone.CurveMovesTheBone", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetCurveToBoneTranslationTest::RunTest(const FString& Parameters)
{
	// A jaw that drops 2 cm at 1, in the head's space; the head is at an identity rotation.
	PuppetTests::FPuppet Puppet(PuppetTests::Face());
	UPuppetModule_CurveToBone& Jaw = Puppet.Add<UPuppetModule_CurveToBone>();
	Jaw.Rotation = FRotator::ZeroRotator;
	Jaw.Translation = FVector(0.0, 0.0, -2.0);
	PuppetTests::FPerformer Face;
	PuppetCurveToBoneTests::SolveWithJaw(Puppet, Face, 0.5f);
	TestEqual(TEXT("Half the drop"), Puppet.Component(TEXT("jaw")).GetTranslation(), Puppet.Reference(TEXT("jaw")).GetTranslation() + FVector(0.0, 0.0, -1.0), 1.e-3f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetCurveToBoneSmoothingTest, "Open3DPuppet.Modules.CurveToBone.SmoothingFollowsOverTime", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetCurveToBoneSmoothingTest::RunTest(const FString& Parameters)
{
	// A time constant of one frame (1/30 s): a jump from 0 to 1 covers 1 - 1/e of the way in a frame.
	PuppetTests::FPuppet Puppet(PuppetTests::Face());
	PuppetCurveToBoneTests::AddJaw(Puppet).SmoothingTime = 1.0f / 30.0f;
	PuppetTests::FPerformer Face;
	PuppetCurveToBoneTests::SolveWithJaw(Puppet, Face, 0.0f);
	Face.SetCurve(TEXT("jawOpen"), 1.0f);
	Puppet.Drive(0, Face);
	Puppet.Solve();
	PuppetTests::TestRotation(*this, TEXT("63% of the turn"), Puppet.Component(TEXT("jaw")).GetRotation(), PuppetCurveToBoneTests::Yaw(30.0 * (1.0 - FMath::Exp(-1.0))));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetCurveToBoneWeightTest, "Open3DPuppet.Modules.CurveToBone.WeightBlendsOverInput", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetCurveToBoneWeightTest::RunTest(const FString& Parameters)
{
	PuppetTests::FPuppet Puppet(PuppetTests::Face());
	PuppetCurveToBoneTests::AddJaw(Puppet).Weight = 0.5f;
	PuppetTests::FPerformer Face;
	PuppetCurveToBoneTests::SolveWithJaw(Puppet, Face, 1.0f);
	PuppetTests::TestRotation(*this, TEXT("Half the turn"), Puppet.Component(TEXT("jaw")).GetRotation(), PuppetCurveToBoneTests::Yaw(15.0));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetCurveToBoneMissingTest, "Open3DPuppet.Modules.CurveToBone.MissingCurveSkipsTheModule", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetCurveToBoneMissingTest::RunTest(const FString& Parameters)
{
	AddExpectedMessagePlain(TEXT("the performer has no curve 'beakOpen'"), ELogVerbosity::Warning);
	PuppetTests::FPuppet Puppet(PuppetTests::Face());
	PuppetCurveToBoneTests::AddJaw(Puppet).PerformerCurve = TEXT("beakOpen");
	PuppetTests::FPerformer Face;
	PuppetCurveToBoneTests::SolveWithJaw(Puppet, Face, 1.0f);
	TestEqual(TEXT("jaw left as the input"), Puppet.Component(TEXT("jaw")), Puppet.Reference(TEXT("jaw")), 1.e-3f);
	return true;
}

#endif
