// Copyright 2026 Lifelike & Believable. All Rights Reserved.

// Hand-offs between an animation and the performer (ADR 0008): easing, the performance
// component's transitions, the control blend per slot, and the Root module's hand-off offset.

#include "PuppetTestHarness.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Harness/PuppetTestFixtures.h"
#include "PuppetEasing.h"
#include "PuppetModules/PuppetModule_Chain.h"
#include "PuppetModules/PuppetModule_Root.h"
#include "PuppetPerformanceComponent.h"

namespace PuppetHandoffTests
{
	const FName Body(TEXT("Body"));

	/** A puppet with a Root module, built, with slot 0 calibrated on a biped performer at rest. */
	struct FRig
	{
		FRig() : Puppet(PuppetTests::Biped()), Performer(PuppetTests::Biped()), Root(Puppet.Add<UPuppetModule_Root>()) {}

		void Start()
		{
			Puppet.Build();
			Puppet.Drive(0, Performer);
			Puppet.Calibrate(0, Performer);
		}

		/** Moves the performer's pelvis to Position (component space; the rest is at 0, 0, 100) and solves from Input. */
		void Step(const FVector& Position, TConstArrayView<FTransform> Input)
		{
			Performer.SetLocalTranslation(TEXT("pelvis"), Position);
			Puppet.Drive(0, Performer);
			Puppet.Solve(Input);
		}

		FVector Pelvis() const { return Puppet.Component(TEXT("pelvis")).GetTranslation(); }

		PuppetTests::FPuppet Puppet;
		PuppetTests::FPerformer Performer;
		UPuppetModule_Root& Root;
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetEasingTest, "Open3DPuppet.Handoffs.Easing.ShapesAndEndpoints", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetEasingTest::RunTest(const FString& Parameters)
{
	for (const EPuppetEasing Easing : { EPuppetEasing::Linear, EPuppetEasing::EaseIn, EPuppetEasing::EaseOut, EPuppetEasing::EaseInOut })
	{
		TestNearlyEqual(TEXT("Starts at 0"), PuppetEasing::Apply(Easing, 0.0f), 0.0f);
		TestNearlyEqual(TEXT("Ends at 1"), PuppetEasing::Apply(Easing, 1.0f), 1.0f);
		TestNearlyEqual(TEXT("Clamped below"), PuppetEasing::Apply(Easing, -1.0f), 0.0f);
		TestNearlyEqual(TEXT("Clamped above"), PuppetEasing::Apply(Easing, 2.0f), 1.0f);
	}
	TestNearlyEqual(TEXT("Linear"), PuppetEasing::Apply(EPuppetEasing::Linear, 0.5f), 0.5f);
	TestNearlyEqual(TEXT("Ease in is slow at first"), PuppetEasing::Apply(EPuppetEasing::EaseIn, 0.5f), 0.25f);
	TestNearlyEqual(TEXT("Ease out is fast at first"), PuppetEasing::Apply(EPuppetEasing::EaseOut, 0.5f), 0.75f);
	TestNearlyEqual(TEXT("Ease in out is symmetric"), PuppetEasing::Apply(EPuppetEasing::EaseInOut, 0.5f), 0.5f);
	TestNearlyEqual(TEXT("Ease in out, first quarter"), PuppetEasing::Apply(EPuppetEasing::EaseInOut, 0.25f), 0.125f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetPerformanceDefaultsTest, "Open3DPuppet.Handoffs.PerformanceComponent.DefaultBlends", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetPerformanceDefaultsTest::RunTest(const FString& Parameters)
{
	UPuppetPerformanceComponent* Performance = NewObject<UPuppetPerformanceComponent>();
	TestNearlyEqual(TEXT("A slot never mentioned is under the performer"), Performance->GetControlBlend(PuppetHandoffTests::Body), 1.0f);
	Performance->SlotsStartingOnAnimation.Add(PuppetHandoffTests::Body);
	TestNearlyEqual(TEXT("A slot starting on the animation"), Performance->GetControlBlend(PuppetHandoffTests::Body), 0.0f);
	TestFalse(TEXT("Not transitioning"), Performance->IsTransitioning(PuppetHandoffTests::Body));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetPerformanceTransitionTest, "Open3DPuppet.Handoffs.PerformanceComponent.TransitionEasesAndFinishes", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetPerformanceTransitionTest::RunTest(const FString& Parameters)
{
	UPuppetPerformanceComponent* Performance = NewObject<UPuppetPerformanceComponent>();
	Performance->SlotsStartingOnAnimation.Add(PuppetHandoffTests::Body);

	Performance->TransitionToPerformer(PuppetHandoffTests::Body, 1.0f, EPuppetEasing::Linear);
	TestTrue(TEXT("Transitioning"), Performance->IsTransitioning(PuppetHandoffTests::Body));
	TestNearlyEqual(TEXT("Starts from the animation"), Performance->GetControlBlend(PuppetHandoffTests::Body), 0.0f);
	Performance->AdvanceTransitions(0.25f);
	TestNearlyEqual(TEXT("A quarter of the way"), Performance->GetControlBlend(PuppetHandoffTests::Body), 0.25f);
	Performance->AdvanceTransitions(1.0f);
	TestNearlyEqual(TEXT("Ends at the performer"), Performance->GetControlBlend(PuppetHandoffTests::Body), 1.0f);
	TestFalse(TEXT("Finished"), Performance->IsTransitioning(PuppetHandoffTests::Body));

	Performance->TransitionToAnimation(PuppetHandoffTests::Body, 2.0f, EPuppetEasing::EaseIn);
	Performance->AdvanceTransitions(1.0f);
	TestNearlyEqual(TEXT("Eased back half way"), Performance->GetControlBlend(PuppetHandoffTests::Body), 0.75f);

	Performance->TransitionToAnimation(TEXT("Tail"), 0.0f, EPuppetEasing::Linear);
	TestNearlyEqual(TEXT("A zero duration is a cut"), Performance->GetControlBlend(TEXT("Tail")), 0.0f);
	TestFalse(TEXT("A cut does not transition"), Performance->IsTransitioning(TEXT("Tail")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetPerformanceSetBlendTest, "Open3DPuppet.Handoffs.PerformanceComponent.SetControlBlendStopsTransition", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetPerformanceSetBlendTest::RunTest(const FString& Parameters)
{
	UPuppetPerformanceComponent* Performance = NewObject<UPuppetPerformanceComponent>();
	Performance->TransitionToAnimation(PuppetHandoffTests::Body, 1.0f, EPuppetEasing::Linear);
	Performance->SetControlBlend(PuppetHandoffTests::Body, 0.4f);
	TestFalse(TEXT("Stopped"), Performance->IsTransitioning(PuppetHandoffTests::Body));
	Performance->AdvanceTransitions(0.5f);
	TestNearlyEqual(TEXT("Holds the set blend"), Performance->GetControlBlend(PuppetHandoffTests::Body), 0.4f);
	Performance->SetControlBlend(PuppetHandoffTests::Body, 3.0f);
	TestNearlyEqual(TEXT("Clamped to 1"), Performance->GetControlBlend(PuppetHandoffTests::Body), 1.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetPerformanceRecentreTest, "Open3DPuppet.Handoffs.PerformanceComponent.HandOffAndRecentreCount", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetPerformanceRecentreTest::RunTest(const FString& Parameters)
{
	UPuppetPerformanceComponent* Performance = NewObject<UPuppetPerformanceComponent>();
	TestEqual(TEXT("None yet"), Performance->GetRecentreCount(PuppetHandoffTests::Body), 0);
	Performance->TransitionToPerformer(PuppetHandoffTests::Body, 1.0f, EPuppetEasing::Linear);
	TestEqual(TEXT("A hand-off to the performer re-centres"), Performance->GetRecentreCount(PuppetHandoffTests::Body), 1);
	Performance->Recentre(PuppetHandoffTests::Body);
	TestEqual(TEXT("A manual re-centre"), Performance->GetRecentreCount(PuppetHandoffTests::Body), 2);
	Performance->TransitionToAnimation(PuppetHandoffTests::Body, 1.0f, EPuppetEasing::Linear);
	TestEqual(TEXT("A hand-off to the animation does not"), Performance->GetRecentreCount(PuppetHandoffTests::Body), 2);
	TestEqual(TEXT("Other slots are separate"), Performance->GetRecentreCount(TEXT("Tail")), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetControlBlendTest, "Open3DPuppet.Handoffs.Instance.ControlBlendScalesModuleWeight", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetControlBlendTest::RunTest(const FString& Parameters)
{
	PuppetHandoffTests::FRig Rig;
	Rig.Start();
	TestNearlyEqual(TEXT("Default blend"), Rig.Puppet.Instance.GetControlBlend(0), 1.0f);

	Rig.Puppet.Instance.SetControlBlend(0, 0.5f);
	Rig.Step(FVector(10, 0, 100), Rig.Puppet.RefSkeleton.GetRefBonePose());
	TestEqual(TEXT("Half way to the performer"), Rig.Pelvis(), FVector(5, 0, 100), 1.e-3f);

	Rig.Puppet.Instance.SetControlBlend(0, 0.0f);
	Rig.Step(FVector(10, 0, 100), Rig.Puppet.RefSkeleton.GetRefBonePose());
	TestEqual(TEXT("The input pose"), Rig.Pelvis(), FVector(0, 0, 100), 1.e-3f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetRecentreWaitsTest, "Open3DPuppet.Handoffs.Instance.RecentreWaitsForModulesToRun", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetRecentreWaitsTest::RunTest(const FString& Parameters)
{
	// A hand-off starts at blend 0, when no module runs; the re-centre lands on the first frame they do.
	PuppetHandoffTests::FRig Rig;
	Rig.Root.bHoldOffset = true;
	Rig.Start();
	const TArray<FTransform> Input = Rig.Puppet.InputWith(TEXT("pelvis"), FTransform(FVector(100, 0, 100)));

	Rig.Puppet.Instance.SetControlBlend(0, 0.0f);
	Rig.Puppet.Instance.RequestRecentre(0);
	Rig.Step(FVector(0, 0, 100), Input);
	Rig.Puppet.Instance.SetControlBlend(0, 1.0f);
	Rig.Step(FVector(0, 0, 100), Input);
	TestEqual(TEXT("Anchored where the animation is"), Rig.Pelvis(), FVector(100, 0, 100), 1.e-3f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetRecentreContinuesTest, "Open3DPuppet.Handoffs.Root.RecentreContinuesFromTheAnimation", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetRecentreContinuesTest::RunTest(const FString& Parameters)
{
	// The animation left the puppet away from the stage centre, facing +Y.
	PuppetHandoffTests::FRig Rig;
	Rig.Root.bHoldOffset = true;
	Rig.Start();
	const FQuat Facing = PuppetTests::Rotation(FVector::ZAxisVector, 90.0);
	const TArray<FTransform> Input = Rig.Puppet.InputWith(TEXT("pelvis"), FTransform(Facing, FVector(200, 50, 100)));

	Rig.Puppet.Instance.RequestRecentre(0);
	Rig.Step(FVector(0, 0, 100), Input);
	TestEqual(TEXT("No snap"), Rig.Pelvis(), FVector(200, 50, 100), 1.e-3f);
	PuppetTests::TestRotation(*this, TEXT("Keeps its facing"), Rig.Puppet.Component(TEXT("pelvis")).GetRotation(), Facing);

	// The performer steps forward: the puppet steps the way it faces.
	Rig.Step(FVector(10, 0, 100), Input);
	TestEqual(TEXT("Moves the way it faces"), Rig.Pelvis(), FVector(200, 60, 100), 1.e-3f);
	Rig.Step(FVector(10, 0, 90), Input);
	TestEqual(TEXT("A crouch comes through"), Rig.Pelvis(), FVector(200, 60, 90), 1.e-3f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetOffsetTurnsModulesTest, "Open3DPuppet.Handoffs.Root.OffsetTurnsEveryModule", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetOffsetTurnsModulesTest::RunTest(const FString& Parameters)
{
	PuppetHandoffTests::FRig Rig;
	Rig.Root.bHoldOffset = true;
	UPuppetModule_Chain& Spine = Rig.Puppet.Add<UPuppetModule_Chain>();
	Spine.PerformerStart = TEXT("spine_01");
	Spine.PerformerEnd = TEXT("spine_03");
	Spine.PuppetStart = TEXT("spine_01");
	Spine.PuppetEnd = TEXT("spine_03");
	Rig.Start();
	const FQuat Facing = PuppetTests::Rotation(FVector::ZAxisVector, 90.0);

	Rig.Puppet.Instance.RequestRecentre(0);
	Rig.Step(FVector(0, 0, 100), Rig.Puppet.InputWith(TEXT("pelvis"), FTransform(Facing, FVector(0, 0, 100))));
	PuppetTests::TestRotation(*this, TEXT("The spine turns with the pelvis"), Rig.Puppet.Component(TEXT("spine_02")).GetRotation(), Facing);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetConvergenceTest, "Open3DPuppet.Handoffs.Root.ConvergesOnlyWhileMoving", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetConvergenceTest::RunTest(const FString& Parameters)
{
	PuppetHandoffTests::FRig Rig;
	Rig.Root.ConvergenceTime = 1.0f;
	Rig.Root.ConvergenceEasing = EPuppetEasing::Linear;
	Rig.Start();
	const TArray<FTransform> Input = Rig.Puppet.InputWith(TEXT("pelvis"), FTransform(FVector(100, 0, 100)));

	Rig.Puppet.Instance.RequestRecentre(0);
	for (int32 Frame = 0; Frame < 30; ++Frame)
	{
		Rig.Step(FVector(0, 0, 100), Input);
	}
	TestEqual(TEXT("A standing performer: the puppet does not slide"), Rig.Pelvis(), FVector(100, 0, 100), 1.e-3f);

	// Walking at 150 cm/s, above Moving Speed: full rate, so half the offset goes in half a second.
	FVector Position(0, 0, 100);
	for (int32 Frame = 0; Frame < 15; ++Frame)
	{
		Position.Y += 5.0;
		Rig.Step(Position, Input);
	}
	TestEqual(TEXT("Half converged"), Rig.Pelvis(), FVector(50, 75, 100), 1.e-2f);
	for (int32 Frame = 0; Frame < 20; ++Frame)
	{
		Position.Y += 5.0;
		Rig.Step(Position, Input);
	}
	TestEqual(TEXT("On the performer"), Rig.Pelvis(), FVector(0, 175, 100), 1.e-2f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetConvergenceSnapTest, "Open3DPuppet.Handoffs.Root.ZeroConvergenceTimeSnaps", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetConvergenceSnapTest::RunTest(const FString& Parameters)
{
	PuppetHandoffTests::FRig Rig;
	Rig.Root.ConvergenceTime = 0.0f;
	Rig.Start();
	Rig.Puppet.Instance.RequestRecentre(0);
	Rig.Step(FVector(0, 0, 100), Rig.Puppet.InputWith(TEXT("pelvis"), FTransform(FVector(100, 0, 100))));
	TestEqual(TEXT("On the performer at once"), Rig.Pelvis(), FVector(0, 0, 100), 1.e-3f);
	return true;
}

#endif
