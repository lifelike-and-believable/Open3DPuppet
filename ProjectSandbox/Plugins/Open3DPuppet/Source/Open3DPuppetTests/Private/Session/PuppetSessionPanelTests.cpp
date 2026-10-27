// Copyright 2026 Lifelike & Believable. All Rights Reserved.

// The Puppet Session panel (WP-8): its model of the session's performers and running puppets, and
// its tab. The widgets themselves need the editor's UI and are checked at the desk.

#include "PuppetTestHarness.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Engine/Engine.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Docking/TabManager.h"
#include "Harness/PuppetTestFixtures.h"
#include "PuppetCalibrationSubsystem.h"
#include "PuppetInstance.h"
#include "PuppetSession.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetSessionPerformersTest, "Open3DPuppet.Session.Panel.PerformersIncludeCalibratedSubjects", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetSessionPerformersTest::RunTest(const FString& Parameters)
{
	UPuppetCalibrationSubsystem* Calibrations = GEngine ? GEngine->GetEngineSubsystem<UPuppetCalibrationSubsystem>() : nullptr;
	if (!TestNotNull(TEXT("The calibration subsystem"), Calibrations))
	{
		return false;
	}
	FLiveLinkSubjectName B;
	B.Name = TEXT("Open3DPuppetTest_SessionB");
	FLiveLinkSubjectName A;
	A.Name = TEXT("Open3DPuppetTest_SessionA");
	const FPuppetPerformerCalibration Calibration = FPuppetPerformerCalibration::Capture(PuppetTests::FPerformer(PuppetTests::Biped()).ToPose(), FRotator::ZeroRotator);
	Calibrations->SetCalibration(B, Calibration);
	Calibrations->SetCalibration(A, Calibration);

	TestTrue(TEXT("The subsystem lists its calibrated subjects"), Calibrations->GetCalibratedSubjects().Contains(A.Name));
	const TArray<PuppetSession::FPerformer> Performers = PuppetSession::GatherPerformers(nullptr, Calibrations);
	const int32 IndexA = Performers.IndexOfByPredicate([&A](const PuppetSession::FPerformer& P) { return P.Subject == A.Name; });
	const int32 IndexB = Performers.IndexOfByPredicate([&B](const PuppetSession::FPerformer& P) { return P.Subject == B.Name; });
	if (TestTrue(TEXT("A calibrated subject is listed without Live Link"), IndexA != INDEX_NONE && IndexB != INDEX_NONE))
	{
		TestTrue(TEXT("As calibrated"), Performers[IndexA].bCalibrated);
		TestFalse(TEXT("And not streaming"), Performers[IndexA].bStreaming);
		TestTrue(TEXT("Sorted by name"), IndexA < IndexB);
	}
	Calibrations->ClearCalibration(A);
	Calibrations->ClearCalibration(B);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetSessionWorldTest, "Open3DPuppet.Session.Panel.FindsTheSessionWorldAndItsPuppets", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetSessionWorldTest::RunTest(const FString& Parameters)
{
	TestNotNull(TEXT("The editor has a session world"), PuppetSession::SessionWorld());
	TestEqual(TEXT("No world, no puppets"), PuppetSession::GatherPuppets(nullptr).Num(), 0);
	TestTrue(TEXT("Each slot state has its own wording"),
		!PuppetSession::SlotStateText(static_cast<int32>(EPuppetSlotState::Ready)).IsEmpty()
		&& PuppetSession::SlotStateText(static_cast<int32>(EPuppetSlotState::Ready)) != PuppetSession::SlotStateText(static_cast<int32>(EPuppetSlotState::NotCalibrated)));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetSessionTabTest, "Open3DPuppet.Session.Panel.TheTabIsRegistered", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetSessionTabTest::RunTest(const FString& Parameters)
{
	if (!FSlateApplication::IsInitialized())
	{
		AddInfo(TEXT("Slate is not initialised in this run, so the editor module registers no tab."));
		return true;
	}
	TestTrue(TEXT("Window > Tools > Puppet Session"), FGlobalTabmanager::Get()->HasTabSpawner(TEXT("PuppetSession")));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
