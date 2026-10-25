// Copyright 2026 Lifelike & Believable. All Rights Reserved.

#include "PuppetTestFixtures.h"

#include "Misc/AutomationTest.h"
#include "PuppetAsset.h"
#include "PuppetPerformerCalibration.h"

namespace PuppetTests
{
	TArray<FBone> Biped()
	{
		return {
			{ TEXT("root"), TEXT(""), FVector(0, 0, 0) },
			{ TEXT("pelvis"), TEXT("root"), FVector(0, 0, 100) },
			{ TEXT("spine_01"), TEXT("pelvis"), FVector(0, 0, 10) },
			{ TEXT("spine_02"), TEXT("spine_01"), FVector(0, 0, 10) },
			{ TEXT("spine_03"), TEXT("spine_02"), FVector(0, 0, 10) },
			{ TEXT("neck_01"), TEXT("spine_03"), FVector(0, 0, 10) },
			{ TEXT("head"), TEXT("neck_01"), FVector(0, 0, 10) },
			{ TEXT("upperarm_l"), TEXT("spine_03"), FVector(0, 15, 0) },
			{ TEXT("lowerarm_l"), TEXT("upperarm_l"), FVector(0, 30, 0) },
			{ TEXT("hand_l"), TEXT("lowerarm_l"), FVector(0, 25, 0) },
			{ TEXT("thigh_l"), TEXT("pelvis"), FVector(0, 10, 0) },
			{ TEXT("calf_l"), TEXT("thigh_l"), FVector(5, 0, -45) },
			{ TEXT("foot_l"), TEXT("calf_l"), FVector(-5, 0, -45) },
		};
	}

	TArray<FBone> Scaled(TConstArrayView<FBone> Bones, double Scale)
	{
		TArray<FBone> Result(Bones);
		for (FBone& Bone : Result)
		{
			Bone.Offset *= Scale;
		}
		return Result;
	}

	TArray<FBone> WithOffset(TConstArrayView<FBone> Bones, const TCHAR* Name, const FVector& Offset)
	{
		TArray<FBone> Result(Bones);
		for (FBone& Bone : Result)
		{
			if (FCString::Strcmp(Bone.Name, Name) == 0)
			{
				Bone.Offset = Offset;
			}
		}
		return Result;
	}

	TArray<FBone> Hand()
	{
		TArray<FBone> Bones = Biped();
		Bones.Append({
			{ TEXT("thumb_01_l"), TEXT("hand_l"), FVector(4, 10, 0) },
			{ TEXT("thumb_02_l"), TEXT("thumb_01_l"), FVector(0, 4, 0) },
			{ TEXT("thumb_03_l"), TEXT("thumb_02_l"), FVector(0, 4, 0) },
			{ TEXT("index_01_l"), TEXT("hand_l"), FVector(2, 10, 0) },
			{ TEXT("index_02_l"), TEXT("index_01_l"), FVector(0, 4, 0) },
			{ TEXT("index_03_l"), TEXT("index_02_l"), FVector(0, 4, 0) },
			{ TEXT("middle_01_l"), TEXT("hand_l"), FVector(0, 10, 0) },
			{ TEXT("middle_02_l"), TEXT("middle_01_l"), FVector(0, 4, 0) },
			{ TEXT("middle_03_l"), TEXT("middle_02_l"), FVector(0, 4, 0) },
			{ TEXT("ring_01_l"), TEXT("hand_l"), FVector(-2, 10, 0) },
			{ TEXT("ring_02_l"), TEXT("ring_01_l"), FVector(0, 4, 0) },
			{ TEXT("ring_03_l"), TEXT("ring_02_l"), FVector(0, 4, 0) },
			{ TEXT("pinky_01_l"), TEXT("hand_l"), FVector(-4, 10, 0) },
			{ TEXT("pinky_02_l"), TEXT("pinky_01_l"), FVector(0, 4, 0) },
			{ TEXT("pinky_03_l"), TEXT("pinky_02_l"), FVector(0, 4, 0) },
		});
		return Bones;
	}

	TArray<FBone> DigitigradeLeg()
	{
		return {
			{ TEXT("root"), TEXT(""), FVector(0, 0, 0) },
			{ TEXT("pelvis"), TEXT("root"), FVector(0, 0, 48) },
			{ TEXT("thigh_l"), TEXT("pelvis"), FVector(0, 10, 0) },
			{ TEXT("calf_l"), TEXT("thigh_l"), FVector(12, 0, -16) },
			{ TEXT("foot_l"), TEXT("calf_l"), FVector(-12, 0, -16) },
			{ TEXT("ball_l"), TEXT("foot_l"), FVector(12, 0, -16) },
		};
	}

	TArray<FBone> Wing()
	{
		TArray<FBone> Bones = Biped();
		Bones.Append({
			{ TEXT("feather_01"), TEXT("hand_l"), FVector(2, 10, 0) },
			{ TEXT("feather_02"), TEXT("hand_l"), FVector(0, 10, 0) },
			{ TEXT("feather_03"), TEXT("hand_l"), FVector(-2, 10, 0) },
		});
		return Bones;
	}

	TArray<FBone> Face()
	{
		TArray<FBone> Bones = Biped();
		Bones.Append({
			{ TEXT("jaw"), TEXT("head"), FVector(3, 0, -2) },
			{ TEXT("eye_l"), TEXT("head"), FVector(8, 3, 5) },
			{ TEXT("eye_r"), TEXT("head"), FVector(8, -3, 5) },
		});
		return Bones;
	}

	TArray<FBone> Paw()
	{
		TArray<FBone> Bones = Biped();
		Bones.Append({
			{ TEXT("digit_01"), TEXT("hand_l"), FVector(0, 2, 0) },
			{ TEXT("digit_02"), TEXT("digit_01"), FVector(0, 2, 0) },
			{ TEXT("digit_03"), TEXT("digit_02"), FVector(0, 2, 0) },
			{ TEXT("digit_04"), TEXT("digit_03"), FVector(0, 2, 0) },
			{ TEXT("digit_05"), TEXT("digit_04"), FVector(0, 2, 0) },
		});
		return Bones;
	}

	TArray<FBone> Tail()
	{
		return {
			{ TEXT("root"), TEXT(""), FVector(0, 0, 0) },
			{ TEXT("pelvis"), TEXT("root"), FVector(0, 0, 100) },
			{ TEXT("tail_01"), TEXT("pelvis"), FVector(0, 0, 10) },
			{ TEXT("tail_02"), TEXT("tail_01"), FVector(0, 0, 10) },
			{ TEXT("tail_03"), TEXT("tail_02"), FVector(0, 0, 10) },
			{ TEXT("tail_04"), TEXT("tail_03"), FVector(0, 0, 10) },
			{ TEXT("tail_05"), TEXT("tail_04"), FVector(0, 0, 10) },
		};
	}

	namespace
	{
		int32 ParentIndex(TConstArrayView<FBone> Bones, int32 Index)
		{
			for (int32 Candidate = 0; Candidate < Index; ++Candidate)
			{
				if (FCString::Strcmp(Bones[Candidate].Name, Bones[Index].Parent) == 0)
				{
					return Candidate;
				}
			}
			return INDEX_NONE;
		}
	}

	FReferenceSkeleton MakeReferenceSkeleton(TConstArrayView<FBone> Bones)
	{
		FReferenceSkeleton RefSkeleton;
		{
			// No USkeleton: the modifier rebuilds the name map when it goes out of scope.
			FReferenceSkeletonModifier Modifier(RefSkeleton, nullptr);
			for (int32 Index = 0; Index < Bones.Num(); ++Index)
			{
				Modifier.Add(FMeshBoneInfo(Bones[Index].Name, Bones[Index].Name, ParentIndex(Bones, Index)), FTransform(Bones[Index].Offset));
			}
		}
		return RefSkeleton;
	}

	FPerformer::FPerformer(TConstArrayView<FBone> Bones)
	{
		for (int32 Index = 0; Index < Bones.Num(); ++Index)
		{
			Names.Add(Bones[Index].Name);
			Parents.Add(ParentIndex(Bones, Index));
			Local.Add(FTransform(Bones[Index].Offset));
		}
	}

	void FPerformer::SetLocalRotation(FName Bone, const FQuat& Rotation)
	{
		Local[Names.IndexOfByKey(Bone)].SetRotation(Rotation);
	}

	void FPerformer::SetLocalTranslation(FName Bone, const FVector& Translation)
	{
		Local[Names.IndexOfByKey(Bone)].SetTranslation(Translation);
	}

	void FPerformer::SetCurve(FName Curve, float Value)
	{
		const int32 Index = CurveNames.IndexOfByKey(Curve);
		if (Index == INDEX_NONE)
		{
			CurveNames.Add(Curve);
			CurveValues.Add(Value);
		}
		else
		{
			CurveValues[Index] = Value;
		}
	}

	FPuppetPerformerPose FPerformer::ToPose() const
	{
		FPuppetPerformerPose Pose;
		if (Names.Num() > 0)
		{
			FString Error;
			verify(Pose.SetSkeleton(Names, Parents, nullptr, Error));
			verify(Pose.SetLocalTransforms(Local));
		}
		Pose.SetCurveNames(CurveNames, nullptr);
		Pose.SetCurveValues(CurveValues);
		return Pose;
	}

	FPuppet::FPuppet(TConstArrayView<FBone> Bones)
		: Asset(NewObject<UPuppetAsset>())
		, RefSkeleton(MakeReferenceSkeleton(Bones))
	{
	}

	void FPuppet::Build()
	{
		Instance.Initialize(*Asset, Asset->Modules, RefSkeleton);
	}

	void FPuppet::Drive(int32 Slot, const FPerformer& Performer)
	{
		if (Performer.Names.Num() > 0)
		{
			FString Error;
			verify(Instance.SetPerformerSkeleton(Slot, Performer.Names, Performer.Parents, nullptr, Error));
			verify(Instance.SetPerformerTransforms(Slot, Performer.Local));
		}
		Instance.SetPerformerCurveNames(Slot, Performer.CurveNames, nullptr);
		Instance.SetPerformerCurveValues(Slot, Performer.CurveValues);
	}

	void FPuppet::Calibrate(int32 Slot, const FPerformer& Performer, const FRotator& Alignment)
	{
		Instance.SetCalibration(Slot, FPuppetPerformerCalibration::Capture(Performer.ToPose(), Alignment));
	}

	void FPuppet::Solve()
	{
		Instance.Solve(RefSkeleton.GetRefBonePose(), 1.0f / 30.0f);
	}

	void FPuppet::Solve(TConstArrayView<FTransform> InputLocal, float DeltaTime)
	{
		Instance.Solve(InputLocal, DeltaTime);
	}

	TArray<FTransform> FPuppet::InputWith(const TCHAR* Bone, const FTransform& Local) const
	{
		TArray<FTransform> Input = RefSkeleton.GetRefBonePose();
		Input[RefSkeleton.FindBoneIndex(Bone)] = Local;
		return Input;
	}

	FTransform FPuppet::Component(const TCHAR* Bone) const
	{
		const FPuppetPose& Pose = Instance.GetPose();
		return Pose.GetComponent(Pose.FindBone(Bone));
	}

	FTransform FPuppet::Reference(const TCHAR* Bone) const
	{
		const FPuppetPose& Pose = Instance.GetPose();
		return Pose.GetReferenceComponent(Pose.FindBone(Bone));
	}

	bool TestRotation(FAutomationTestBase& Test, const TCHAR* What, const FQuat& Actual, const FQuat& Expected, double Tolerance)
	{
		if (Actual.Equals(Expected, Tolerance) || Actual.Equals(-Expected, Tolerance))
		{
			return true;
		}
		Test.AddError(FString::Printf(TEXT("%s: expected %s, got %s"), What, *Expected.ToString(), *Actual.ToString()));
		return false;
	}

	FQuat Rotation(const FVector& Axis, double Degrees)
	{
		return FQuat(Axis.GetSafeNormal(), FMath::DegreesToRadians(Degrees));
	}
}
