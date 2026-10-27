// Copyright 2026 Lifelike & Believable. All Rights Reserved.

// What the session panel relies on (WP-8): module numbers tuned during a session apply live,
// without binding the puppet again (which would reset its hand-off offset and dynamics), and the
// Puppet Solve node reports each slot's state on the game thread.

#include "PuppetTestHarness.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "AnimNode_PuppetSolve.h"
#include "Animation/AnimSequence.h"
#include "Animation/Skeleton.h"
#include "Harness/PuppetTestFixtures.h"
#include "PuppetModules/PuppetModule_Chain.h"
#include "PuppetModules/PuppetModule_Digits.h"

namespace PuppetSessionTests
{
	FProperty* Property(const UStruct* Owner, const TCHAR* Name)
	{
		FProperty* Found = FindFProperty<FProperty>(Owner, Name);
		check(Found);
		return Found;
	}

	/** Whether an edit of Name on Module, through the module, binds the puppet again. */
	bool Rebinds(UPuppetAsset& Asset, UObject& Module, const TCHAR* Name, EPropertyChangeType::Type ChangeType)
	{
		const int32 Before = Asset.GetRevision();
		FPropertyChangedEvent Event(Property(Module.GetClass(), Name), ChangeType);
		Module.PostEditChangeProperty(Event);
		return Asset.GetRevision() != Before;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetLiveEditTest, "Open3DPuppet.Session.LiveEdits.NumbersApplyLiveNamesRebind", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetLiveEditTest::RunTest(const FString& Parameters)
{
	PuppetTests::FPuppet Puppet(PuppetTests::Hand());
	UPuppetAsset& Asset = *Puppet.Asset;
	UPuppetModule_Chain& Chain = Puppet.Add<UPuppetModule_Chain>();
	UPuppetModule_Digits& Digits = Puppet.Add<UPuppetModule_Digits>();

	TestFalse(TEXT("Amplification set in place"), PuppetSessionTests::Rebinds(Asset, Chain, TEXT("Amplification"), EPropertyChangeType::ValueSet));
	TestFalse(TEXT("Amplification while dragging"), PuppetSessionTests::Rebinds(Asset, Chain, TEXT("Amplification"), EPropertyChangeType::Interactive));
	TestFalse(TEXT("The module's Weight"), PuppetSessionTests::Rebinds(Asset, Chain, TEXT("Weight"), EPropertyChangeType::ValueSet));
	TestTrue(TEXT("A bone name rebinds"), PuppetSessionTests::Rebinds(Asset, Chain, TEXT("PerformerStart"), EPropertyChangeType::ValueSet));
	TestTrue(TEXT("A vector rebinds (the spread axis is read in Bind)"), PuppetSessionTests::Rebinds(Asset, Digits, TEXT("SpreadAxis"), EPropertyChangeType::ValueSet));
	TestTrue(TEXT("Adding a digit rebinds"), PuppetSessionTests::Rebinds(Asset, Digits, TEXT("Digits"), EPropertyChangeType::ArrayAdd));

	// A digit's gain, inside the Digits array, set in place.
	{
		const int32 Before = Asset.GetRevision();
		FPropertyChangedEvent Event(PuppetSessionTests::Property(FPuppetDigit::StaticStruct(), TEXT("CurlGain")), EPropertyChangeType::ValueSet);
		Event.SetActiveMemberProperty(PuppetSessionTests::Property(UPuppetModule_Digits::StaticClass(), TEXT("Digits")));
		Digits.PostEditChangeProperty(Event);
		TestEqual(TEXT("A digit's gain applies live"), Asset.GetRevision(), Before);
	}
	// The same edits through the asset's details panel, where a module is a member of Modules.
	{
		const int32 Before = Asset.GetRevision();
		FPropertyChangedEvent Event(PuppetSessionTests::Property(UPuppetModule_Chain::StaticClass(), TEXT("Amplification")), EPropertyChangeType::ValueSet);
		Event.SetActiveMemberProperty(PuppetSessionTests::Property(UPuppetAsset::StaticClass(), TEXT("Modules")));
		Asset.PostEditChangeProperty(Event);
		TestEqual(TEXT("Through the asset, a module number applies live"), Asset.GetRevision(), Before);
		FPropertyChangedEvent Teleport(PuppetSessionTests::Property(UPuppetAsset::StaticClass(), TEXT("TeleportDistance")), EPropertyChangeType::ValueSet);
		Teleport.SetActiveMemberProperty(Teleport.Property);
		Asset.PostEditChangeProperty(Teleport);
		TestNotEqual(TEXT("An asset number rebinds (it is read when the puppet initialises)"), Asset.GetRevision(), Before);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetLiveEditSolveTest, "Open3DPuppet.Session.LiveEdits.ANumberTakesEffectWithoutRebinding", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetLiveEditSolveTest::RunTest(const FString& Parameters)
{
	// Solvers read their settings every frame, so a live edit shows on the next solve.
	PuppetTests::FPuppet Puppet(PuppetTests::Biped());
	UPuppetModule_Chain& Spine = Puppet.Add<UPuppetModule_Chain>();
	Spine.PerformerEnd = TEXT("spine_03");
	Spine.PuppetEnd = TEXT("spine_03");
	PuppetTests::FPerformer Performer(PuppetTests::Biped());
	Puppet.Build();
	Puppet.Drive(0, Performer);
	Puppet.Calibrate(0, Performer);
	Performer.SetLocalRotation(TEXT("spine_01"), PuppetTests::Rotation(FVector::XAxisVector, 30.0));
	Puppet.Drive(0, Performer);
	Puppet.Solve();
	PuppetTests::TestRotation(*this, TEXT("Before"), Puppet.Component(TEXT("spine_03")).GetRotation(), PuppetTests::Rotation(FVector::XAxisVector, 30.0));

	Spine.Amplification = 2.0f;
	FPropertyChangedEvent Event(PuppetSessionTests::Property(UPuppetModule_Chain::StaticClass(), TEXT("Amplification")), EPropertyChangeType::ValueSet);
	Spine.PostEditChangeProperty(Event);
	Puppet.Solve();
	PuppetTests::TestRotation(*this, TEXT("After, with no rebind"), Puppet.Component(TEXT("spine_03")).GetRotation(), PuppetTests::Rotation(FVector::XAxisVector, 60.0));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetSlotStatusTest, "Open3DPuppet.Session.SlotStatus.TheNodeReportsEachSlot", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetSlotStatusTest::RunTest(const FString& Parameters)
{
	PuppetTests::FPuppet Puppet(PuppetTests::Biped());
	Puppet.Asset->PerformerSlots = { TEXT("Body"), TEXT("Tail") };
	const TStrongObjectPtr<USkeleton> Skeleton = PuppetTests::MakeSkeleton(PuppetTests::Biped());
	const PuppetTests::FPerformer Rest(PuppetTests::Biped());
	const TStrongObjectPtr<UAnimSequence> Clip = PuppetTests::MakeClip(*Skeleton, { Rest.Local, Rest.Local });

	FAnimNode_PuppetSolve Node;
	Node.Puppet = Puppet.Asset.Get();
	FPuppetCastEntry& Body = Node.Cast.AddDefaulted_GetRef();
	Body.Slot = TEXT("Body");
	Body.Subject.Name = TEXT("Open3DPuppetTest_BodySubject");
	FPuppetCastEntry& Tail = Node.Cast.AddDefaulted_GetRef();
	Tail.Slot = TEXT("Tail");
	Tail.Source = EPuppetCastSource::Animation;
	Tail.Animation = Clip.Get();
	Node.PreUpdate(nullptr);

	const TConstArrayView<FPuppetSlotStatus> Status = Node.GetSlotStatus();
	if (!TestEqual(TEXT("One per slot"), Status.Num(), 2))
	{
		return false;
	}
	TestEqual(TEXT("Body: the slot"), Status[0].Slot, FName(TEXT("Body")));
	TestTrue(TEXT("Body: from Live Link"), Status[0].Source == EPuppetCastSource::LiveLink);
	TestEqual(TEXT("Body: the subject"), Status[0].Performer, FName(TEXT("Open3DPuppetTest_BodySubject")));
	TestTrue(TEXT("Body: no data yet"), Status[0].State == EPuppetSlotState::NoData);
	TestEqual(TEXT("Body: under the performer"), Status[0].ControlBlend, 1.0f);
	TestTrue(TEXT("Tail: from an animation"), Status[1].Source == EPuppetCastSource::Animation);
	TestEqual(TEXT("Tail: the clip"), Status[1].Performer, Clip->GetFName());
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
