// Copyright 2026 Lifelike & Believable. All Rights Reserved.

// UPuppetModule_Chain: FK chains with rotation deltas and chain resampling (design overview §4).

#include "PuppetTestHarness.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Harness/PuppetTestFixtures.h"
#include "PuppetModules/PuppetModule_Chain.h"

namespace PuppetChainTests
{
	UPuppetModule_Chain& AddSpine(PuppetTests::FPuppet& Puppet)
	{
		UPuppetModule_Chain& Spine = Puppet.Add<UPuppetModule_Chain>();
		Spine.PerformerStart = TEXT("spine_01");
		Spine.PerformerEnd = TEXT("spine_03");
		Spine.PuppetStart = TEXT("spine_01");
		Spine.PuppetEnd = TEXT("spine_03");
		return Spine;
	}

	/** Calibrates slot 0 at rest, then drives it with spine_01 turned 30 degrees about X. */
	void SolveWithSpineTurned(PuppetTests::FPuppet& Puppet)
	{
		PuppetTests::FPerformer Performer(PuppetTests::Biped());
		Puppet.Build();
		Puppet.Drive(0, Performer);
		Puppet.Calibrate(0, Performer);
		Performer.SetLocalRotation(TEXT("spine_01"), PuppetTests::Rotation(FVector::XAxisVector, 30.0));
		Puppet.Drive(0, Performer);
		Puppet.Solve();
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetChainRestTest, "Open3DPuppet.Modules.Chain.RestMapsToReference", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetChainRestTest::RunTest(const FString& Parameters)
{
	PuppetTests::FPuppet Puppet(PuppetTests::Biped());
	PuppetChainTests::AddSpine(Puppet);
	PuppetTests::FPerformer Performer(PuppetTests::Biped());
	Puppet.Build();
	Puppet.Drive(0, Performer);
	Puppet.Calibrate(0, Performer);
	Puppet.Solve();
	TestEqual(TEXT("spine_03 at its reference"), Puppet.Component(TEXT("spine_03")), Puppet.Reference(TEXT("spine_03")), 1.e-3f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetChainFollowsTest, "Open3DPuppet.Modules.Chain.FollowsPerformerRotation", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetChainFollowsTest::RunTest(const FString& Parameters)
{
	PuppetTests::FPuppet Puppet(PuppetTests::Biped());
	PuppetChainTests::AddSpine(Puppet);
	PuppetChainTests::SolveWithSpineTurned(Puppet);

	const FQuat Turn = PuppetTests::Rotation(FVector::XAxisVector, 30.0);
	PuppetTests::TestRotation(*this, TEXT("spine_01"), Puppet.Component(TEXT("spine_01")).GetRotation(), Turn);
	PuppetTests::TestRotation(*this, TEXT("spine_03"), Puppet.Component(TEXT("spine_03")).GetRotation(), Turn);
	PuppetTests::TestRotation(*this, TEXT("head, outside the chain, follows its parent"), Puppet.Component(TEXT("head")).GetRotation(), Turn);
	TestEqual(TEXT("spine_02 keeps its length from spine_01"), Puppet.Component(TEXT("spine_02")).GetTranslation(),
		Puppet.Component(TEXT("spine_01")).GetTranslation() + Turn.RotateVector(FVector(0, 0, 10)), 1.e-3f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetChainAmplificationTest, "Open3DPuppet.Modules.Chain.AmplificationScalesRotation", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetChainAmplificationTest::RunTest(const FString& Parameters)
{
	PuppetTests::FPuppet Puppet(PuppetTests::Biped());
	PuppetChainTests::AddSpine(Puppet).Amplification = 2.0f;
	PuppetChainTests::SolveWithSpineTurned(Puppet);
	PuppetTests::TestRotation(*this, TEXT("Twice the turn"), Puppet.Component(TEXT("spine_03")).GetRotation(), PuppetTests::Rotation(FVector::XAxisVector, 60.0));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetChainWeightTest, "Open3DPuppet.Modules.Chain.WeightBlendsOverInput", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetChainWeightTest::RunTest(const FString& Parameters)
{
	PuppetTests::FPuppet Puppet(PuppetTests::Biped());
	PuppetChainTests::AddSpine(Puppet).Weight = 0.5f;
	PuppetChainTests::SolveWithSpineTurned(Puppet);
	PuppetTests::TestRotation(*this, TEXT("Half the turn"), Puppet.Component(TEXT("spine_03")).GetRotation(), PuppetTests::Rotation(FVector::XAxisVector, 15.0));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetChainResampleTest, "Open3DPuppet.Modules.Chain.ResamplesDifferentBoneCounts", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetChainResampleTest::RunTest(const FString& Parameters)
{
	// Three performer spine joints drive a five-bone tail. Only spine_03, the end, turns: the
	// tail's last bone turns fully, the one before it halfway, the middle not at all.
	PuppetTests::FPuppet Puppet(PuppetTests::Tail());
	UPuppetModule_Chain& Tail = Puppet.Add<UPuppetModule_Chain>();
	Tail.PerformerStart = TEXT("spine_01");
	Tail.PerformerEnd = TEXT("spine_03");
	Tail.PuppetStart = TEXT("tail_01");
	Tail.PuppetEnd = TEXT("tail_05");

	PuppetTests::FPerformer Performer(PuppetTests::Biped());
	Puppet.Build();
	Puppet.Drive(0, Performer);
	Puppet.Calibrate(0, Performer);
	Performer.SetLocalRotation(TEXT("spine_03"), PuppetTests::Rotation(FVector::XAxisVector, 90.0));
	Puppet.Drive(0, Performer);
	Puppet.Solve();

	PuppetTests::TestRotation(*this, TEXT("tail_03, the middle"), Puppet.Component(TEXT("tail_03")).GetRotation(), FQuat::Identity);
	PuppetTests::TestRotation(*this, TEXT("tail_04, three quarters"), Puppet.Component(TEXT("tail_04")).GetRotation(), PuppetTests::Rotation(FVector::XAxisVector, 45.0));
	PuppetTests::TestRotation(*this, TEXT("tail_05, the end"), Puppet.Component(TEXT("tail_05")).GetRotation(), PuppetTests::Rotation(FVector::XAxisVector, 90.0));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetChainMissingBoneTest, "Open3DPuppet.Modules.Chain.MissingBoneSkipsTheModule", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetChainMissingBoneTest::RunTest(const FString& Parameters)
{
	AddExpectedMessagePlain(TEXT("the puppet has no chain from 'spine_01' to 'spine_09'"), ELogVerbosity::Warning);
	PuppetTests::FPuppet Puppet(PuppetTests::Biped());
	PuppetChainTests::AddSpine(Puppet).PuppetEnd = TEXT("spine_09");
	PuppetChainTests::SolveWithSpineTurned(Puppet);
	TestEqual(TEXT("spine_01 left as the input"), Puppet.Component(TEXT("spine_01")), Puppet.Reference(TEXT("spine_01")), 1.e-3f);
	return true;
}

#endif
