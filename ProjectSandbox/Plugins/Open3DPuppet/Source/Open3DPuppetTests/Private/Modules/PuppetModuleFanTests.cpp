// Copyright 2026 Lifelike & Believable. All Rights Reserved.

// UPuppetModule_Fan: a fan of sibling bones spread from the averaged bend of performer joints
// (design overview §10, the Eagle's tail from the elbows). The Fan fixture serves as both puppet
// and performer: both arms, and five feathers under tail_01 at identity rest rotations.

#include "PuppetTestHarness.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Harness/PuppetTestFixtures.h"
#include "PuppetModules/PuppetModule_Fan.h"
#include "Templates/Function.h"

namespace PuppetFanTests
{
	/** A five-feather fan, fully open at a bend of 0 degrees and closed at 90, each edge spreading 40. */
	UPuppetModule_Fan& AddFan(PuppetTests::FPuppet& Puppet)
	{
		UPuppetModule_Fan& Fan = Puppet.Add<UPuppetModule_Fan>();
		Fan.Feathers = { TEXT("fan_01"), TEXT("fan_02"), TEXT("fan_03"), TEXT("fan_04"), TEXT("fan_05") };
		Fan.OpenBend = 0.0f;
		Fan.ClosedBend = 90.0f;
		Fan.SpreadAngle = 40.0f;
		return Fan;
	}

	FQuat Yaw(double Degrees)
	{
		return PuppetTests::Rotation(FVector::ZAxisVector, Degrees);
	}

	/** Bends both elbows by Left and Right degrees. */
	void BendElbows(PuppetTests::FPerformer& Performer, double Left, double Right)
	{
		Performer.SetLocalRotation(TEXT("lowerarm_l"), Yaw(Left));
		Performer.SetLocalRotation(TEXT("lowerarm_r"), Yaw(-Right));
	}

	/** Calibrates the performer standing (elbows straight), then poses them with Pose and solves one frame. */
	void Solve(PuppetTests::FPuppet& Puppet, PuppetTests::FPerformer& Performer, TFunctionRef<void(PuppetTests::FPerformer&)> Pose)
	{
		Puppet.Build();
		Puppet.Drive(0, Performer);
		Puppet.Calibrate(0, Performer);
		Pose(Performer);
		Puppet.Drive(0, Performer);
		Puppet.Solve();
	}

	void Solve(PuppetTests::FPuppet& Puppet, TFunctionRef<void(PuppetTests::FPerformer&)> Pose)
	{
		PuppetTests::FPerformer Performer(PuppetTests::Fan());
		Solve(Puppet, Performer, Pose);
	}

	/** Checks each of the five feathers' turn about Z against Degrees. */
	void TestFan(FAutomationTestBase& Test, const PuppetTests::FPuppet& Puppet, const double (&Degrees)[5])
	{
		const TCHAR* Names[5] = { TEXT("fan_01"), TEXT("fan_02"), TEXT("fan_03"), TEXT("fan_04"), TEXT("fan_05") };
		for (int32 Index = 0; Index < 5; ++Index)
		{
			PuppetTests::TestRotation(Test, Names[Index], Puppet.Component(Names[Index]).GetRotation(), Yaw(Degrees[Index]));
		}
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetFanOpenTest, "Open3DPuppet.Modules.Fan.StraightElbowsOpenTheFan", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetFanOpenTest::RunTest(const FString& Parameters)
{
	// Fully open: the middle feather stays, the edges turn 40 degrees each way, the others half that.
	PuppetTests::FPuppet Puppet(PuppetTests::Fan());
	PuppetFanTests::AddFan(Puppet);
	PuppetFanTests::Solve(Puppet, [](PuppetTests::FPerformer&) {});
	PuppetFanTests::TestFan(*this, Puppet, { -40.0, -20.0, 0.0, 20.0, 40.0 });
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetFanClosedTest, "Open3DPuppet.Modules.Fan.BentElbowsCloseTheFan", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetFanClosedTest::RunTest(const FString& Parameters)
{
	// Past the closed bend (100 of 90): every feather at its reference pose.
	PuppetTests::FPuppet Puppet(PuppetTests::Fan());
	PuppetFanTests::AddFan(Puppet);
	PuppetFanTests::Solve(Puppet, [](PuppetTests::FPerformer& Performer)
	{
		PuppetFanTests::BendElbows(Performer, 100.0, 100.0);
	});
	TestEqual(TEXT("fan_01 at its reference"), Puppet.Component(TEXT("fan_01")), Puppet.Reference(TEXT("fan_01")), 1.e-3f);
	TestEqual(TEXT("fan_05 at its reference"), Puppet.Component(TEXT("fan_05")), Puppet.Reference(TEXT("fan_05")), 1.e-3f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetFanHalfTest, "Open3DPuppet.Modules.Fan.HalfTheBendHalfTheSpread", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetFanHalfTest::RunTest(const FString& Parameters)
{
	PuppetTests::FPuppet Puppet(PuppetTests::Fan());
	PuppetFanTests::AddFan(Puppet);
	PuppetFanTests::Solve(Puppet, [](PuppetTests::FPerformer& Performer)
	{
		PuppetFanTests::BendElbows(Performer, 45.0, 45.0);
	});
	PuppetFanTests::TestFan(*this, Puppet, { -20.0, -10.0, 0.0, 10.0, 20.0 });
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetFanAverageTest, "Open3DPuppet.Modules.Fan.BendsAreAveraged", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetFanAverageTest::RunTest(const FString& Parameters)
{
	// One elbow fully bent, the other straight: an average of 45, half open.
	PuppetTests::FPuppet Puppet(PuppetTests::Fan());
	PuppetFanTests::AddFan(Puppet);
	PuppetFanTests::Solve(Puppet, [](PuppetTests::FPerformer& Performer)
	{
		PuppetFanTests::BendElbows(Performer, 90.0, 0.0);
	});
	PuppetFanTests::TestFan(*this, Puppet, { -20.0, -10.0, 0.0, 10.0, 20.0 });
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetFanRangeTest, "Open3DPuppet.Modules.Fan.InputRangeRemapsTheBend", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetFanRangeTest::RunTest(const FString& Parameters)
{
	// Open at 30, closed at 90: a bend of 60 is half open, and one of 10 is fully open.
	{
		PuppetTests::FPuppet Puppet(PuppetTests::Fan());
		PuppetFanTests::AddFan(Puppet).OpenBend = 30.0f;
		PuppetFanTests::Solve(Puppet, [](PuppetTests::FPerformer& Performer)
		{
			PuppetFanTests::BendElbows(Performer, 60.0, 60.0);
		});
		PuppetTests::TestRotation(*this, TEXT("Half open at 60"), Puppet.Component(TEXT("fan_05")).GetRotation(), PuppetFanTests::Yaw(20.0));
	}
	{
		PuppetTests::FPuppet Puppet(PuppetTests::Fan());
		PuppetFanTests::AddFan(Puppet).OpenBend = 30.0f;
		PuppetFanTests::Solve(Puppet, [](PuppetTests::FPerformer& Performer)
		{
			PuppetFanTests::BendElbows(Performer, 10.0, 10.0);
		});
		PuppetTests::TestRotation(*this, TEXT("Fully open at 10"), Puppet.Component(TEXT("fan_05")).GetRotation(), PuppetFanTests::Yaw(40.0));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetFanEvenTest, "Open3DPuppet.Modules.Fan.EvenCountHasNoMiddleFeather", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetFanEvenTest::RunTest(const FString& Parameters)
{
	// Four feathers, edge to edge: places -1, -1/3, 1/3 and 1 from the middle.
	PuppetTests::FPuppet Puppet(PuppetTests::Fan());
	PuppetFanTests::AddFan(Puppet).Feathers = { TEXT("fan_01"), TEXT("fan_02"), TEXT("fan_04"), TEXT("fan_05") };
	PuppetFanTests::Solve(Puppet, [](PuppetTests::FPerformer&) {});
	PuppetTests::TestRotation(*this, TEXT("fan_01"), Puppet.Component(TEXT("fan_01")).GetRotation(), PuppetFanTests::Yaw(-40.0));
	PuppetTests::TestRotation(*this, TEXT("fan_02"), Puppet.Component(TEXT("fan_02")).GetRotation(), PuppetFanTests::Yaw(-40.0 / 3.0));
	PuppetTests::TestRotation(*this, TEXT("fan_04"), Puppet.Component(TEXT("fan_04")).GetRotation(), PuppetFanTests::Yaw(40.0 / 3.0));
	PuppetTests::TestRotation(*this, TEXT("fan_05"), Puppet.Component(TEXT("fan_05")).GetRotation(), PuppetFanTests::Yaw(40.0));
	TestEqual(TEXT("fan_03, not listed, at its reference"), Puppet.Component(TEXT("fan_03")), Puppet.Reference(TEXT("fan_03")), 1.e-3f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetFanSmoothingTest, "Open3DPuppet.Modules.Fan.SmoothingFollowsOverTime", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetFanSmoothingTest::RunTest(const FString& Parameters)
{
	// A time constant of one frame (1/30 s): from closed, straightening the elbows opens the fan
	// 1 - 1/e of the way in a frame. The first frame starts at the bend, closed.
	PuppetTests::FPuppet Puppet(PuppetTests::Fan());
	PuppetFanTests::AddFan(Puppet).SmoothingTime = 1.0f / 30.0f;
	PuppetTests::FPerformer Performer(PuppetTests::Fan());
	PuppetFanTests::Solve(Puppet, Performer, [](PuppetTests::FPerformer& Bent)
	{
		PuppetFanTests::BendElbows(Bent, 90.0, 90.0);
	});
	TestEqual(TEXT("Closed on the first frame"), Puppet.Component(TEXT("fan_05")), Puppet.Reference(TEXT("fan_05")), 1.e-3f);
	PuppetFanTests::BendElbows(Performer, 0.0, 0.0);
	Puppet.Drive(0, Performer);
	Puppet.Solve();
	PuppetTests::TestRotation(*this, TEXT("63% open"), Puppet.Component(TEXT("fan_05")).GetRotation(), PuppetFanTests::Yaw(40.0 * (1.0 - FMath::Exp(-1.0))));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetFanWeightTest, "Open3DPuppet.Modules.Fan.WeightBlendsOverInput", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetFanWeightTest::RunTest(const FString& Parameters)
{
	PuppetTests::FPuppet Puppet(PuppetTests::Fan());
	PuppetFanTests::AddFan(Puppet).Weight = 0.5f;
	PuppetFanTests::Solve(Puppet, [](PuppetTests::FPerformer&) {});
	PuppetTests::TestRotation(*this, TEXT("Half the spread"), Puppet.Component(TEXT("fan_05")).GetRotation(), PuppetFanTests::Yaw(20.0));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetFanMissingBoneTest, "Open3DPuppet.Modules.Fan.MissingBoneSkipsTheModule", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetFanMissingBoneTest::RunTest(const FString& Parameters)
{
	AddExpectedMessagePlain(TEXT("the puppet has no feather bone 'fan_09'"), ELogVerbosity::Warning);
	PuppetTests::FPuppet Puppet(PuppetTests::Fan());
	PuppetFanTests::AddFan(Puppet).Feathers.Add(TEXT("fan_09"));
	PuppetFanTests::Solve(Puppet, [](PuppetTests::FPerformer&) {});
	TestEqual(TEXT("fan_05 left as the input"), Puppet.Component(TEXT("fan_05")), Puppet.Reference(TEXT("fan_05")), 1.e-3f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetFanMissingJointTest, "Open3DPuppet.Modules.Fan.MissingJointSkipsTheModule", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetFanMissingJointTest::RunTest(const FString& Parameters)
{
	AddExpectedMessagePlain(TEXT("the performer has no joint 'lowerarm_x'"), ELogVerbosity::Warning);
	PuppetTests::FPuppet Puppet(PuppetTests::Fan());
	PuppetFanTests::AddFan(Puppet).PerformerJoints.Add(TEXT("lowerarm_x"));
	PuppetFanTests::Solve(Puppet, [](PuppetTests::FPerformer&) {});
	TestEqual(TEXT("fan_05 left as the input"), Puppet.Component(TEXT("fan_05")), Puppet.Reference(TEXT("fan_05")), 1.e-3f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetFanNoJointsTest, "Open3DPuppet.Modules.Fan.NoJointsSkipsTheModule", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetFanNoJointsTest::RunTest(const FString& Parameters)
{
	AddExpectedMessagePlain(TEXT("the fan reads no performer joints"), ELogVerbosity::Warning);
	PuppetTests::FPuppet Puppet(PuppetTests::Fan());
	PuppetFanTests::AddFan(Puppet).PerformerJoints.Reset();
	PuppetFanTests::Solve(Puppet, [](PuppetTests::FPerformer&) {});
	TestEqual(TEXT("fan_05 left as the input"), Puppet.Component(TEXT("fan_05")), Puppet.Reference(TEXT("fan_05")), 1.e-3f);
	return true;
}

#endif
