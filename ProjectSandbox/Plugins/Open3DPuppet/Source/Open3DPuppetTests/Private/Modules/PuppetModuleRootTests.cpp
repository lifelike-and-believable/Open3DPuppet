// Copyright 2026 Lifelike & Believable. All Rights Reserved.

// UPuppetModule_Root: the pelvis, scaled by the puppet's size (design overview §4, ADR 0004).

#include "PuppetTestHarness.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Harness/PuppetTestFixtures.h"
#include "PuppetAsset.h"
#include "PuppetModules/PuppetModule_Root.h"

namespace PuppetRootTests
{
	/** Calibrates slot 0 at rest with Alignment, then drives it with the pelvis moved to PelvisLocal. */
	void SolveWithPelvis(PuppetTests::FPuppet& Puppet, const FTransform& PelvisLocal, const FRotator& Alignment = FRotator::ZeroRotator)
	{
		PuppetTests::FPerformer Performer(PuppetTests::Biped());
		Puppet.Build();
		Puppet.Drive(0, Performer);
		Puppet.Calibrate(0, Performer, Alignment);
		Performer.SetLocalTranslation(TEXT("pelvis"), PelvisLocal.GetTranslation());
		Performer.SetLocalRotation(TEXT("pelvis"), PelvisLocal.GetRotation());
		Puppet.Drive(0, Performer);
		Puppet.Solve();
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetRootScaleTest, "Open3DPuppet.Modules.Root.ScalesTranslationByPelvisHeight", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetRootScaleTest::RunTest(const FString& Parameters)
{
	// A puppet twice the performer's size covers twice the distance.
	PuppetTests::FPuppet Puppet(PuppetTests::Scaled(PuppetTests::Biped(), 2.0));
	Puppet.Add<UPuppetModule_Root>();
	PuppetRootTests::SolveWithPelvis(Puppet, FTransform(FVector(10, 0, 100)));
	TestEqual(TEXT("Pelvis"), Puppet.Component(TEXT("pelvis")).GetTranslation(), FVector(20, 0, 200), 1.e-3f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetRootRotationTest, "Open3DPuppet.Modules.Root.FollowsPelvisRotation", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetRootRotationTest::RunTest(const FString& Parameters)
{
	PuppetTests::FPuppet Puppet(PuppetTests::Biped());
	Puppet.Add<UPuppetModule_Root>();
	const FQuat Turn = PuppetTests::Rotation(FVector::ZAxisVector, 45.0);
	PuppetRootTests::SolveWithPelvis(Puppet, FTransform(Turn, FVector(0, 0, 100)));
	PuppetTests::TestRotation(*this, TEXT("Pelvis"), Puppet.Component(TEXT("pelvis")).GetRotation(), Turn);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetRootVerticalTest, "Open3DPuppet.Modules.Root.VerticalScaleAndHeightOffset", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetRootVerticalTest::RunTest(const FString& Parameters)
{
	PuppetTests::FPuppet Puppet(PuppetTests::Scaled(PuppetTests::Biped(), 2.0));
	UPuppetModule_Root& Root = Puppet.Add<UPuppetModule_Root>();
	Root.VerticalScale = 0.5f;
	Root.HeightOffset = 3.0f;
	// A 10 cm crouch: root scale 2 times vertical scale 0.5 is 10 cm down, then 3 cm up.
	PuppetRootTests::SolveWithPelvis(Puppet, FTransform(FVector(0, 0, 90)));
	TestEqual(TEXT("Pelvis"), Puppet.Component(TEXT("pelvis")).GetTranslation(), FVector(0, 0, 193), 1.e-3f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetRootAlignmentTest, "Open3DPuppet.Modules.Root.AlignmentTurnsTheStage", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetRootAlignmentTest::RunTest(const FString& Parameters)
{
	// Calibrated facing 90 degrees away: stepping along the performer's X moves the puppet along Y.
	PuppetTests::FPuppet Puppet(PuppetTests::Biped());
	Puppet.Add<UPuppetModule_Root>();
	PuppetRootTests::SolveWithPelvis(Puppet, FTransform(FVector(10, 0, 100)), FRotator(0.0, 90.0, 0.0));
	TestEqual(TEXT("Pelvis"), Puppet.Component(TEXT("pelvis")).GetTranslation(), FVector(0, 10, 100), 1.e-3f);
	PuppetTests::TestRotation(*this, TEXT("No turn"), Puppet.Component(TEXT("pelvis")).GetRotation(), FQuat::Identity);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetRootSlotTest, "Open3DPuppet.Modules.Root.OnlyTheRootSlotMovesThePuppet", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetRootSlotTest::RunTest(const FString& Parameters)
{
	AddExpectedMessagePlain(TEXT("only a module on the root slot may drive translation"), ELogVerbosity::Warning);
	PuppetTests::FPuppet Puppet(PuppetTests::Biped());
	Puppet.Asset->PerformerSlots = { TEXT("Body"), TEXT("Tail") };
	Puppet.Add<UPuppetModule_Root>().PerformerSlot = TEXT("Tail");

	PuppetTests::FPerformer Performer(PuppetTests::Biped());
	Puppet.Build();
	Puppet.Drive(1, Performer);
	Puppet.Calibrate(1, Performer);
	Performer.SetLocalTranslation(TEXT("pelvis"), FVector(10, 0, 100));
	Puppet.Drive(1, Performer);
	Puppet.Solve();
	TestEqual(TEXT("Pelvis stays"), Puppet.Component(TEXT("pelvis")).GetTranslation(), FVector(0, 0, 100), 1.e-3f);
	return true;
}

#endif
