// Copyright 2026 Lifelike & Believable. All Rights Reserved.

#pragma once

#include "PuppetAsset.h"
#include "PuppetInstance.h"
#include "ReferenceSkeleton.h"
#include "UObject/StrongObjectPtr.h"

class FAutomationTestBase;
class UAnimSequence;
class USkeletalMesh;
class USkeleton;

// Fixtures for solver tests (ADR 0005): skeletons built in code, and performers that carry the
// same data a Live Link Animation-role subject does (bone names, parent indices, local transforms).
namespace PuppetTests
{
	/** One bone of a skeleton table: name, parent name (empty for the root) and offset from the parent. */
	struct FBone
	{
		const TCHAR* Name;
		const TCHAR* Parent;
		FVector Offset;
	};

	/**
	 * A mannequin-named biped with identity rest rotations: pelvis at 100 cm, a three-bone spine,
	 * neck and head, a left arm along +Y (upper arm 30, forearm 25) and a left leg with the knee
	 * bent forward.
	 */
	TArray<FBone> Biped();

	/** The biped with every offset multiplied by Scale. */
	TArray<FBone> Scaled(TConstArrayView<FBone> Bones, double Scale);

	/** Bones with Name's offset replaced. */
	TArray<FBone> WithOffset(TConstArrayView<FBone> Bones, const TCHAR* Name, const FVector& Offset);

	/**
	 * The biped with a mannequin-named left hand: thumb, index, middle, ring and pinky, each three
	 * bones along +Y (01 10 cm from hand_l, then 4 cm and 4 cm), side by side along X. The palm
	 * faces down, so fingers curl about X and spread about Z.
	 */
	TArray<FBone> Hand();

	/**
	 * Root, pelvis at 48 cm and a left digitigrade leg in the XZ plane, every segment 20 cm and every
	 * rest rotation identity: thigh_l at the hip (0, 10, 48), calf_l at the knee (12, 10, 32),
	 * foot_l at the hock (0, 10, 16) and ball_l at the paw (12, 10, 0). The metatarsal (foot_l to
	 * ball_l) points forward and down, (0.6, 0, -0.8).
	 */
	TArray<FBone> DigitigradeLeg();

	/** The biped with three feathers under hand_l, feather_01 to feather_03, 10 cm out along +Y and 2 cm apart along X. */
	TArray<FBone> Wing();

	/**
	 * The biped with a right arm, mirroring the left along -Y, and a tail fan: tail_01 20 cm behind
	 * the pelvis (along -X) with five sibling feathers under it, fan_01 to fan_05, 10 cm further
	 * back and 4 cm apart from +Y to -Y. Feathers spread about Z.
	 */
	TArray<FBone> Fan();

	/** The biped with a jaw and two eyes under the head, all at identity rest rotations: jaw, eye_l (+Y side), eye_r. */
	TArray<FBone> Face();

	/** The biped with a five-bone digit under hand_l, digit_01 to digit_05, 2 cm apart along +Y. */
	TArray<FBone> Paw();

	/** Root and pelvis (at 100 cm) and a five-bone tail, tail_01 to tail_05, 10 cm apart, upwards. */
	TArray<FBone> Tail();

	FReferenceSkeleton MakeReferenceSkeleton(TConstArrayView<FBone> Bones);

	/** A skeletal mesh with Bones and nothing else (no render data), as a puppet asset's Target Mesh. */
	TStrongObjectPtr<USkeletalMesh> MakeMesh(TConstArrayView<FBone> Bones);

	/** A skeleton asset with Bones, as a performer clip's (ADR 0008). */
	TStrongObjectPtr<USkeleton> MakeSkeleton(TConstArrayView<FBone> Bones);

	/**
	 * A sequence on Skeleton at 30 frames per second, one key per entry of Keys (at least two), each
	 * a local transform per skeleton bone; Keys[1] is at 1/30 s. Built through the editor's animation
	 * data controller, as the engine's animation recorder builds one.
	 */
	TStrongObjectPtr<UAnimSequence> MakeClip(USkeleton& Skeleton, TConstArrayView<TArray<FTransform>> Keys);

	/** A performer as Live Link delivers it, starting in its rest pose: bones, curves, or both. */
	struct FPerformer
	{
		/** A curve-only performer, as a Basic-role subject (a face). */
		FPerformer() = default;
		explicit FPerformer(TConstArrayView<FBone> Bones);

		void SetLocalRotation(FName Bone, const FQuat& Rotation);
		void SetLocalTranslation(FName Bone, const FVector& Translation);
		/** Sets a curve's value, adding the curve if new. */
		void SetCurve(FName Curve, float Value);

		/** The pose as a performer pose (no profile). */
		FPuppetPerformerPose ToPose() const;

		TArray<FName> Names;
		TArray<int32> Parents;
		TArray<FTransform> Local;
		TArray<FName> CurveNames;
		TArray<float> CurveValues;
	};

	/** A puppet asset, a mesh skeleton and a running instance. */
	struct FPuppet
	{
		explicit FPuppet(TConstArrayView<FBone> Bones);

		/** Adds a module of type T to the asset and returns it for setup. */
		template <typename T>
		T& Add()
		{
			T* Module = NewObject<T>(Asset.Get());
			Asset->Modules.Add(Module);
			return *Module;
		}

		/** Creates the instance from the asset's current modules, with RestLocal as the rest pose (empty: the reference pose). */
		void Build(TConstArrayView<FTransform> RestLocal = {});

		/** Gives Slot its performer's current pose. */
		void Drive(int32 Slot, const FPerformer& Performer);

		/** Calibrates Slot from Performer's current pose. */
		void Calibrate(int32 Slot, const FPerformer& Performer, const FRotator& Alignment = FRotator::ZeroRotator);

		/** Solves one frame from the reference pose. */
		void Solve();

		/** Solves one frame from InputLocal, the pose an animation feeds the node. */
		void Solve(TConstArrayView<FTransform> InputLocal, float DeltaTime = 1.0f / 30.0f);

		/** The reference pose with Bone's local transform replaced, as an input pose. */
		TArray<FTransform> InputWith(const TCHAR* Bone, const FTransform& Local) const;

		FTransform Component(const TCHAR* Bone) const;
		FTransform Reference(const TCHAR* Bone) const;

		TStrongObjectPtr<UPuppetAsset> Asset;
		FReferenceSkeleton RefSkeleton;
		FPuppetInstance Instance;
	};

	/** Quaternions equal up to sign (the same rotation). */
	bool TestRotation(FAutomationTestBase& Test, const TCHAR* What, const FQuat& Actual, const FQuat& Expected, double Tolerance = 1.e-4);

	/** A rotation of Degrees about Axis. */
	FQuat Rotation(const FVector& Axis, double Degrees);
}
