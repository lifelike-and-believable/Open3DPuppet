// Copyright 2026 Lifelike & Believable. All Rights Reserved.

// Curves in the pipeline (ADR 0006): performer curves from Live Link, their neutral values from
// calibration, curve-only (Basic-role) slots, and output curves written by modules.

#include "PuppetTestHarness.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Harness/PuppetTestFixtures.h"
#include "Harness/PuppetTestModules.h"
#include "PuppetPerformerCalibration.h"
#include "PuppetPerformerPose.h"
#include "PuppetPerformerProfile.h"
#include "PuppetPose.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetCurvesFindTest, "Open3DPuppet.Curves.PerformerPose.FindByNameAndProfile", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetCurvesFindTest::RunTest(const FString& Parameters)
{
	UPuppetPerformerProfile* Profile = NewObject<UPuppetPerformerProfile>();
	Profile->CurveCanonicalToSource.Add(TEXT("eyeBlinkLeft"), TEXT("EyeBlink_L"));

	FPuppetPerformerPose Pose;
	Pose.SetCurveNames(TArray<FName>({ TEXT("jawOpen"), TEXT("EyeBlink_L") }), Profile);
	TestTrue(TEXT("Values accepted"), Pose.SetCurveValues(TArray<float>({ 0.25f, 0.75f })));
	TestTrue(TEXT("Has curve values"), Pose.HasCurveValues());
	TestTrue(TEXT("Has data without bones"), Pose.HasData());

	const int32 Jaw = Pose.FindCurve(TEXT("jawOpen"));
	const int32 Blink = Pose.FindCurve(TEXT("eyeBlinkLeft"));
	TestEqual(TEXT("Looked up by its own name"), Jaw, 0);
	TestEqual(TEXT("Mapped through the profile"), Blink, 1);
	TestEqual(TEXT("Unknown curve"), Pose.FindCurve(TEXT("browInnerUp")), INDEX_NONE);
	if (Jaw != INDEX_NONE && Blink != INDEX_NONE)
	{
		TestNearlyEqual(TEXT("jawOpen value"), Pose.GetCurveValues()[Jaw], 0.25f);
		TestNearlyEqual(TEXT("eyeBlinkLeft value"), Pose.GetCurveValues()[Blink], 0.75f);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetCurvesCountTest, "Open3DPuppet.Curves.PerformerPose.RejectsValueCountMismatch", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetCurvesCountTest::RunTest(const FString& Parameters)
{
	FPuppetPerformerPose Pose;
	Pose.SetCurveNames(TArray<FName>({ TEXT("jawOpen"), TEXT("browInnerUp") }), nullptr);
	TestFalse(TEXT("One value for two curves"), Pose.SetCurveValues(TArray<float>({ 0.5f })));
	TestFalse(TEXT("No values yet"), Pose.HasCurveValues());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetCurvesCalibrationTest, "Open3DPuppet.Curves.Calibration.CapturesNeutralValues", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetCurvesCalibrationTest::RunTest(const FString& Parameters)
{
	PuppetTests::FPerformer Face;
	Face.SetCurve(TEXT("jawOpen"), 0.1f);
	Face.SetCurve(TEXT("browInnerUp"), 0.2f);
	const FPuppetPerformerCalibration Calibration = FPuppetPerformerCalibration::Capture(Face.ToPose(), FRotator::ZeroRotator);
	TestNearlyEqual(TEXT("jawOpen neutral"), Calibration.GetRestCurve(TEXT("jawOpen")), 0.1f);
	TestNearlyEqual(TEXT("browInnerUp neutral"), Calibration.GetRestCurve(TEXT("browInnerUp")), 0.2f);
	TestNearlyEqual(TEXT("A curve not captured is 0"), Calibration.GetRestCurve(TEXT("eyeBlinkLeft")), 0.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetCurvesOutputTest, "Open3DPuppet.Curves.PuppetPose.SetFindAndReset", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetCurvesOutputTest::RunTest(const FString& Parameters)
{
	FPuppetPose Pose;
	Pose.Initialize(PuppetTests::MakeReferenceSkeleton(PuppetTests::Biped()));
	Pose.SetCurve(TEXT("Snarl"), 0.5f);
	Pose.SetCurve(TEXT("Snarl"), 0.75f);
	const float* Snarl = Pose.FindCurve(TEXT("Snarl"));
	TestTrue(TEXT("Written"), Snarl != nullptr);
	TestNearlyEqual(TEXT("The last value written"), Snarl ? *Snarl : -1.0f, 0.75f);
	TestEqual(TEXT("One curve"), Pose.GetCurveNames().Num(), 1);
	Pose.ResetCurves();
	TestTrue(TEXT("Gone after a reset"), Pose.FindCurve(TEXT("Snarl")) == nullptr);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetCurvesSlotTest, "Open3DPuppet.Curves.Instance.CurveOnlySlotIsReady", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetCurvesSlotTest::RunTest(const FString& Parameters)
{
	// A face subject has curves and no bones (Basic role).
	PuppetTests::FPuppet Puppet(PuppetTests::Biped());
	Puppet.Build();
	PuppetTests::FPerformer Face;
	Face.SetCurve(TEXT("jawOpen"), 0.0f);
	Puppet.Drive(0, Face);
	TestTrue(TEXT("Curves count as data"), Puppet.Instance.GetSlotState(0) == EPuppetSlotState::NotCalibrated);
	Puppet.Calibrate(0, Face);
	TestTrue(TEXT("Ready once calibrated"), Puppet.Instance.GetSlotState(0) == EPuppetSlotState::Ready);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetCurvesModuleTest, "Open3DPuppet.Curves.Instance.ModuleWritesChangeFromNeutral", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetCurvesModuleTest::RunTest(const FString& Parameters)
{
	PuppetTests::FPuppet Puppet(PuppetTests::Biped());
	UPuppetTestModule_CurveCopy& Copy = Puppet.Add<UPuppetTestModule_CurveCopy>();
	Copy.PerformerCurve = TEXT("jawOpen");
	Copy.PuppetCurve = TEXT("Jaw");

	// The performer's neutral face has the jaw slightly open; only the change from it counts.
	PuppetTests::FPerformer Face;
	Face.SetCurve(TEXT("jawOpen"), 0.1f);
	Puppet.Build();
	Puppet.Drive(0, Face);
	Puppet.Calibrate(0, Face);
	Face.SetCurve(TEXT("jawOpen"), 0.6f);
	Puppet.Drive(0, Face);
	Puppet.Solve();

	const float* Jaw = Puppet.Instance.GetPose().FindCurve(TEXT("Jaw"));
	TestTrue(TEXT("The module wrote its curve"), Jaw != nullptr);
	TestNearlyEqual(TEXT("Change from neutral"), Jaw ? *Jaw : -1.0f, 0.5f, 1.e-5f);

	Copy.bEnabled = false;
	Puppet.Solve();
	TestTrue(TEXT("Not written this frame, so gone"), Puppet.Instance.GetPose().FindCurve(TEXT("Jaw")) == nullptr);
	return true;
}

#endif
