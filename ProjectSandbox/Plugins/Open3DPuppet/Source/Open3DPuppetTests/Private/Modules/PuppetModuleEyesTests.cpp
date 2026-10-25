// Copyright 2026 Lifelike & Believable. All Rights Reserved.

// UPuppetModule_Eyes: the performer's gaze, from ARKit eye-look curves or eye joints, onto the
// puppet's eyes with range limits and convergence (ADR 0006). The Face fixture's eyes sit under
// the head at identity rest rotations and face +X, so the default axes apply: looking left is a
// turn about +Z, looking up a turn about -Y.

#include "PuppetTestHarness.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Harness/PuppetTestFixtures.h"
#include "PuppetModules/PuppetModule_Eyes.h"
#include "Templates/Function.h"

namespace PuppetEyesTests
{
	const TCHAR* const GazeCurves[] = {
		TEXT("eyeLookInLeft"), TEXT("eyeLookOutLeft"), TEXT("eyeLookUpLeft"), TEXT("eyeLookDownLeft"),
		TEXT("eyeLookInRight"), TEXT("eyeLookOutRight"), TEXT("eyeLookUpRight"), TEXT("eyeLookDownRight"),
	};

	/** A face looking straight ahead, calibrated, then solved with the curves Set gives. */
	void SolveWithGaze(PuppetTests::FPuppet& Puppet, TFunctionRef<void(PuppetTests::FPerformer&)> Set)
	{
		PuppetTests::FPerformer Face;
		for (const TCHAR* Curve : GazeCurves)
		{
			Face.SetCurve(Curve, 0.0f);
		}
		Puppet.Build();
		Puppet.Drive(0, Face);
		Puppet.Calibrate(0, Face);
		Set(Face);
		Puppet.Drive(0, Face);
		Puppet.Solve();
	}

	FQuat Yaw(double Degrees)
	{
		return PuppetTests::Rotation(FVector::ZAxisVector, Degrees);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetEyesLookLeftTest, "Open3DPuppet.Modules.Eyes.CurvesTurnBothEyes", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetEyesLookLeftTest::RunTest(const FString& Parameters)
{
	// Looking fully to the left: the left eye looks out, the right eye in. Both turn Max Yaw (30).
	PuppetTests::FPuppet Puppet(PuppetTests::Face());
	Puppet.Add<UPuppetModule_Eyes>();
	PuppetEyesTests::SolveWithGaze(Puppet, [](PuppetTests::FPerformer& Face)
	{
		Face.SetCurve(TEXT("eyeLookOutLeft"), 1.0f);
		Face.SetCurve(TEXT("eyeLookInRight"), 1.0f);
	});
	PuppetTests::TestRotation(*this, TEXT("eye_l"), Puppet.Component(TEXT("eye_l")).GetRotation(), PuppetEyesTests::Yaw(30.0));
	PuppetTests::TestRotation(*this, TEXT("eye_r"), Puppet.Component(TEXT("eye_r")).GetRotation(), PuppetEyesTests::Yaw(30.0));
	TestEqual(TEXT("eye_l keeps its place"), Puppet.Component(TEXT("eye_l")).GetTranslation(), Puppet.Reference(TEXT("eye_l")).GetTranslation(), 1.e-3f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetEyesLookUpTest, "Open3DPuppet.Modules.Eyes.LookUpTurnsAboutThePitchAxis", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetEyesLookUpTest::RunTest(const FString& Parameters)
{
	// Halfway up with Max Pitch 20: 10 degrees about -Y, which tips +X towards +Z.
	PuppetTests::FPuppet Puppet(PuppetTests::Face());
	Puppet.Add<UPuppetModule_Eyes>();
	PuppetEyesTests::SolveWithGaze(Puppet, [](PuppetTests::FPerformer& Face)
	{
		Face.SetCurve(TEXT("eyeLookUpLeft"), 0.5f);
		Face.SetCurve(TEXT("eyeLookUpRight"), 0.5f);
	});
	const FQuat Up = PuppetTests::Rotation(FVector(0.0, -1.0, 0.0), 10.0);
	PuppetTests::TestRotation(*this, TEXT("eye_l"), Puppet.Component(TEXT("eye_l")).GetRotation(), Up);
	TestTrue(TEXT("eye_l looks up"), Puppet.Component(TEXT("eye_l")).GetRotation().RotateVector(FVector::XAxisVector).Z > 0.0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetEyesConvergenceTest, "Open3DPuppet.Modules.Eyes.ConvergenceSharesTheGaze", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetEyesConvergenceTest::RunTest(const FString& Parameters)
{
	// Only the left eye looks out (30 degrees); fully converged, both take the average, 15.
	PuppetTests::FPuppet Puppet(PuppetTests::Face());
	Puppet.Add<UPuppetModule_Eyes>().Convergence = 1.0f;
	PuppetEyesTests::SolveWithGaze(Puppet, [](PuppetTests::FPerformer& Face)
	{
		Face.SetCurve(TEXT("eyeLookOutLeft"), 1.0f);
	});
	PuppetTests::TestRotation(*this, TEXT("eye_l"), Puppet.Component(TEXT("eye_l")).GetRotation(), PuppetEyesTests::Yaw(15.0));
	PuppetTests::TestRotation(*this, TEXT("eye_r"), Puppet.Component(TEXT("eye_r")).GetRotation(), PuppetEyesTests::Yaw(15.0));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetEyesBonesTest, "Open3DPuppet.Modules.Eyes.BonesClampToTheRange", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetEyesBonesTest::RunTest(const FString& Parameters)
{
	// Performer eye joints: the left turns 45 degrees left, past Max Yaw (30), so it stops at 30;
	// the right turns 10 degrees right, inside the range.
	PuppetTests::FPuppet Puppet(PuppetTests::Face());
	Puppet.Add<UPuppetModule_Eyes>().Input = EPuppetEyeInput::Bones;
	PuppetTests::FPerformer Performer(PuppetTests::Face());
	Puppet.Build();
	Puppet.Drive(0, Performer);
	Puppet.Calibrate(0, Performer);
	Performer.SetLocalRotation(TEXT("eye_l"), PuppetEyesTests::Yaw(45.0));
	Performer.SetLocalRotation(TEXT("eye_r"), PuppetEyesTests::Yaw(-10.0));
	Puppet.Drive(0, Performer);
	Puppet.Solve();
	PuppetTests::TestRotation(*this, TEXT("eye_l, clamped"), Puppet.Component(TEXT("eye_l")).GetRotation(), PuppetEyesTests::Yaw(30.0));
	PuppetTests::TestRotation(*this, TEXT("eye_r"), Puppet.Component(TEXT("eye_r")).GetRotation(), PuppetEyesTests::Yaw(-10.0));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetEyesWeightTest, "Open3DPuppet.Modules.Eyes.WeightBlendsOverInput", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetEyesWeightTest::RunTest(const FString& Parameters)
{
	PuppetTests::FPuppet Puppet(PuppetTests::Face());
	Puppet.Add<UPuppetModule_Eyes>().Weight = 0.5f;
	PuppetEyesTests::SolveWithGaze(Puppet, [](PuppetTests::FPerformer& Face)
	{
		Face.SetCurve(TEXT("eyeLookOutLeft"), 1.0f);
	});
	PuppetTests::TestRotation(*this, TEXT("Half the turn"), Puppet.Component(TEXT("eye_l")).GetRotation(), PuppetEyesTests::Yaw(15.0));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetEyesMissingTest, "Open3DPuppet.Modules.Eyes.MissingBoneSkipsTheModule", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetEyesMissingTest::RunTest(const FString& Parameters)
{
	AddExpectedMessagePlain(TEXT("the puppet has no bone 'eye_x'"), ELogVerbosity::Warning);
	PuppetTests::FPuppet Puppet(PuppetTests::Face());
	Puppet.Add<UPuppetModule_Eyes>().PuppetRightEye = TEXT("eye_x");
	PuppetEyesTests::SolveWithGaze(Puppet, [](PuppetTests::FPerformer& Face)
	{
		Face.SetCurve(TEXT("eyeLookOutLeft"), 1.0f);
	});
	TestEqual(TEXT("eye_l left as the input"), Puppet.Component(TEXT("eye_l")), Puppet.Reference(TEXT("eye_l")), 1.e-3f);
	return true;
}

#endif
