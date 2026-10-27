// Copyright 2026 Lifelike & Believable. All Rights Reserved.

#include "PuppetSetup.h"

#include "Engine/SkeletalMesh.h"
#include "PuppetAsset.h"
#include "PuppetModules/PuppetModule_Chain.h"
#include "PuppetModules/PuppetModule_Digits.h"
#include "PuppetModules/PuppetModule_DynamicChain.h"
#include "PuppetModules/PuppetModule_Eyes.h"
#include "PuppetModules/PuppetModule_Root.h"
#include "PuppetModules/PuppetModule_TwoBoneLimb.h"

namespace PuppetSetup
{
	namespace Proposing
	{
		/** The mesh's skeleton, with the bone each canonical joint is, and the bones the proposed modules claim. */
		struct FMesh
		{
			const FReferenceSkeleton& Skeleton;
			TMap<FName, int32> Joints;
			TArray<TArray<int32>> Children;
			TSet<int32> Claimed;

			FMesh(const FReferenceSkeleton& InSkeleton) : Skeleton(InSkeleton)
			{
				Children.SetNum(Skeleton.GetNum());
				for (int32 Bone = 0; Bone < Skeleton.GetNum(); ++Bone)
				{
					const int32 Parent = Skeleton.GetParentIndex(Bone);
					if (Parent == INDEX_NONE)
					{
						// The skeleton's root bones belong to no chain.
						Claimed.Add(Bone);
					}
					else
					{
						Children[Parent].Add(Bone);
					}
				}
			}

			int32 Joint(const FString& Canonical) const
			{
				const int32* Bone = Joints.Find(FName(Canonical));
				return Bone ? *Bone : INDEX_NONE;
			}

			FName Name(int32 Bone) const { return Skeleton.GetBoneName(Bone); }

			/** The bones from Start down to End, Start first; empty unless End is Start or below it. */
			TArray<int32> Path(int32 Start, int32 End) const
			{
				TArray<int32> Bones;
				for (int32 Bone = End; Start != INDEX_NONE && Bone != INDEX_NONE; Bone = Skeleton.GetParentIndex(Bone))
				{
					Bones.Insert(Bone, 0);
					if (Bone == Start)
					{
						return Bones;
					}
				}
				return {};
			}

			void Claim(TConstArrayView<int32> Bones) { Claimed.Append(Bones); }
		};

		/** A bone meshes carry for rigging rather than motion: IK targets, twist and corrective bones. */
		bool IsHelper(FName Bone)
		{
			const FString Name = Bone.ToString().ToLower();
			return Name.StartsWith(TEXT("ik_")) || Name.Contains(TEXT("_ik")) || Name.Contains(TEXT("twist")) || Name.Contains(TEXT("corrective"));
		}

		template <typename T>
		T& Add(UPuppetAsset& Asset)
		{
			T* Module = NewObject<T>(&Asset, NAME_None, RF_Transactional);
			Asset.Modules.Add(Module);
			return *Module;
		}
	}

	bool ProposeModules(UPuppetAsset& Asset, TConstArrayView<FPuppetNamingScheme> Schemes, FString& OutReport)
	{
		using namespace Proposing;
		const USkeletalMesh* MeshAsset = Asset.TargetMesh.LoadSynchronous();
		if (!MeshAsset)
		{
			OutReport = TEXT("Set Target Mesh first.");
			return false;
		}
		if (Asset.Modules.Num() > 0)
		{
			OutReport = TEXT("Propose Modules fills an empty module list; remove the modules first to start over.");
			return false;
		}
		FMesh Mesh(MeshAsset->GetRefSkeleton());
		TArray<FName> Names;
		TArray<int32> Parents;
		for (int32 Bone = 0; Bone < Mesh.Skeleton.GetNum(); ++Bone)
		{
			Names.Add(Mesh.Skeleton.GetBoneName(Bone));
			Parents.Add(Mesh.Skeleton.GetParentIndex(Bone));
		}
		const PuppetNaming::FMatch Match = PuppetNaming::Match(Names, Parents, Schemes);
		if (Match.Scheme == INDEX_NONE)
		{
			OutReport = TEXT("No naming scheme in Project Settings > Open3DPuppet names the mesh's bones; add the modules by hand.");
			return false;
		}
		for (const TPair<FName, FName>& Row : Schemes[Match.Scheme].CanonicalToSource)
		{
			if (!Match.Missing.Contains(Row.Key))
			{
				const FName* Source = Match.CanonicalToSource.Find(Row.Key);
				const int32 Bone = Mesh.Skeleton.FindBoneIndex(Source ? *Source : Row.Key);
				if (Bone != INDEX_NONE)
				{
					Mesh.Joints.Add(Row.Key, Bone);
				}
			}
		}
		// Every recognised joint is the body's, so none of them starts a leftover chain.
		for (const TPair<FName, int32>& Joint : Mesh.Joints)
		{
			Mesh.Claimed.Add(Joint.Value);
		}

		Asset.Modify();
		TArray<FString> Proposed;
		const int32 Pelvis = Mesh.Joint(TEXT("pelvis"));
		if (Pelvis != INDEX_NONE)
		{
			Asset.PelvisBone = Mesh.Name(Pelvis);
			Add<UPuppetModule_Root>(Asset);
			Proposed.Add(FString::Printf(TEXT("Root (%s)"), *Mesh.Name(Pelvis).ToString()));
		}

		const auto AddChain = [&Asset, &Mesh, &Proposed](const TCHAR* PerformerStart, const TCHAR* PerformerEnd, int32 Start, int32 End)
		{
			const TArray<int32> Path = Mesh.Path(Start, End);
			if (Path.Num() == 0)
			{
				return;
			}
			UPuppetModule_Chain& Chain = Add<UPuppetModule_Chain>(Asset);
			Chain.PerformerStart = PerformerStart;
			Chain.PerformerEnd = PerformerEnd;
			Chain.PuppetStart = Mesh.Name(Start);
			Chain.PuppetEnd = Mesh.Name(End);
			Mesh.Claim(Path);
			Proposed.Add(FString::Printf(TEXT("Chain %s to %s"), *Chain.PuppetStart.ToString(), *Chain.PuppetEnd.ToString()));
		};
		// The spine ends at the top of the mesh's spine, or where the neck hangs when the scheme has no top.
		const int32 Neck = Mesh.Joint(TEXT("neck_01"));
		int32 SpineTop = Mesh.Joint(TEXT("spine_05"));
		if (SpineTop == INDEX_NONE && Neck != INDEX_NONE)
		{
			SpineTop = Mesh.Skeleton.GetParentIndex(Neck);
		}
		AddChain(TEXT("spine_01"), TEXT("spine_05"), Mesh.Joint(TEXT("spine_01")), SpineTop);
		AddChain(TEXT("neck_01"), TEXT("head"), Neck, Mesh.Joint(TEXT("head")));

		for (const TCHAR* Side : { TEXT("_l"), TEXT("_r") })
		{
			const auto Joint = [Side](const TCHAR* Name) { return FString(Name) + Side; };
			const auto AddLimb = [&](const TCHAR* Upper, const TCHAR* Lower, const TCHAR* End, EPuppetEffectorMode Mode)
			{
				const int32 UpperBone = Mesh.Joint(Joint(Upper));
				const int32 LowerBone = Mesh.Joint(Joint(Lower));
				const int32 EndBone = Mesh.Joint(Joint(End));
				const TArray<int32> Path = Mesh.Path(UpperBone, EndBone);
				if (LowerBone == INDEX_NONE || !Path.Contains(LowerBone))
				{
					return;
				}
				UPuppetModule_TwoBoneLimb& Limb = Add<UPuppetModule_TwoBoneLimb>(Asset);
				Limb.PerformerUpper = *Joint(Upper);
				Limb.PerformerLower = *Joint(Lower);
				Limb.PerformerEnd = *Joint(End);
				Limb.PuppetUpper = Mesh.Name(UpperBone);
				Limb.PuppetLower = Mesh.Name(LowerBone);
				Limb.PuppetEnd = Mesh.Name(EndBone);
				Limb.EffectorMode = Mode;
				Mesh.Claim(Path);
				Proposed.Add(FString::Printf(TEXT("Two-Bone Limb %s to %s"), *Limb.PuppetUpper.ToString(), *Limb.PuppetEnd.ToString()));
			};
			AddLimb(TEXT("upperarm"), TEXT("lowerarm"), TEXT("hand"), EPuppetEffectorMode::ScaledReach);
			AddLimb(TEXT("thigh"), TEXT("calf"), TEXT("foot"), EPuppetEffectorMode::PelvisRelative);

			TArray<FPuppetDigit> Digits;
			for (const TCHAR* Finger : { TEXT("thumb"), TEXT("index"), TEXT("middle"), TEXT("ring"), TEXT("pinky") })
			{
				const FString First = FString::Printf(TEXT("%s_01%s"), Finger, Side);
				const FString Last = FString::Printf(TEXT("%s_03%s"), Finger, Side);
				const TArray<int32> Path = Mesh.Path(Mesh.Joint(First), Mesh.Joint(Last));
				if (Path.Num() > 0)
				{
					FPuppetDigit& Digit = Digits.AddDefaulted_GetRef();
					Digit.PerformerStart = *First;
					Digit.PerformerEnd = *Last;
					Digit.PuppetStart = Mesh.Name(Path[0]);
					Digit.PuppetEnd = Mesh.Name(Path.Last());
					Mesh.Claim(Path);
				}
			}
			if (Digits.Num() > 0)
			{
				Add<UPuppetModule_Digits>(Asset).Digits = Digits;
				Proposed.Add(FString::Printf(TEXT("Digits (%d fingers, hand%s)"), Digits.Num(), Side));
			}
		}

		const int32 LeftEye = Mesh.Joint(TEXT("eye_l"));
		const int32 RightEye = Mesh.Joint(TEXT("eye_r"));
		if (LeftEye != INDEX_NONE && RightEye != INDEX_NONE)
		{
			UPuppetModule_Eyes& Eyes = Add<UPuppetModule_Eyes>(Asset);
			Eyes.PuppetLeftEye = Mesh.Name(LeftEye);
			Eyes.PuppetRightEye = Mesh.Name(RightEye);
			Proposed.Add(TEXT("Eyes"));
		}

		// Leftover chains: a run of unclaimed bones from a claimed parent, while each has one child.
		for (int32 Bone = 0; Bone < Mesh.Skeleton.GetNum(); ++Bone)
		{
			const int32 Parent = Mesh.Skeleton.GetParentIndex(Bone);
			if (Mesh.Claimed.Contains(Bone) || Parent == INDEX_NONE || !Mesh.Claimed.Contains(Parent) || IsHelper(Mesh.Name(Bone)))
			{
				continue;
			}
			TArray<int32> Chain = { Bone };
			while (Mesh.Children[Chain.Last()].Num() == 1)
			{
				const int32 Child = Mesh.Children[Chain.Last()][0];
				if (Mesh.Claimed.Contains(Child) || IsHelper(Mesh.Name(Child)))
				{
					break;
				}
				Chain.Add(Child);
			}
			if (Chain.Num() < 2)
			{
				continue;
			}
			UPuppetModule_DynamicChain& Dynamic = Add<UPuppetModule_DynamicChain>(Asset);
			Dynamic.PuppetStart = Mesh.Name(Chain[0]);
			Dynamic.PuppetEnd = Mesh.Name(Chain.Last());
			Mesh.Claim(Chain);
			Proposed.Add(FString::Printf(TEXT("Dynamic Chain %s to %s"), *Dynamic.PuppetStart.ToString(), *Dynamic.PuppetEnd.ToString()));
		}

		Asset.MarkChanged();
		OutReport = FString::Printf(TEXT("Proposed %d modules from the mesh's bones named as %s: %s. Check the right hand's Digits Spread Axis, and tune the Dynamic Chains' Stiffness and Damping."),
			Asset.Modules.Num(), *Schemes[Match.Scheme].Name, *FString::Join(Proposed, TEXT("; ")));
		return Asset.Modules.Num() > 0;
	}
}
