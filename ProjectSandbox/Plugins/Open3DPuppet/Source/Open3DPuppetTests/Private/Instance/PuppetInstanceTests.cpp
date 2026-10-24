// Copyright 2026 Lifelike & Believable. All Rights Reserved.

// FPuppetInstance: performer slots, calibration state and several performers (ADR 0003, ADR 0004).

#include "PuppetTestHarness.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Harness/PuppetTestFixtures.h"
#include "PuppetAsset.h"
#include "PuppetModules/PuppetModule_Chain.h"

namespace PuppetInstanceTests
{
	UPuppetModule_Chain& AddChain(PuppetTests::FPuppet& Puppet, FName Slot, FName Start, FName End)
	{
		UPuppetModule_Chain& Chain = Puppet.Add<UPuppetModule_Chain>();
		Chain.PerformerSlot = Slot;
		Chain.PerformerStart = Chain.PuppetStart = Start;
		Chain.PerformerEnd = Chain.PuppetEnd = End;
		return Chain;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetInstanceSlotStateTest, "Open3DPuppet.Instance.Slots.StateFollowsDataAndCalibration", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetInstanceSlotStateTest::RunTest(const FString& Parameters)
{
	PuppetTests::FPuppet Puppet(PuppetTests::Biped());
	Puppet.Build();
	TestEqual(TEXT("One slot"), Puppet.Instance.NumSlots(), 1);
	TestTrue(TEXT("No data yet"), Puppet.Instance.GetSlotState(0) == EPuppetSlotState::NoData);

	PuppetTests::FPerformer Performer(PuppetTests::Biped());
	Puppet.Drive(0, Performer);
	TestTrue(TEXT("Data, no calibration"), Puppet.Instance.GetSlotState(0) == EPuppetSlotState::NotCalibrated);

	Puppet.Calibrate(0, Performer);
	TestTrue(TEXT("Ready"), Puppet.Instance.GetSlotState(0) == EPuppetSlotState::Ready);

	Puppet.Drive(0, PuppetTests::FPerformer(PuppetTests::Tail()));
	TestTrue(TEXT("A different skeleton"), Puppet.Instance.GetSlotState(0) == EPuppetSlotState::CalibrationMismatch);

	Puppet.Instance.ClearCalibration(0);
	TestTrue(TEXT("Calibration cleared"), Puppet.Instance.GetSlotState(0) == EPuppetSlotState::NotCalibrated);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetInstanceNotCalibratedTest, "Open3DPuppet.Instance.Slots.UncalibratedPerformerLeavesTheInput", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetInstanceNotCalibratedTest::RunTest(const FString& Parameters)
{
	PuppetTests::FPuppet Puppet(PuppetTests::Biped());
	PuppetInstanceTests::AddChain(Puppet, TEXT("Body"), TEXT("spine_01"), TEXT("spine_03"));
	PuppetTests::FPerformer Performer(PuppetTests::Biped());
	Performer.SetLocalRotation(TEXT("spine_01"), PuppetTests::Rotation(FVector::XAxisVector, 30.0));
	Puppet.Build();
	Puppet.Drive(0, Performer);
	Puppet.Solve();
	TestEqual(TEXT("spine_03 left as the input"), Puppet.Component(TEXT("spine_03")), Puppet.Reference(TEXT("spine_03")), 1.e-3f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetInstanceTwoPerformersTest, "Open3DPuppet.Instance.Slots.TwoPerformersDriveTheirOwnModules", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetInstanceTwoPerformersTest::RunTest(const FString& Parameters)
{
	// Front drives the spine, Back the neck and head. Only Back's performer moves.
	PuppetTests::FPuppet Puppet(PuppetTests::Biped());
	Puppet.Asset->PerformerSlots = { TEXT("Front"), TEXT("Back") };
	Puppet.Asset->RootSlot = TEXT("Front");
	PuppetInstanceTests::AddChain(Puppet, TEXT("Front"), TEXT("spine_01"), TEXT("spine_03"));
	PuppetInstanceTests::AddChain(Puppet, TEXT("Back"), TEXT("neck_01"), TEXT("head"));

	PuppetTests::FPerformer Front(PuppetTests::Biped());
	PuppetTests::FPerformer Back(PuppetTests::Biped());
	Puppet.Build();
	const int32 FrontSlot = Puppet.Instance.FindSlot(TEXT("Front"));
	const int32 BackSlot = Puppet.Instance.FindSlot(TEXT("Back"));
	Puppet.Drive(FrontSlot, Front);
	Puppet.Calibrate(FrontSlot, Front);
	Puppet.Drive(BackSlot, Back);
	Puppet.Calibrate(BackSlot, Back);

	const FQuat Turn = PuppetTests::Rotation(FVector::ZAxisVector, 30.0);
	Back.SetLocalRotation(TEXT("neck_01"), Turn);
	Puppet.Drive(BackSlot, Back);
	Puppet.Solve();

	PuppetTests::TestRotation(*this, TEXT("spine_03, Front at rest"), Puppet.Component(TEXT("spine_03")).GetRotation(), FQuat::Identity);
	PuppetTests::TestRotation(*this, TEXT("head, Back turned"), Puppet.Component(TEXT("head")).GetRotation(), Turn);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetInstanceUndeclaredSlotTest, "Open3DPuppet.Instance.Slots.UndeclaredSlotModuleDoesNotRun", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetInstanceUndeclaredSlotTest::RunTest(const FString& Parameters)
{
	AddExpectedMessagePlain(TEXT("reads slot 'Wing', which the puppet does not declare"), ELogVerbosity::Warning);
	PuppetTests::FPuppet Puppet(PuppetTests::Biped());
	PuppetInstanceTests::AddChain(Puppet, TEXT("Wing"), TEXT("spine_01"), TEXT("spine_03"));
	PuppetTests::FPerformer Performer(PuppetTests::Biped());
	Puppet.Build();
	Puppet.Drive(0, Performer);
	Puppet.Calibrate(0, Performer);
	Performer.SetLocalRotation(TEXT("spine_01"), PuppetTests::Rotation(FVector::XAxisVector, 30.0));
	Puppet.Drive(0, Performer);
	Puppet.Solve();
	TestEqual(TEXT("spine_03 left as the input"), Puppet.Component(TEXT("spine_03")), Puppet.Reference(TEXT("spine_03")), 1.e-3f);
	return true;
}

#endif
