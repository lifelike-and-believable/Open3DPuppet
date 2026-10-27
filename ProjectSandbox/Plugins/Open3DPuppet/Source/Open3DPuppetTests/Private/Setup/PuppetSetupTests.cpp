// Copyright 2026 Lifelike & Believable. All Rights Reserved.

// The setup assistant (WP-8): Propose Modules fills a puppet asset's empty module list for its
// target mesh, naming the mesh's bones through the naming schemes and finding leftover chains
// (tails, ears) by topology.

#include "PuppetTestHarness.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Engine/SkeletalMesh.h"
#include "Harness/PuppetTestFixtures.h"
#include "PuppetAsset.h"
#include "PuppetModules/PuppetModule_Chain.h"
#include "PuppetModules/PuppetModule_DynamicChain.h"
#include "PuppetModules/PuppetModule_Root.h"
#include "PuppetModules/PuppetModule_TwoBoneLimb.h"
#include "PuppetNaming.h"
#include "PuppetSetup.h"

namespace PuppetSetupTests
{
	/** The biped with a tail, ears and the helper bones meshes carry (IK targets, a twist bone). */
	TArray<PuppetTests::FBone> Creature()
	{
		TArray<PuppetTests::FBone> Bones = PuppetTests::Biped();
		Bones.Append({
			{ TEXT("tail_01"), TEXT("pelvis"), FVector(-10, 0, 0) },
			{ TEXT("tail_02"), TEXT("tail_01"), FVector(-10, 0, 0) },
			{ TEXT("tail_03"), TEXT("tail_02"), FVector(-10, 0, 0) },
			{ TEXT("ear_01_l"), TEXT("head"), FVector(0, 5, 10) },
			{ TEXT("ear_02_l"), TEXT("ear_01_l"), FVector(0, 0, 5) },
			{ TEXT("ik_foot_root"), TEXT("root"), FVector::ZeroVector },
			{ TEXT("ik_foot_l"), TEXT("ik_foot_root"), FVector(0, 10, 10) },
			{ TEXT("upperarm_twist_01_l"), TEXT("upperarm_l"), FVector(0, 15, 0) },
		});
		return Bones;
	}

	/** A puppet asset whose target mesh has Bones; the mesh lives as long as the struct. */
	struct FAsset
	{
		explicit FAsset(TConstArrayView<PuppetTests::FBone> Bones) : Puppet(Bones), Mesh(PuppetTests::MakeMesh(Bones))
		{
			Puppet.Asset->TargetMesh = Mesh.Get();
		}

		bool Propose(FString& OutReport) { return PuppetSetup::ProposeModules(*Puppet.Asset, PuppetNaming::DefaultSchemes(), OutReport); }

		template <typename T>
		TArray<T*> Modules() const
		{
			TArray<T*> Result;
			for (UPuppetModule* Module : Puppet.Asset->Modules)
			{
				if (T* Typed = Cast<T>(Module))
				{
					Result.Add(Typed);
				}
			}
			return Result;
		}

		PuppetTests::FPuppet Puppet;
		TStrongObjectPtr<USkeletalMesh> Mesh;
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetSetupBipedTest, "Open3DPuppet.Setup.ProposeModules.ABipedWithATailAndEars", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetSetupBipedTest::RunTest(const FString& Parameters)
{
	PuppetSetupTests::FAsset Asset(PuppetSetupTests::Creature());
	FString Report;
	if (!TestTrue(TEXT("Proposes"), Asset.Propose(Report)))
	{
		AddError(Report);
		return false;
	}
	TestFalse(TEXT("With a report"), Report.IsEmpty());
	TestEqual(TEXT("Root"), Asset.Modules<UPuppetModule_Root>().Num(), 1);
	TestEqual(TEXT("The pelvis bone"), Asset.Puppet.Asset->PelvisBone, FName(TEXT("pelvis")));

	const TArray<UPuppetModule_Chain*> Chains = Asset.Modules<UPuppetModule_Chain>();
	if (TestEqual(TEXT("A spine chain and a neck chain"), Chains.Num(), 2))
	{
		TestEqual(TEXT("The spine ends where the neck hangs"), Chains[0]->PuppetEnd, FName(TEXT("spine_03")));
		TestEqual(TEXT("Read from the performer's whole spine"), Chains[0]->PerformerEnd, FName(TEXT("spine_05")));
		TestEqual(TEXT("The neck to the head"), Chains[1]->PuppetEnd, FName(TEXT("head")));
	}

	const TArray<UPuppetModule_TwoBoneLimb*> Limbs = Asset.Modules<UPuppetModule_TwoBoneLimb>();
	if (TestEqual(TEXT("The left arm and leg (the biped has no right side)"), Limbs.Num(), 2))
	{
		TestEqual(TEXT("The arm"), Limbs[0]->PuppetEnd, FName(TEXT("hand_l")));
		TestEqual(TEXT("The leg"), Limbs[1]->PuppetEnd, FName(TEXT("foot_l")));
		TestEqual(TEXT("The leg keeps the feet planted"), Limbs[1]->EffectorMode, EPuppetEffectorMode::PelvisRelative);
	}

	const TArray<UPuppetModule_DynamicChain*> Dynamic = Asset.Modules<UPuppetModule_DynamicChain>();
	if (TestEqual(TEXT("The tail and the ear, not the helper bones"), Dynamic.Num(), 2))
	{
		TestEqual(TEXT("The tail's base"), Dynamic[0]->PuppetStart, FName(TEXT("tail_01")));
		TestEqual(TEXT("The tail's tip"), Dynamic[0]->PuppetEnd, FName(TEXT("tail_03")));
		TestEqual(TEXT("The ear"), Dynamic[1]->PuppetEnd, FName(TEXT("ear_02_l")));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetSetupMixamoTest, "Open3DPuppet.Setup.ProposeModules.PuppetBonesKeepTheMeshsNames", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetSetupMixamoTest::RunTest(const FString& Parameters)
{
	PuppetSetupTests::FAsset Asset({
		{ TEXT("mixamorig:Hips"), TEXT(""), FVector(0, 0, 100) },
		{ TEXT("mixamorig:Spine"), TEXT("mixamorig:Hips"), FVector(0, 0, 10) },
		{ TEXT("mixamorig:Spine1"), TEXT("mixamorig:Spine"), FVector(0, 0, 10) },
		{ TEXT("mixamorig:Spine2"), TEXT("mixamorig:Spine1"), FVector(0, 0, 10) },
		{ TEXT("mixamorig:Neck"), TEXT("mixamorig:Spine2"), FVector(0, 0, 10) },
		{ TEXT("mixamorig:Head"), TEXT("mixamorig:Neck"), FVector(0, 0, 10) },
		{ TEXT("mixamorig:LeftShoulder"), TEXT("mixamorig:Spine2"), FVector(0, 5, 0) },
		{ TEXT("mixamorig:LeftArm"), TEXT("mixamorig:LeftShoulder"), FVector(0, 10, 0) },
		{ TEXT("mixamorig:LeftForeArm"), TEXT("mixamorig:LeftArm"), FVector(0, 30, 0) },
		{ TEXT("mixamorig:LeftHand"), TEXT("mixamorig:LeftForeArm"), FVector(0, 25, 0) },
	});
	FString Report;
	if (!TestTrue(TEXT("Proposes"), Asset.Propose(Report)))
	{
		AddError(Report);
		return false;
	}
	TestEqual(TEXT("The pelvis bone"), Asset.Puppet.Asset->PelvisBone, FName(TEXT("mixamorig:Hips")));
	const TArray<UPuppetModule_Chain*> Chains = Asset.Modules<UPuppetModule_Chain>();
	if (TestTrue(TEXT("A spine chain"), Chains.Num() >= 1))
	{
		TestEqual(TEXT("From the mesh's spine"), Chains[0]->PuppetStart, FName(TEXT("mixamorig:Spine")));
		TestEqual(TEXT("To its top"), Chains[0]->PuppetEnd, FName(TEXT("mixamorig:Spine2")));
		TestEqual(TEXT("The performer joint stays canonical"), Chains[0]->PerformerStart, FName(TEXT("spine_01")));
	}
	const TArray<UPuppetModule_TwoBoneLimb*> Limbs = Asset.Modules<UPuppetModule_TwoBoneLimb>();
	if (TestEqual(TEXT("The arm"), Limbs.Num(), 1))
	{
		TestEqual(TEXT("Puppet upper arm"), Limbs[0]->PuppetUpper, FName(TEXT("mixamorig:LeftArm")));
		TestEqual(TEXT("Performer upper arm"), Limbs[0]->PerformerUpper, FName(TEXT("upperarm_l")));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetSetupRefusesTest, "Open3DPuppet.Setup.ProposeModules.LeavesASetUpAssetAlone", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetSetupRefusesTest::RunTest(const FString& Parameters)
{
	FString Report;
	{
		PuppetTests::FPuppet NoMesh(PuppetTests::Biped());
		TestFalse(TEXT("Without a target mesh"), PuppetSetup::ProposeModules(*NoMesh.Asset, PuppetNaming::DefaultSchemes(), Report));
		TestFalse(TEXT("It says why"), Report.IsEmpty());
	}
	{
		PuppetSetupTests::FAsset Asset(PuppetTests::Biped());
		Asset.Puppet.Add<UPuppetModule_Chain>();
		TestFalse(TEXT("With modules already"), Asset.Propose(Report));
		TestEqual(TEXT("They are left as they are"), Asset.Puppet.Asset->Modules.Num(), 1);
		TestFalse(TEXT("It says why"), Report.IsEmpty());
	}
	{
		PuppetSetupTests::FAsset Asset({ { TEXT("Bone_A"), TEXT(""), FVector::ZeroVector }, { TEXT("Bone_B"), TEXT("Bone_A"), FVector(0, 0, 10) } });
		TestFalse(TEXT("With bones no scheme names"), Asset.Propose(Report));
		TestEqual(TEXT("Nothing is added"), Asset.Puppet.Asset->Modules.Num(), 0);
	}
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
