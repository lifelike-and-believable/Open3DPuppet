// Copyright 2026 Lifelike & Believable. All Rights Reserved.

// UPuppetModule_DigitigradeLeg: a three-segment leg from a two-segment one (design overview §5.2).
// The puppet is the DigitigradeLeg fixture (60 cm of leg); the performer's left leg is straight,
// hip (0, 10, 100), knee (0, 10, 60), ankle (0, 10, 20): 80 cm. So the ankle's moves reach the
// paw scaled by 60 / 80 = 0.75. Nothing moves the puppet's pelvis.

#include "PuppetTestHarness.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Harness/PuppetTestFixtures.h"
#include "PuppetModules/PuppetModule_DigitigradeLeg.h"
#include "Templates/Function.h"

namespace PuppetDigitigradeTests
{
	const FVector PawRest(12, 10, 0);
	/** The metatarsal's bend axis: the shin (-0.6, 0, -0.8) crossed with the metatarsal (0.6, 0, -0.8). Turning about it flexes the hock. */
	const FVector HockAxis(0, -1, 0);

	TArray<PuppetTests::FBone> PerformerLeg()
	{
		return PuppetTests::WithOffset(PuppetTests::WithOffset(PuppetTests::Biped(), TEXT("calf_l"), FVector(0, 0, -40)), TEXT("foot_l"), FVector(0, 0, -40));
	}

	/** Calibrates the performer standing, then poses them with Pose and solves one frame. */
	void Solve(PuppetTests::FPuppet& Puppet, TFunctionRef<void(PuppetTests::FPerformer&)> Pose)
	{
		PuppetTests::FPerformer Performer(PerformerLeg());
		Puppet.Build();
		Puppet.Drive(0, Performer);
		Puppet.Calibrate(0, Performer);
		Pose(Performer);
		Puppet.Drive(0, Performer);
		Puppet.Solve();
	}

	/** A leg whose hock does not follow the knee, so tests that do not bend the knee read plainly. */
	UPuppetModule_DigitigradeLeg& AddLeg(PuppetTests::FPuppet& Puppet)
	{
		UPuppetModule_DigitigradeLeg& Leg = Puppet.Add<UPuppetModule_DigitigradeLeg>();
		Leg.KneeToHockRatio = 0.0f;
		return Leg;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetDigitigradeRestTest, "Open3DPuppet.Modules.DigitigradeLeg.RestMapsToReference", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetDigitigradeRestTest::RunTest(const FString& Parameters)
{
	PuppetTests::FPuppet Puppet(PuppetTests::DigitigradeLeg());
	Puppet.Add<UPuppetModule_DigitigradeLeg>();
	PuppetDigitigradeTests::Solve(Puppet, [](PuppetTests::FPerformer&) {});
	TestEqual(TEXT("calf_l at its reference"), Puppet.Component(TEXT("calf_l")), Puppet.Reference(TEXT("calf_l")), 1.e-2f);
	TestEqual(TEXT("foot_l at its reference"), Puppet.Component(TEXT("foot_l")), Puppet.Reference(TEXT("foot_l")), 1.e-2f);
	TestEqual(TEXT("ball_l at its reference"), Puppet.Component(TEXT("ball_l")), Puppet.Reference(TEXT("ball_l")), 1.e-2f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetDigitigradePawTest, "Open3DPuppet.Modules.DigitigradeLeg.PawFollowsTheAnkle", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetDigitigradePawTest::RunTest(const FString& Parameters)
{
	// The ankle steps 8 cm forward: the paw 6 cm. The metatarsal keeps its reference angle, so the
	// IK puts the hock 6 cm forward too.
	PuppetTests::FPuppet Puppet(PuppetTests::DigitigradeLeg());
	PuppetDigitigradeTests::AddLeg(Puppet);
	PuppetDigitigradeTests::Solve(Puppet, [](PuppetTests::FPerformer& Performer)
	{
		Performer.SetLocalTranslation(TEXT("foot_l"), FVector(8, 0, -40));
	});
	TestEqual(TEXT("Paw"), Puppet.Component(TEXT("ball_l")).GetTranslation(), PuppetDigitigradeTests::PawRest + FVector(6, 0, 0), 1.e-2f);
	TestEqual(TEXT("Hock"), Puppet.Component(TEXT("foot_l")).GetTranslation(), FVector(6, 10, 16), 1.e-2f);
	PuppetTests::TestRotation(*this, TEXT("Metatarsal at its reference angle"), Puppet.Component(TEXT("foot_l")).GetRotation(), FQuat::Identity);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetDigitigradeStrideTest, "Open3DPuppet.Modules.DigitigradeLeg.StrideScaleScalesHorizontalMoves", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetDigitigradeStrideTest::RunTest(const FString& Parameters)
{
	// The ankle moves 8 forward and 4 up; Stride Scale 2 doubles the forward part only: (16, 0, 4),
	// times 0.75 is (12, 0, 3).
	PuppetTests::FPuppet Puppet(PuppetTests::DigitigradeLeg());
	PuppetDigitigradeTests::AddLeg(Puppet).StrideScale = 2.0f;
	PuppetDigitigradeTests::Solve(Puppet, [](PuppetTests::FPerformer& Performer)
	{
		Performer.SetLocalTranslation(TEXT("foot_l"), FVector(8, 0, -36));
	});
	TestEqual(TEXT("Paw"), Puppet.Component(TEXT("ball_l")).GetTranslation(), PuppetDigitigradeTests::PawRest + FVector(12, 0, 3), 1.e-2f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetDigitigradeHeelLiftTest, "Open3DPuppet.Modules.DigitigradeLeg.HeelLiftRaisesTheHock", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetDigitigradeHeelLiftTest::RunTest(const FString& Parameters)
{
	// Standing, with Heel Lift 30: the metatarsal turns 30 degrees against the hock's flex axis
	// towards upright, the paw stays where it is, and the hock rises above its 16 cm.
	PuppetTests::FPuppet Puppet(PuppetTests::DigitigradeLeg());
	PuppetDigitigradeTests::AddLeg(Puppet).HeelLift = 30.0f;
	PuppetDigitigradeTests::Solve(Puppet, [](PuppetTests::FPerformer&) {});
	PuppetTests::TestRotation(*this, TEXT("Metatarsal"), Puppet.Component(TEXT("foot_l")).GetRotation(), PuppetTests::Rotation(PuppetDigitigradeTests::HockAxis, -30.0));
	TestEqual(TEXT("Paw"), Puppet.Component(TEXT("ball_l")).GetTranslation(), PuppetDigitigradeTests::PawRest, 1.e-2f);
	TestTrue(TEXT("Hock raised"), Puppet.Component(TEXT("foot_l")).GetTranslation().Z > 16.5);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetDigitigradeKneeToHockTest, "Open3DPuppet.Modules.DigitigradeLeg.HockFollowsTheKnee", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetDigitigradeKneeToHockTest::RunTest(const FString& Parameters)
{
	// The performer bends the knee 40 degrees (about Y): with Knee To Hock Ratio 0.5 the hock flexes
	// 20. The ankle swings from (0, 0, -80) below the hip to (0, 0, -40) + 40 * (-sin 40, 0, -cos 40),
	// and the paw follows 0.75 of that move.
	PuppetTests::FPuppet Puppet(PuppetTests::DigitigradeLeg());
	PuppetDigitigradeTests::AddLeg(Puppet).KneeToHockRatio = 0.5f;
	PuppetDigitigradeTests::Solve(Puppet, [](PuppetTests::FPerformer& Performer)
	{
		Performer.SetLocalRotation(TEXT("calf_l"), PuppetTests::Rotation(FVector::YAxisVector, 40.0));
	});
	const double Bend = FMath::DegreesToRadians(40.0);
	const FVector AnkleMove = FVector(-40.0 * FMath::Sin(Bend), 0.0, 40.0 - 40.0 * FMath::Cos(Bend));
	PuppetTests::TestRotation(*this, TEXT("Metatarsal flexed 20"), Puppet.Component(TEXT("foot_l")).GetRotation(), PuppetTests::Rotation(PuppetDigitigradeTests::HockAxis, 20.0));
	TestEqual(TEXT("Paw"), Puppet.Component(TEXT("ball_l")).GetTranslation(), PuppetDigitigradeTests::PawRest + AnkleMove * 0.75, 1.e-2f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetDigitigradeContactTest, "Open3DPuppet.Modules.DigitigradeLeg.PawStaysLevelOnTheGround", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetDigitigradeContactTest::RunTest(const FString& Parameters)
{
	// The performer's foot tips 30 degrees. On the ground the paw stays level; lifted 6 cm (an 8 cm
	// lift of the ankle), above Contact Height (2), it tips with the foot.
	PuppetTests::FPuppet Puppet(PuppetTests::DigitigradeLeg());
	PuppetDigitigradeTests::AddLeg(Puppet);
	const FQuat Tip = PuppetTests::Rotation(FVector::YAxisVector, 30.0);
	PuppetTests::FPerformer Performer(PuppetDigitigradeTests::PerformerLeg());
	Puppet.Build();
	Puppet.Drive(0, Performer);
	Puppet.Calibrate(0, Performer);
	Performer.SetLocalRotation(TEXT("foot_l"), Tip);
	Puppet.Drive(0, Performer);
	Puppet.Solve();
	PuppetTests::TestRotation(*this, TEXT("Level on the ground"), Puppet.Component(TEXT("ball_l")).GetRotation(), FQuat::Identity);

	Performer.SetLocalTranslation(TEXT("foot_l"), FVector(0, 0, -32));
	Puppet.Drive(0, Performer);
	Puppet.Solve();
	TestEqual(TEXT("Paw lifted"), Puppet.Component(TEXT("ball_l")).GetTranslation(), PuppetDigitigradeTests::PawRest + FVector(0, 0, 6), 1.e-2f);
	PuppetTests::TestRotation(*this, TEXT("Tipped with the foot"), Puppet.Component(TEXT("ball_l")).GetRotation(), Tip);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetDigitigradeWeightTest, "Open3DPuppet.Modules.DigitigradeLeg.WeightBlendsOverInput", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetDigitigradeWeightTest::RunTest(const FString& Parameters)
{
	PuppetTests::FPuppet Puppet(PuppetTests::DigitigradeLeg());
	PuppetDigitigradeTests::AddLeg(Puppet).Weight = 0.5f;
	PuppetDigitigradeTests::Solve(Puppet, [](PuppetTests::FPerformer& Performer)
	{
		Performer.SetLocalTranslation(TEXT("foot_l"), FVector(8, 0, -40));
	});
	TestEqual(TEXT("Half the paw's move"), Puppet.Component(TEXT("ball_l")).GetTranslation(), PuppetDigitigradeTests::PawRest + FVector(3, 0, 0), 1.e-2f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetDigitigradeMissingTest, "Open3DPuppet.Modules.DigitigradeLeg.MissingBoneSkipsTheModule", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetDigitigradeMissingTest::RunTest(const FString& Parameters)
{
	AddExpectedMessagePlain(TEXT("the puppet has no bone 'foot_x'"), ELogVerbosity::Warning);
	PuppetTests::FPuppet Puppet(PuppetTests::DigitigradeLeg());
	PuppetDigitigradeTests::AddLeg(Puppet).PuppetMetatarsal = TEXT("foot_x");
	PuppetDigitigradeTests::Solve(Puppet, [](PuppetTests::FPerformer& Performer)
	{
		Performer.SetLocalTranslation(TEXT("foot_l"), FVector(8, 0, -40));
	});
	TestEqual(TEXT("ball_l left as the input"), Puppet.Component(TEXT("ball_l")), Puppet.Reference(TEXT("ball_l")), 1.e-3f);
	return true;
}

#endif
