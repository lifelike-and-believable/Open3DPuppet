// Copyright 2026 Lifelike & Believable. All Rights Reserved.

// Bone pickers (WP-8): the details panel offers the target mesh's bones for puppet bones, the
// canonical joints for performer joints, and the asset's slots for slots, through each property's
// GetOptions function.

#include "PuppetTestHarness.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Animation/Skeleton.h"
#include "Engine/SkeletalMesh.h"
#include "Harness/PuppetTestFixtures.h"
#include "PuppetAsset.h"
#include "PuppetModule.h"
#include "PuppetModules/PuppetModule_Chain.h"
#include "PuppetNaming.h"
#include "PuppetPerformerProfile.h"
#include "UObject/UObjectIterator.h"

namespace PuppetPickerTests
{
	TArray<FName> Names(TConstArrayView<PuppetTests::FBone> Bones)
	{
		TArray<FName> Result;
		for (const PuppetTests::FBone& Bone : Bones)
		{
			Result.Add(Bone.Name);
		}
		return Result;
	}

	/**
	 * Checks that every bone or joint name property in Struct (and the plugin's structs inside it)
	 * has a GetOptions function: puppet bones (categories Puppet and Collision) and performer joints
	 * (Performer), leaving out curve names. Counts the properties checked.
	 */
	void CheckPickers(FAutomationTestBase& Test, const UStruct* Struct, const FString& Path, int32& OutChecked)
	{
		for (TFieldIterator<FProperty> It(Struct); It; ++It)
		{
			const FProperty* Property = *It;
			const FProperty* Element = Property;
			if (const FArrayProperty* Array = CastField<FArrayProperty>(Property))
			{
				Element = Array->Inner;
			}
			if (const FStructProperty* Inner = CastField<FStructProperty>(Element))
			{
				if (Inner->Struct->GetOutermost()->GetName() == TEXT("/Script/Open3DPuppet"))
				{
					CheckPickers(Test, Inner->Struct, Path + TEXT(".") + Property->GetName(), OutChecked);
				}
				continue;
			}
			const FString Category = Property->GetMetaData(TEXT("Category"));
			if (!Element->IsA<FNameProperty>() || Property->GetName().Contains(TEXT("Curve"))
				|| (Category != TEXT("Puppet") && Category != TEXT("Performer") && Category != TEXT("Collision")))
			{
				continue;
			}
			++OutChecked;
			Test.TestTrue(FString::Printf(TEXT("%s.%s has a picker"), *Path, *Property->GetName()), Property->HasMetaData(TEXT("GetOptions")));
		}
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetPickerBonesTest, "Open3DPuppet.Pickers.Options.PuppetBonesComeFromTheTargetMesh", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetPickerBonesTest::RunTest(const FString& Parameters)
{
	PuppetTests::FPuppet Puppet(PuppetTests::Tail());
	UPuppetModule_Chain& Chain = Puppet.Add<UPuppetModule_Chain>();
	TestEqual(TEXT("Without a mesh, the canonical joints"), Chain.GetPuppetBoneOptions(), PuppetNaming::CanonicalJoints());

	const TStrongObjectPtr<USkeletalMesh> Mesh = PuppetTests::MakeMesh(PuppetTests::Tail());
	Puppet.Asset->TargetMesh = Mesh.Get();
	TestEqual(TEXT("The mesh's bones, in its order"), Chain.GetPuppetBoneOptions(), PuppetPickerTests::Names(PuppetTests::Tail()));
	TestEqual(TEXT("The asset offers the same"), Puppet.Asset->GetPuppetBoneOptions(), PuppetPickerTests::Names(PuppetTests::Tail()));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetPickerJointsTest, "Open3DPuppet.Pickers.Options.PerformerJointsAndSlots", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetPickerJointsTest::RunTest(const FString& Parameters)
{
	const TArray<FName> Joints = PuppetNaming::CanonicalJoints();
	for (const TCHAR* Joint : { TEXT("pelvis"), TEXT("spine_01"), TEXT("spine_03"), TEXT("spine_05"), TEXT("neck_01"), TEXT("head"), TEXT("eye_l"),
		TEXT("clavicle_r"), TEXT("hand_l"), TEXT("thumb_01_l"), TEXT("pinky_03_r"), TEXT("ball_l"), TEXT("prop") })
	{
		TestTrue(FString::Printf(TEXT("Canonical %s"), Joint), Joints.Contains(FName(Joint)));
	}
	if (Joints.Num() > 0)
	{
		TestEqual(TEXT("In skeleton order, the pelvis first"), Joints[0], FName(TEXT("pelvis")));
	}

	PuppetTests::FPuppet Puppet(PuppetTests::Biped());
	Puppet.Asset->PerformerSlots = { TEXT("Body"), TEXT("Tail") };
	UPuppetModule_Chain& Chain = Puppet.Add<UPuppetModule_Chain>();
	TestEqual(TEXT("A module offers the canonical joints"), Chain.GetPerformerJointOptions(), Joints);
	TestEqual(TEXT("A module offers the asset's slots"), Chain.GetPerformerSlotOptions(), TArray<FName>({ TEXT("Body"), TEXT("Tail") }));
	TestEqual(TEXT("So does the asset, for its root slot"), Puppet.Asset->GetPerformerSlotOptions(), TArray<FName>({ TEXT("Body"), TEXT("Tail") }));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetPickerProfileTest, "Open3DPuppet.Pickers.Options.ProfileKeysAndValues", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetPickerProfileTest::RunTest(const FString& Parameters)
{
	UPuppetPerformerProfile* Profile = NewObject<UPuppetPerformerProfile>();
	TestEqual(TEXT("Keys: the canonical joints"), Profile->GetCanonicalJointOptions(), PuppetNaming::CanonicalJoints());
	// A picker takes no typed names, so without a skeleton it offers what the schemes and the mapping name.
	Profile->CanonicalToSource.Add(TEXT("pelvis"), TEXT("MyRig_Pelvis"));
	TestTrue(TEXT("Values without a skeleton: the schemes' names"), Profile->GetSourceBoneOptions().Contains(FName(TEXT("LeftForeArm"))));
	TestTrue(TEXT("And the values already mapped"), Profile->GetSourceBoneOptions().Contains(FName(TEXT("MyRig_Pelvis"))));
	const TStrongObjectPtr<USkeleton> Skeleton = PuppetTests::MakeSkeleton(PuppetTests::Tail());
	Profile->SourceSkeleton = Skeleton.Get();
	TestEqual(TEXT("Values: the source skeleton's bones"), Profile->GetSourceBoneOptions(), PuppetPickerTests::Names(PuppetTests::Tail()));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetPickerCoverageTest, "Open3DPuppet.Pickers.Coverage.EveryBoneAndJointHasAPicker", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetPickerCoverageTest::RunTest(const FString& Parameters)
{
	int32 Checked = 0;
	for (TObjectIterator<UClass> It; It; ++It)
	{
		if (It->IsChildOf(UPuppetModule::StaticClass()) && !It->HasAnyClassFlags(CLASS_Abstract)
			&& It->GetOutermost()->GetName() == TEXT("/Script/Open3DPuppet"))
		{
			PuppetPickerTests::CheckPickers(*this, *It, It->GetName(), Checked);
		}
	}
	TestTrue(TEXT("Every library module was checked"), Checked >= 40);
	const FProperty* Pelvis = UPuppetAsset::StaticClass()->FindPropertyByName(TEXT("PelvisBone"));
	TestTrue(TEXT("The asset's pelvis bone has a picker"), Pelvis && Pelvis->HasMetaData(TEXT("GetOptions")));
	const FProperty* RootSlot = UPuppetAsset::StaticClass()->FindPropertyByName(TEXT("RootSlot"));
	TestTrue(TEXT("Its root slot has a picker"), RootSlot && RootSlot->HasMetaData(TEXT("GetOptions")));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
