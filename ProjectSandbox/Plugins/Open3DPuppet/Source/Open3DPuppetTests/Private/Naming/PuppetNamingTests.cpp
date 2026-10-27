// Copyright 2026 Lifelike & Believable. All Rights Reserved.

// Naming schemes and profile auto-mapping (WP-2): the default schemes, picking the scheme that fits
// a source skeleton, and filling a performer profile from it.

#include "PuppetTestHarness.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Animation/Skeleton.h"
#include "Harness/PuppetTestFixtures.h"
#include "PuppetNaming.h"
#include "PuppetPerformerProfile.h"
#include "PuppetSettings.h"

namespace PuppetNamingTests
{
	/** A bone table that owns its names, for skeletons named in code. */
	struct FBody
	{
		TArray<FString> Names;
		TArray<FString> Parents;

		void Add(const FString& Name, const FString& Parent)
		{
			Names.Add(Name);
			Parents.Add(Parent);
		}

		/** The table as fixture bones; they point into this body, so use them while it lives. */
		TArray<PuppetTests::FBone> Bones() const
		{
			TArray<PuppetTests::FBone> Result;
			for (int32 Bone = 0; Bone < Names.Num(); ++Bone)
			{
				Result.Add({ *Names[Bone], *Parents[Bone], FVector(0, 0, 10) });
			}
			return Result;
		}
	};

	/**
	 * A body (no fingers) with every name behind Prefix: pelvis, four spine bones (Spine3 the top),
	 * neck, head, arms and legs, under a root. Without Spine3 when bTopIsSpine2.
	 */
	FBody Body(const FString& Prefix, bool bTopIsSpine2)
	{
		FBody Body;
		const auto Add = [&Body, &Prefix](const TCHAR* Name, const TCHAR* Parent) { Body.Add(Prefix + Name, *Parent ? Prefix + Parent : FString()); };
		const TCHAR* Top = bTopIsSpine2 ? TEXT("Spine2") : TEXT("Spine3");
		Add(TEXT("Reference"), TEXT(""));
		Add(TEXT("Hips"), TEXT("Reference"));
		Add(TEXT("Spine"), TEXT("Hips"));
		Add(TEXT("Spine1"), TEXT("Spine"));
		Add(TEXT("Spine2"), TEXT("Spine1"));
		if (!bTopIsSpine2)
		{
			Add(TEXT("Spine3"), TEXT("Spine2"));
		}
		Add(TEXT("Neck"), Top);
		Add(TEXT("Head"), TEXT("Neck"));
		for (const TCHAR* Side : { TEXT("Left"), TEXT("Right") })
		{
			const FString S(Side);
			Add(*(S + TEXT("Shoulder")), Top);
			Add(*(S + TEXT("Arm")), *(S + TEXT("Shoulder")));
			Add(*(S + TEXT("ForeArm")), *(S + TEXT("Arm")));
			Add(*(S + TEXT("Hand")), *(S + TEXT("ForeArm")));
			Add(*(S + TEXT("UpLeg")), TEXT("Hips"));
			Add(*(S + TEXT("Leg")), *(S + TEXT("UpLeg")));
			Add(*(S + TEXT("Foot")), *(S + TEXT("Leg")));
			Add(*(S + TEXT("ToeBase")), *(S + TEXT("Foot")));
		}
		return Body;
	}

	/** Names and parents of a bone table, as Live Link gives them. */
	struct FSource
	{
		explicit FSource(TConstArrayView<PuppetTests::FBone> Bones) : Performer(Bones) {}
		PuppetTests::FPerformer Performer;
		TConstArrayView<FName> Names() const { return Performer.Names; }
		TConstArrayView<int32> Parents() const { return Performer.Parents; }
	};

	const FPuppetNamingScheme* Find(TConstArrayView<FPuppetNamingScheme> Schemes, const TCHAR* Name)
	{
		return Schemes.FindByPredicate([Name](const FPuppetNamingScheme& Scheme) { return Scheme.Name == Name; });
	}

	FName Mapped(const TMap<FName, FName>& Mapping, const TCHAR* Canonical)
	{
		const FName* Source = Mapping.Find(Canonical);
		return Source ? *Source : NAME_None;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetNamingDefaultsTest, "Open3DPuppet.Naming.Defaults.CommonCaptureSystems", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetNamingDefaultsTest::RunTest(const FString& Parameters)
{
	const TArray<FPuppetNamingScheme> Schemes = PuppetNaming::DefaultSchemes();
	for (const TCHAR* Name : { TEXT("UE5 Mannequin"), TEXT("Xsens"), TEXT("OptiTrack / Motive"), TEXT("Rokoko"), TEXT("Vicon"), TEXT("Qualisys"),
		TEXT("Sony Mocopi"), TEXT("Move One"), TEXT("HumanIK / Motionbuilder"), TEXT("Mixamo"), TEXT("Unity humanoid / VMC") })
	{
		TestNotNull(FString::Printf(TEXT("A default scheme for %s"), Name), PuppetNamingTests::Find(Schemes, Name));
	}
	if (Schemes.Num() == 0)
	{
		return false;
	}
	TestEqual(TEXT("The canonical names come first"), Schemes[0].Name, FString(TEXT("UE5 Mannequin")));
	const FPuppetNamingScheme* Xsens = PuppetNamingTests::Find(Schemes, TEXT("Xsens"));
	const FPuppetNamingScheme* Rokoko = PuppetNamingTests::Find(Schemes, TEXT("Rokoko"));
	const FPuppetNamingScheme* OptiTrack = PuppetNamingTests::Find(Schemes, TEXT("OptiTrack / Motive"));
	if (Xsens && Rokoko && OptiTrack)
	{
		TestEqual(TEXT("Xsens: the top of the spine"), PuppetNamingTests::Mapped(Xsens->CanonicalToSource, TEXT("spine_05")), FName(TEXT("Spine3")));
		TestEqual(TEXT("Rokoko: the calf"), PuppetNamingTests::Mapped(Rokoko->CanonicalToSource, TEXT("calf_l")), FName(TEXT("LeftShin")));
		TestEqual(TEXT("OptiTrack: the top of the spine"), PuppetNamingTests::Mapped(OptiTrack->CanonicalToSource, TEXT("spine_05")), FName(TEXT("spine_02")));
	}
	TestTrue(TEXT("Project Settings start with the defaults"), GetDefault<UPuppetSettings>()->NamingSchemes.Num() >= Schemes.Num());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetNamingMatchTest, "Open3DPuppet.Naming.Match.PicksTheSchemeByNamesThenParents", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetNamingMatchTest::RunTest(const FString& Parameters)
{
	const TArray<FPuppetNamingScheme> Schemes = PuppetNaming::DefaultSchemes();

	// Mixamo's names are all on this skeleton too (it has a Spine2), so the parents decide: the
	// neck and shoulders hang from Spine3.
	const PuppetNamingTests::FBody XsensBody = PuppetNamingTests::Body(TEXT(""), false);
	const PuppetNamingTests::FSource Xsens(XsensBody.Bones());
	const PuppetNaming::FMatch XsensMatch = PuppetNaming::Match(Xsens.Names(), Xsens.Parents(), Schemes);
	if (!TestTrue(TEXT("A Spine3 skeleton matches a scheme"), Schemes.IsValidIndex(XsensMatch.Scheme)))
	{
		return false;
	}
	TestNotEqual(TEXT("Not as Mixamo"), Schemes[XsensMatch.Scheme].Name, FString(TEXT("Mixamo")));
	TestEqual(TEXT("The top of the spine"), PuppetNamingTests::Mapped(XsensMatch.CanonicalToSource, TEXT("spine_05")), FName(TEXT("Spine3")));
	TestEqual(TEXT("The pelvis"), PuppetNamingTests::Mapped(XsensMatch.CanonicalToSource, TEXT("pelvis")), FName(TEXT("Hips")));
	TestEqual(TEXT("The calf"), PuppetNamingTests::Mapped(XsensMatch.CanonicalToSource, TEXT("calf_r")), FName(TEXT("RightLeg")));
	TestTrue(TEXT("The fingers are missing"), XsensMatch.Missing.Contains(FName(TEXT("index_01_l"))));

	// Mixamo exports carry a namespace; the mapping keeps it, as the bones arrive with it.
	const PuppetNamingTests::FBody MixamoBody = PuppetNamingTests::Body(TEXT("mixamorig:"), true);
	const PuppetNamingTests::FSource Mixamo(MixamoBody.Bones());
	const PuppetNaming::FMatch MixamoMatch = PuppetNaming::Match(Mixamo.Names(), Mixamo.Parents(), Schemes);
	if (!TestTrue(TEXT("A Mixamo skeleton matches a scheme"), Schemes.IsValidIndex(MixamoMatch.Scheme)))
	{
		return false;
	}
	TestEqual(TEXT("As Mixamo"), Schemes[MixamoMatch.Scheme].Name, FString(TEXT("Mixamo")));
	TestEqual(TEXT("The top of the spine, namespace and all"), PuppetNamingTests::Mapped(MixamoMatch.CanonicalToSource, TEXT("spine_05")), FName(TEXT("mixamorig:Spine2")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetNamingPrefixTest, "Open3DPuppet.Naming.Match.SharedPrefixesAndCanonicalNames", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetNamingPrefixTest::RunTest(const FString& Parameters)
{
	const TArray<FPuppetNamingScheme> Schemes = PuppetNaming::DefaultSchemes();

	// OptiTrack's names with a skeleton name in front of every bone, in another case.
	const PuppetNamingTests::FSource OptiTrack({
		{ TEXT("Actor_Root"), TEXT(""), FVector::ZeroVector },
		{ TEXT("Actor_Pelvis"), TEXT("Actor_Root"), FVector(0, 0, 100) },
		{ TEXT("Actor_Spine_01"), TEXT("Actor_Pelvis"), FVector(0, 0, 10) },
		{ TEXT("Actor_Spine_02"), TEXT("Actor_Spine_01"), FVector(0, 0, 10) },
		{ TEXT("Actor_Neck_01"), TEXT("Actor_Spine_02"), FVector(0, 0, 10) },
		{ TEXT("Actor_Head"), TEXT("Actor_Neck_01"), FVector(0, 0, 10) },
	});
	const PuppetNaming::FMatch OptiTrackMatch = PuppetNaming::Match(OptiTrack.Names(), OptiTrack.Parents(), Schemes);
	if (!TestTrue(TEXT("A prefixed skeleton matches a scheme"), Schemes.IsValidIndex(OptiTrackMatch.Scheme)))
	{
		return false;
	}
	TestEqual(TEXT("As OptiTrack"), Schemes[OptiTrackMatch.Scheme].Name, FString(TEXT("OptiTrack / Motive")));
	TestEqual(TEXT("The pelvis as the source spells it"), PuppetNamingTests::Mapped(OptiTrackMatch.CanonicalToSource, TEXT("pelvis")), FName(TEXT("Actor_Pelvis")));
	TestEqual(TEXT("The top of the spine"), PuppetNamingTests::Mapped(OptiTrackMatch.CanonicalToSource, TEXT("spine_05")), FName(TEXT("Actor_Spine_02")));

	// A mannequin-named source needs no profile: the canonical scheme wins and maps nothing.
	const PuppetNamingTests::FSource Mannequin(PuppetTests::Biped());
	const PuppetNaming::FMatch MannequinMatch = PuppetNaming::Match(Mannequin.Names(), Mannequin.Parents(), Schemes);
	if (TestTrue(TEXT("A mannequin matches a scheme"), Schemes.IsValidIndex(MannequinMatch.Scheme)))
	{
		TestEqual(TEXT("As the canonical names"), Schemes[MannequinMatch.Scheme].Name, FString(TEXT("UE5 Mannequin")));
		TestEqual(TEXT("Nothing to map"), MannequinMatch.CanonicalToSource.Num(), 0);
	}

	const PuppetNamingTests::FSource Unrelated({ { TEXT("Bone_A"), TEXT(""), FVector::ZeroVector }, { TEXT("Bone_B"), TEXT("Bone_A"), FVector(0, 0, 10) } });
	TestEqual(TEXT("Unrelated names match nothing"), PuppetNaming::Match(Unrelated.Names(), Unrelated.Parents(), Schemes).Scheme, INDEX_NONE);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetNamingProfileTest, "Open3DPuppet.Naming.Profile.MapsBonesFromASkeleton", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetNamingProfileTest::RunTest(const FString& Parameters)
{
	UPuppetPerformerProfile* Profile = NewObject<UPuppetPerformerProfile>();
	Profile->CurveCanonicalToSource.Add(TEXT("jawOpen"), TEXT("JawOpen"));
	Profile->CanonicalToSource.Add(TEXT("stale"), TEXT("Stale"));

	const PuppetNamingTests::FSource Unrelated({ { TEXT("Bone_A"), TEXT(""), FVector::ZeroVector } });
	TestFalse(TEXT("No scheme fits"), Profile->MapBones(Unrelated.Names(), Unrelated.Parents(), PuppetNaming::DefaultSchemes()));
	TestTrue(TEXT("The mapping is unchanged"), Profile->CanonicalToSource.Contains(FName(TEXT("stale"))));
	TestFalse(TEXT("And the result says why"), Profile->MappingResult.IsEmpty());

	// Map From Skeleton reads a skeleton asset's bones and the schemes in Project Settings.
	const PuppetNamingTests::FBody MixamoBody = PuppetNamingTests::Body(TEXT("mixamorig:"), true);
	const TStrongObjectPtr<USkeleton> Skeleton = PuppetTests::MakeSkeleton(MixamoBody.Bones());
	Profile->SourceSkeleton = Skeleton.Get();
	Profile->MapFromSkeleton();
	TestEqual(TEXT("The top of the spine"), PuppetNamingTests::Mapped(Profile->CanonicalToSource, TEXT("spine_05")), FName(TEXT("mixamorig:Spine2")));
	TestEqual(TEXT("The hand"), PuppetNamingTests::Mapped(Profile->CanonicalToSource, TEXT("hand_l")), FName(TEXT("mixamorig:LeftHand")));
	TestFalse(TEXT("The old mapping is replaced"), Profile->CanonicalToSource.Contains(FName(TEXT("stale"))));
	TestEqual(TEXT("Curves are left as they are"), Profile->CurveCanonicalToSource.Num(), 1);
	TestTrue(TEXT("The result names the scheme"), Profile->MappingResult.Contains(TEXT("Mixamo")));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
