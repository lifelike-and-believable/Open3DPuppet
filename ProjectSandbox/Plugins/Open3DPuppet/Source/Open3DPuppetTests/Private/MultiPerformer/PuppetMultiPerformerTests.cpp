// Copyright 2026 Lifelike & Believable. All Rights Reserved.

// The multi-performer sample (WP-7, docs/guides/multi-performer-sample.md): a creature whose body
// a performer plays (slot Body: Root and a spine Chain) and whose tail a puppeteer plays with a
// tracked rod (slot Tail: Prop Chain), as ADR 0004 describes.

#include "PuppetTestHarness.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Harness/PuppetTestFixtures.h"
#include "PuppetModules/PuppetModule_Chain.h"
#include "PuppetModules/PuppetModule_PropChain.h"
#include "PuppetModules/PuppetModule_Root.h"

namespace PuppetMultiPerformerTests
{
	/** The biped with a three-bone tail behind the pelvis, tail_01 to tail_03, 10 cm apart along -X. */
	TArray<PuppetTests::FBone> TailedBiped()
	{
		TArray<PuppetTests::FBone> Bones = PuppetTests::Biped();
		Bones.Append({
			{ TEXT("tail_01"), TEXT("pelvis"), FVector(-10, 0, 0) },
			{ TEXT("tail_02"), TEXT("tail_01"), FVector(-10, 0, 0) },
			{ TEXT("tail_03"), TEXT("tail_02"), FVector(-10, 0, 0) },
		});
		return Bones;
	}

	/** The sample's setup, as the guide builds it: slots Body (the root) and Tail. */
	struct FSample
	{
		FSample()
			: Puppet(TailedBiped())
			, Body(PuppetTests::Biped())
			, Rod({ { TEXT("prop"), TEXT(""), FVector::ZeroVector } })
		{
			Puppet.Asset->PerformerSlots = { TEXT("Body"), TEXT("Tail") };
			Puppet.Asset->RootSlot = TEXT("Body");
			Puppet.Add<UPuppetModule_Root>();
			UPuppetModule_Chain& Spine = Puppet.Add<UPuppetModule_Chain>();
			Spine.PerformerStart = TEXT("spine_01");
			Spine.PerformerEnd = TEXT("spine_03");
			Spine.PuppetStart = TEXT("spine_01");
			Spine.PuppetEnd = TEXT("spine_03");
			UPuppetModule_PropChain& Tail = Puppet.Add<UPuppetModule_PropChain>();
			Tail.PerformerSlot = TEXT("Tail");
			Tail.PuppetStart = TEXT("tail_01");
			Tail.PuppetEnd = TEXT("tail_03");
		}

		/** Builds, calibrates the body and, when bCalibrateRod, the rod, both at rest. */
		void Start(bool bCalibrateRod = true)
		{
			Puppet.Build();
			Puppet.Drive(0, Body);
			Puppet.Calibrate(0, Body);
			Puppet.Drive(1, Rod);
			if (bCalibrateRod)
			{
				Puppet.Calibrate(1, Rod);
			}
		}

		void Solve()
		{
			Puppet.Drive(0, Body);
			Puppet.Drive(1, Rod);
			Puppet.Solve();
		}

		PuppetTests::FPuppet Puppet;
		PuppetTests::FPerformer Body;
		PuppetTests::FPerformer Rod;
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetMultiPerformerBodyTest, "Open3DPuppet.MultiPerformer.BodyAndTail.TheBodyMovesThePuppetAndTheTailFollows", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetMultiPerformerBodyTest::RunTest(const FString& Parameters)
{
	PuppetMultiPerformerTests::FSample Sample;
	Sample.Start();
	const FVector TailRest = Sample.Puppet.Component(TEXT("tail_03")).GetTranslation();
	Sample.Body.SetLocalTranslation(TEXT("pelvis"), FVector(20, 0, 100));
	Sample.Solve();
	TestEqual(TEXT("The body's performer moves the puppet"), Sample.Puppet.Component(TEXT("pelvis")).GetTranslation(), FVector(20, 0, 100), 1.e-3f);
	TestEqual(TEXT("The tail goes with it"), Sample.Puppet.Component(TEXT("tail_03")).GetTranslation(), TailRest + FVector(20, 0, 0), 1.e-3f);
	PuppetTests::TestRotation(*this, TEXT("Keeping its rest while the rod is still"), Sample.Puppet.Component(TEXT("tail_03")).GetRotation(), FQuat::Identity);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetMultiPerformerRodTest, "Open3DPuppet.MultiPerformer.BodyAndTail.TheRodBendsOnlyTheTail", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetMultiPerformerRodTest::RunTest(const FString& Parameters)
{
	PuppetMultiPerformerTests::FSample Sample;
	Sample.Start();
	const FQuat Swing = PuppetTests::Rotation(FVector::ZAxisVector, 30.0);
	Sample.Rod.SetLocalRotation(TEXT("prop"), Swing);
	Sample.Solve();
	PuppetTests::TestRotation(*this, TEXT("The tail's tip swings with the rod"), Sample.Puppet.Component(TEXT("tail_03")).GetRotation(), Swing);
	PuppetTests::TestRotation(*this, TEXT("The spine is the body performer's"), Sample.Puppet.Component(TEXT("spine_03")).GetRotation(), FQuat::Identity);
	TestEqual(TEXT("The rod does not move the puppet"), Sample.Puppet.Component(TEXT("pelvis")).GetTranslation(), FVector(0, 0, 100), 1.e-3f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetMultiPerformerUncalibratedTest, "Open3DPuppet.MultiPerformer.BodyAndTail.AnUncalibratedRodLeavesTheTail", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetMultiPerformerUncalibratedTest::RunTest(const FString& Parameters)
{
	PuppetMultiPerformerTests::FSample Sample;
	Sample.Start(false);
	Sample.Rod.SetLocalRotation(TEXT("prop"), PuppetTests::Rotation(FVector::ZAxisVector, 30.0));
	Sample.Body.SetLocalTranslation(TEXT("pelvis"), FVector(20, 0, 100));
	Sample.Solve();
	PuppetTests::TestRotation(*this, TEXT("The tail stays in the input pose"), Sample.Puppet.Component(TEXT("tail_03")).GetRotation(), FQuat::Identity);
	TestEqual(TEXT("While the body still drives"), Sample.Puppet.Component(TEXT("pelvis")).GetTranslation(), FVector(20, 0, 100), 1.e-3f);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
