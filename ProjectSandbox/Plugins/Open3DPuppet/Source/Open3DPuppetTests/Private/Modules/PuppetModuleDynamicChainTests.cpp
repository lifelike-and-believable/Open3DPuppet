// Copyright 2026 Lifelike & Believable. All Rights Reserved.

// UPuppetModule_DynamicChain: a tail that swings with the puppet's motion and settles (ADR 0007,
// Tier 1). The Tail fixture's tail stands straight up from tail_01 at (0, 0, 110) to tail_05 at
// (0, 0, 150), 10 cm a bone. Frames are 1/30 s; the simulation steps at 120 Hz.

#include "PuppetTestHarness.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Harness/PuppetTestFixtures.h"
#include "PuppetModules/PuppetModule_DynamicChain.h"

namespace PuppetDynamicChainTests
{
	/** A tail with no gravity and no springs unless a test sets them. */
	UPuppetModule_DynamicChain& AddTail(PuppetTests::FPuppet& Puppet)
	{
		UPuppetModule_DynamicChain& Tail = Puppet.Add<UPuppetModule_DynamicChain>();
		Tail.Gravity = FVector::ZeroVector;
		Tail.Stiffness = 0.0f;
		Tail.Damping = 0.0f;
		return Tail;
	}

	/** Builds the puppet and calibrates a standing performer. */
	PuppetTests::FPerformer Start(PuppetTests::FPuppet& Puppet)
	{
		PuppetTests::FPerformer Performer(PuppetTests::Biped());
		Puppet.Build();
		Puppet.Drive(0, Performer);
		Puppet.Calibrate(0, Performer);
		return Performer;
	}

	/** Solves Frames frames of 1/30 s with the puppet's component at Location. */
	void SolveAt(PuppetTests::FPuppet& Puppet, const FVector& Location, int32 Frames = 1)
	{
		for (int32 Frame = 0; Frame < Frames; ++Frame)
		{
			Puppet.Instance.Solve(Puppet.RefSkeleton.GetRefBonePose(), 1.0f / 30.0f, FTransform(Location));
		}
	}

	/** The tail's tip after one frame standing still, then one frame 10 cm along +X. */
	FVector TipAfterAStep(PuppetTests::FPuppet& Puppet)
	{
		Start(Puppet);
		SolveAt(Puppet, FVector::ZeroVector);
		SolveAt(Puppet, FVector(10, 0, 0));
		return Puppet.Component(TEXT("tail_05")).GetTranslation();
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetDynamicChainRestTest, "Open3DPuppet.Modules.DynamicChain.AtRestStaysOnThePose", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetDynamicChainRestTest::RunTest(const FString& Parameters)
{
	PuppetTests::FPuppet Puppet(PuppetTests::Tail());
	PuppetDynamicChainTests::AddTail(Puppet);
	PuppetDynamicChainTests::Start(Puppet);
	PuppetDynamicChainTests::SolveAt(Puppet, FVector::ZeroVector, 10);
	TestEqual(TEXT("tail_05 at its reference"), Puppet.Component(TEXT("tail_05")), Puppet.Reference(TEXT("tail_05")), 1.e-3f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetDynamicChainLagTest, "Open3DPuppet.Modules.DynamicChain.LagsWhenThePuppetMoves", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetDynamicChainLagTest::RunTest(const FString& Parameters)
{
	// The puppet steps 10 cm forward in one frame: the free tail stays behind in the world, so its
	// tip is behind (-X) the pose in component space.
	PuppetTests::FPuppet Puppet(PuppetTests::Tail());
	PuppetDynamicChainTests::AddTail(Puppet);
	const FVector Tip = PuppetDynamicChainTests::TipAfterAStep(Puppet);
	TestTrue(TEXT("Tip lags behind"), Tip.X < -1.0);
	TestEqual(TEXT("tail_01 stays on the pose"), Puppet.Component(TEXT("tail_01")).GetTranslation(), Puppet.Reference(TEXT("tail_01")).GetTranslation(), 1.e-3f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetDynamicChainSettleTest, "Open3DPuppet.Modules.DynamicChain.SettlesBackOnThePose", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetDynamicChainSettleTest::RunTest(const FString& Parameters)
{
	// With springs and damping, three seconds after the step the tail is back on the pose.
	PuppetTests::FPuppet Puppet(PuppetTests::Tail());
	UPuppetModule_DynamicChain& Tail = PuppetDynamicChainTests::AddTail(Puppet);
	Tail.Stiffness = 0.05f;
	Tail.Damping = 5.0f;
	PuppetDynamicChainTests::TipAfterAStep(Puppet);
	PuppetDynamicChainTests::SolveAt(Puppet, FVector(10, 0, 0), 90);
	TestEqual(TEXT("tail_05 back at its reference"), Puppet.Component(TEXT("tail_05")).GetTranslation(), Puppet.Reference(TEXT("tail_05")).GetTranslation(), 0.1f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetDynamicChainPhysicsWeightTest, "Open3DPuppet.Modules.DynamicChain.PhysicsWeightZeroFollowsThePose", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetDynamicChainPhysicsWeightTest::RunTest(const FString& Parameters)
{
	PuppetTests::FPuppet Puppet(PuppetTests::Tail());
	PuppetDynamicChainTests::AddTail(Puppet).PhysicsWeight.Weight = 0.0f;
	const FVector Tip = PuppetDynamicChainTests::TipAfterAStep(Puppet);
	TestEqual(TEXT("Tip on the pose"), Tip, Puppet.Reference(TEXT("tail_05")).GetTranslation(), 1.e-3f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetDynamicChainAmplitudeTest, "Open3DPuppet.Modules.DynamicChain.AmplitudeScalesTheSwing", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetDynamicChainAmplitudeTest::RunTest(const FString& Parameters)
{
	// Twice the swing: the bones keep their lengths, so the tip moves less than twice as far, but
	// clearly further.
	PuppetTests::FPuppet Normal(PuppetTests::Tail());
	PuppetDynamicChainTests::AddTail(Normal);
	const double NormalSwing = -PuppetDynamicChainTests::TipAfterAStep(Normal).X;
	PuppetTests::FPuppet Doubled(PuppetTests::Tail());
	PuppetDynamicChainTests::AddTail(Doubled).Amplitude = 2.0f;
	const double DoubledSwing = -PuppetDynamicChainTests::TipAfterAStep(Doubled).X;
	TestTrue(TEXT("Swings"), NormalSwing > 1.0);
	TestTrue(TEXT("Further with Amplitude 2"), DoubledSwing > 1.3 * NormalSwing);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetDynamicChainGravityTest, "Open3DPuppet.Modules.DynamicChain.GravityBendsTheTail", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetDynamicChainGravityTest::RunTest(const FString& Parameters)
{
	// Gravity along +X, no springs, damped: after three seconds the tail lies along +X from tail_01.
	PuppetTests::FPuppet Puppet(PuppetTests::Tail());
	UPuppetModule_DynamicChain& Tail = PuppetDynamicChainTests::AddTail(Puppet);
	Tail.Gravity = FVector(980, 0, 0);
	Tail.Damping = 5.0f;
	PuppetDynamicChainTests::Start(Puppet);
	PuppetDynamicChainTests::SolveAt(Puppet, FVector::ZeroVector, 90);
	TestTrue(TEXT("Tip along +X"), Puppet.Component(TEXT("tail_05")).GetTranslation().X > 35.0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetDynamicChainGroundTest, "Open3DPuppet.Modules.DynamicChain.GroundHoldsTheTailUp", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetDynamicChainGroundTest::RunTest(const FString& Parameters)
{
	// As above, with the ground at 112 cm and a 1 cm thick tail: it lies on the ground at 113, not
	// at tail_01's 110.
	PuppetTests::FPuppet Puppet(PuppetTests::Tail());
	UPuppetModule_DynamicChain& Tail = PuppetDynamicChainTests::AddTail(Puppet);
	Tail.Gravity = FVector(980, 0, 0);
	Tail.Damping = 5.0f;
	Tail.bCollideWithGround = true;
	Tail.GroundHeight = 112.0f;
	Tail.Radius = 1.0f;
	PuppetDynamicChainTests::Start(Puppet);
	PuppetDynamicChainTests::SolveAt(Puppet, FVector::ZeroVector, 90);
	const FVector Tip = Puppet.Component(TEXT("tail_05")).GetTranslation();
	TestTrue(TEXT("Tip along +X"), Tip.X > 30.0);
	TestTrue(TEXT("Tip on the ground"), Tip.Z > 112.5);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetDynamicChainInputMixTest, "Open3DPuppet.Modules.DynamicChain.InputMixFollowsThePerformerChain", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetDynamicChainInputMixTest::RunTest(const FString& Parameters)
{
	// The performer's spine drives the tail as the Chain module would: spine_03 turns 90 degrees
	// about X, so tail_03 stays, tail_04 turns 45 and tail_05 90. On the first frame the simulation
	// starts on that pose.
	PuppetTests::FPuppet Puppet(PuppetTests::Tail());
	UPuppetModule_DynamicChain& Tail = PuppetDynamicChainTests::AddTail(Puppet);
	Tail.PerformerStart = TEXT("spine_01");
	Tail.PerformerEnd = TEXT("spine_03");
	Tail.InputMix = 1.0f;
	PuppetTests::FPerformer Performer = PuppetDynamicChainTests::Start(Puppet);
	Performer.SetLocalRotation(TEXT("spine_03"), PuppetTests::Rotation(FVector::XAxisVector, 90.0));
	Puppet.Drive(0, Performer);
	PuppetDynamicChainTests::SolveAt(Puppet, FVector::ZeroVector);
	PuppetTests::TestRotation(*this, TEXT("tail_03"), Puppet.Component(TEXT("tail_03")).GetRotation(), FQuat::Identity);
	PuppetTests::TestRotation(*this, TEXT("tail_04"), Puppet.Component(TEXT("tail_04")).GetRotation(), PuppetTests::Rotation(FVector::XAxisVector, 45.0));
	PuppetTests::TestRotation(*this, TEXT("tail_05"), Puppet.Component(TEXT("tail_05")).GetRotation(), PuppetTests::Rotation(FVector::XAxisVector, 90.0));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetDynamicChainWeightTest, "Open3DPuppet.Modules.DynamicChain.WeightBlendsOverInput", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetDynamicChainWeightTest::RunTest(const FString& Parameters)
{
	PuppetTests::FPuppet Puppet(PuppetTests::Tail());
	UPuppetModule_DynamicChain& Tail = PuppetDynamicChainTests::AddTail(Puppet);
	Tail.PerformerStart = TEXT("spine_01");
	Tail.PerformerEnd = TEXT("spine_03");
	Tail.InputMix = 1.0f;
	Tail.Weight = 0.5f;
	PuppetTests::FPerformer Performer = PuppetDynamicChainTests::Start(Puppet);
	Performer.SetLocalRotation(TEXT("spine_03"), PuppetTests::Rotation(FVector::XAxisVector, 90.0));
	Puppet.Drive(0, Performer);
	PuppetDynamicChainTests::SolveAt(Puppet, FVector::ZeroVector);
	PuppetTests::TestRotation(*this, TEXT("Half of tail_05's turn"), Puppet.Component(TEXT("tail_05")).GetRotation(), PuppetTests::Rotation(FVector::XAxisVector, 45.0));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetDynamicChainMissingTest, "Open3DPuppet.Modules.DynamicChain.MissingBoneSkipsTheModule", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetDynamicChainMissingTest::RunTest(const FString& Parameters)
{
	AddExpectedMessagePlain(TEXT("the puppet has no chain from 'tail_01' to 'tail_09'"), ELogVerbosity::Warning);
	PuppetTests::FPuppet Puppet(PuppetTests::Tail());
	UPuppetModule_DynamicChain& Tail = PuppetDynamicChainTests::AddTail(Puppet);
	Tail.PuppetEnd = TEXT("tail_09");
	Tail.Gravity = FVector(980, 0, 0);
	PuppetDynamicChainTests::Start(Puppet);
	PuppetDynamicChainTests::SolveAt(Puppet, FVector::ZeroVector, 5);
	TestEqual(TEXT("tail_05 left as the input"), Puppet.Component(TEXT("tail_05")), Puppet.Reference(TEXT("tail_05")), 1.e-3f);
	return true;
}

#endif
