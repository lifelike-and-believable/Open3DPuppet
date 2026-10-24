// Copyright 2026 Lifelike & Believable. All Rights Reserved.

// Derived finger-curl curves (ADR 0009): curl_<finger>_<side>, the summed unsigned bend of a
// finger's three joints relative to their parents since calibration, over 270 degrees. In the Hand
// fixture fingers point along +Y with the palm down, so a curl is a turn about X.

#include "PuppetTestHarness.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Harness/PuppetTestFixtures.h"
#include "Harness/PuppetTestModules.h"
#include "PuppetPerformerCalibration.h"
#include "PuppetPerformerPose.h"
#include "PuppetPose.h"

namespace PuppetFingerCurlTests
{
	/** A puppet whose only module copies curl_index_l's change from neutral to the output curve "Curl". */
	void AddCurlCopy(PuppetTests::FPuppet& Puppet)
	{
		UPuppetTestModule_CurveCopy& Copy = Puppet.Add<UPuppetTestModule_CurveCopy>();
		Copy.PerformerCurve = TEXT("curl_index_l");
		Copy.PuppetCurve = TEXT("Curl");
	}

	float Curl(const PuppetTests::FPuppet& Puppet)
	{
		const float* Value = Puppet.Instance.GetPose().FindCurve(TEXT("Curl"));
		return Value ? *Value : -1.0f;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetFingerCurlNamesTest, "Open3DPuppet.Curves.FingerCurl.OnePerFingerWithJoints", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetFingerCurlNamesTest::RunTest(const FString& Parameters)
{
	const FPuppetPerformerPose Hand = PuppetTests::FPerformer(PuppetTests::Hand()).ToPose();
	TestTrue(TEXT("curl_thumb_l"), Hand.FindCurve(TEXT("curl_thumb_l")) != INDEX_NONE);
	TestTrue(TEXT("curl_index_l"), Hand.FindCurve(TEXT("curl_index_l")) != INDEX_NONE);
	TestTrue(TEXT("curl_pinky_l"), Hand.FindCurve(TEXT("curl_pinky_l")) != INDEX_NONE);
	TestEqual(TEXT("No right hand, no curl_index_r"), Hand.FindCurve(TEXT("curl_index_r")), INDEX_NONE);
	TestEqual(TEXT("Five derived curves"), Hand.GetCurveNames().Num(), 5);
	TestEqual(TEXT("No source curves"), Hand.GetSourceCurveNames().Num(), 0);

	const FPuppetPerformerPose Body = PuppetTests::FPerformer(PuppetTests::Biped()).ToPose();
	TestEqual(TEXT("No finger bones, no curl"), Body.FindCurve(TEXT("curl_index_l")), INDEX_NONE);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetFingerCurlSourceTest, "Open3DPuppet.Curves.FingerCurl.SourceCurveWins", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetFingerCurlSourceTest::RunTest(const FString& Parameters)
{
	PuppetTests::FPerformer Performer(PuppetTests::Hand());
	Performer.SetCurve(TEXT("curl_index_l"), 0.7f);
	Performer.SetCurve(TEXT("jawOpen"), 0.2f);
	FPuppetPerformerPose Pose = Performer.ToPose();
	TestEqual(TEXT("The source's curl_index_l"), Pose.FindCurve(TEXT("curl_index_l")), 0);
	TestEqual(TEXT("Two source curves and four derived"), Pose.GetCurveNames().Num(), 6);
	TestTrue(TEXT("Values counted against the source curves"), Pose.SetCurveValues(TArray<float>({ 0.5f, 0.25f })));
	TestNearlyEqual(TEXT("The source's value"), Pose.GetCurveValues()[0], 0.5f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetFingerCurlCalibrationTest, "Open3DPuppet.Curves.FingerCurl.CalibrationCapturesSourceCurvesOnly", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetFingerCurlCalibrationTest::RunTest(const FString& Parameters)
{
	PuppetTests::FPerformer Performer(PuppetTests::Hand());
	Performer.SetCurve(TEXT("jawOpen"), 0.2f);
	const FPuppetPerformerCalibration Calibration = FPuppetPerformerCalibration::Capture(Performer.ToPose(), FRotator::ZeroRotator);
	TestEqual(TEXT("Only jawOpen captured"), Calibration.CurveNames.Num(), 1);
	TestNearlyEqual(TEXT("A derived curve's neutral is 0"), Calibration.GetRestCurve(TEXT("curl_index_l")), 0.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetFingerCurlFistTest, "Open3DPuppet.Curves.FingerCurl.FistIsOneWhateverTheHand", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetFingerCurlFistTest::RunTest(const FString& Parameters)
{
	// Calibrated flat: 0. Then the wrist turns 90 degrees about the forearm and each index joint
	// bends 90 degrees in its parent: 3 x 90 / 270 = 1, the wrist not counted.
	PuppetTests::FPuppet Puppet(PuppetTests::Biped());
	PuppetFingerCurlTests::AddCurlCopy(Puppet);
	PuppetTests::FPerformer Performer(PuppetTests::Hand());
	Puppet.Build();
	Puppet.Drive(0, Performer);
	Puppet.Calibrate(0, Performer);
	Puppet.Solve();
	TestNearlyEqual(TEXT("Flat, as calibrated"), PuppetFingerCurlTests::Curl(Puppet), 0.0f, 1.e-4f);

	const FQuat Bend = PuppetTests::Rotation(FVector::XAxisVector, 90.0);
	Performer.SetLocalRotation(TEXT("hand_l"), PuppetTests::Rotation(FVector::YAxisVector, 90.0));
	Performer.SetLocalRotation(TEXT("index_01_l"), Bend);
	Performer.SetLocalRotation(TEXT("index_02_l"), Bend);
	Performer.SetLocalRotation(TEXT("index_03_l"), Bend);
	Puppet.Drive(0, Performer);
	Puppet.Solve();
	TestNearlyEqual(TEXT("A fist"), PuppetFingerCurlTests::Curl(Puppet), 1.0f, 1.e-4f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetFingerCurlCalibratedTest, "Open3DPuppet.Curves.FingerCurl.MeasuredFromCalibrationUnsigned", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetFingerCurlCalibratedTest::RunTest(const FString& Parameters)
{
	// Calibrated with index_01_l bent 30 degrees. Bending it to 60 and index_02_l to 45 is
	// (30 + 45) / 270; straightening index_01_l to 0 is a bend of 30 back, which also counts: 30 / 270.
	PuppetTests::FPuppet Puppet(PuppetTests::Biped());
	PuppetFingerCurlTests::AddCurlCopy(Puppet);
	PuppetTests::FPerformer Performer(PuppetTests::Hand());
	Performer.SetLocalRotation(TEXT("index_01_l"), PuppetTests::Rotation(FVector::XAxisVector, 30.0));
	Puppet.Build();
	Puppet.Drive(0, Performer);
	Puppet.Calibrate(0, Performer);

	Performer.SetLocalRotation(TEXT("index_01_l"), PuppetTests::Rotation(FVector::XAxisVector, 60.0));
	Performer.SetLocalRotation(TEXT("index_02_l"), PuppetTests::Rotation(FVector::XAxisVector, 45.0));
	Puppet.Drive(0, Performer);
	Puppet.Solve();
	TestNearlyEqual(TEXT("Curled from calibration"), PuppetFingerCurlTests::Curl(Puppet), 75.0f / 270.0f, 1.e-4f);

	Performer.SetLocalRotation(TEXT("index_01_l"), FQuat::Identity);
	Performer.SetLocalRotation(TEXT("index_02_l"), FQuat::Identity);
	Puppet.Drive(0, Performer);
	Puppet.Solve();
	TestNearlyEqual(TEXT("Bent back from calibration"), PuppetFingerCurlTests::Curl(Puppet), 30.0f / 270.0f, 1.e-4f);
	return true;
}

#endif
