// Copyright 2026 Lifelike & Believable. All Rights Reserved.

// UPuppetCalibrationAsset (WP-2, ADR 0003): a calibration saved in an asset, applied to the Puppet
// Calibration subsystem, and captured from a Live Link subject.

#include "PuppetTestHarness.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Engine/Engine.h"
#include "Harness/PuppetTestFixtures.h"
#include "PuppetCalibrationAsset.h"
#include "PuppetCalibrationSubsystem.h"

namespace PuppetCalibrationAssetTests
{
	/** A calibration of the biped at rest, with Alignment. */
	FPuppetPerformerCalibration Biped(const FRotator& Alignment = FRotator::ZeroRotator)
	{
		return FPuppetPerformerCalibration::Capture(PuppetTests::FPerformer(PuppetTests::Biped()).ToPose(), Alignment);
	}

	UPuppetCalibrationSubsystem* Subsystem()
	{
		return GEngine ? GEngine->GetEngineSubsystem<UPuppetCalibrationSubsystem>() : nullptr;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetCalibrationAssetSaveTest, "Open3DPuppet.Calibration.Asset.SavesACalibration", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetCalibrationAssetSaveTest::RunTest(const FString& Parameters)
{
	UPuppetCalibrationAsset* Asset = NewObject<UPuppetCalibrationAsset>();
	TestFalse(TEXT("Nothing saved at first"), Asset->HasCalibration());

	const int32 Before = Asset->GetRevision();
	Asset->SetCalibration(PuppetCalibrationAssetTests::Biped(FRotator(0.0, 90.0, 0.0)));
	TestTrue(TEXT("Saved"), Asset->HasCalibration());
	TestEqual(TEXT("Its bones"), Asset->GetCalibration().BoneNames.Num(), PuppetTests::Biped().Num());
	TestEqual(TEXT("Its alignment"), Asset->GetCalibration().Alignment, FRotator(0.0, 90.0, 0.0));
	TestNotEqual(TEXT("A new calibration changes the revision"), Asset->GetRevision(), Before);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetCalibrationAssetApplyTest, "Open3DPuppet.Calibration.Asset.ApplyMakesItTheSubjectsCalibration", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetCalibrationAssetApplyTest::RunTest(const FString& Parameters)
{
	UPuppetCalibrationSubsystem* Calibrations = PuppetCalibrationAssetTests::Subsystem();
	if (!TestNotNull(TEXT("The calibration subsystem"), Calibrations))
	{
		return false;
	}
	UPuppetCalibrationAsset* Asset = NewObject<UPuppetCalibrationAsset>();
	Asset->Subject.Name = TEXT("Open3DPuppetTest_ApplySubject");
	Calibrations->ClearCalibration(Asset->Subject);

	Asset->Apply();
	FPuppetPerformerCalibration Found;
	TestFalse(TEXT("Applying nothing saved calibrates nothing"), Calibrations->GetCalibration(Asset->Subject, Found));

	Asset->SetCalibration(PuppetCalibrationAssetTests::Biped(FRotator(0.0, 45.0, 0.0)));
	Asset->Apply();
	TestTrue(TEXT("The subject is calibrated"), Calibrations->GetCalibration(Asset->Subject, Found));
	TestEqual(TEXT("With the saved bones"), Found.BoneNames, Asset->GetCalibration().BoneNames);
	TestEqual(TEXT("And alignment"), Found.Alignment, FRotator(0.0, 45.0, 0.0));
	Calibrations->ClearCalibration(Asset->Subject);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetCalibrationAssetCaptureTest, "Open3DPuppet.Calibration.Asset.CaptureWithoutAFrameKeepsTheSaved", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetCalibrationAssetCaptureTest::RunTest(const FString& Parameters)
{
	AddExpectedMessagePlain(TEXT("Cannot calibrate Open3DPuppetTest_MissingSubject"), ELogVerbosity::Warning);
	UPuppetCalibrationAsset* Asset = NewObject<UPuppetCalibrationAsset>();
	Asset->Subject.Name = TEXT("Open3DPuppetTest_MissingSubject");
	Asset->SetCalibration(PuppetCalibrationAssetTests::Biped(FRotator(0.0, 30.0, 0.0)));
	const int32 Before = Asset->GetRevision();

	Asset->Capture();
	TestTrue(TEXT("Still saved"), Asset->HasCalibration());
	TestEqual(TEXT("Unchanged"), Asset->GetCalibration().Alignment, FRotator(0.0, 30.0, 0.0));
	TestEqual(TEXT("Same revision"), Asset->GetRevision(), Before);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
