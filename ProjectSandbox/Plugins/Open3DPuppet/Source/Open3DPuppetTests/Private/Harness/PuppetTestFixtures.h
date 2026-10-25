// Copyright 2026 Lifelike & Believable. All Rights Reserved.

#pragma once

#include "PuppetAsset.h"
#include "PuppetInstance.h"
#include "ReferenceSkeleton.h"
#include "UObject/StrongObjectPtr.h"

class FAutomationTestBase;

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

	/** The biped with a jaw and two eyes under the head, all at identity rest rotations: jaw, eye_l (+Y side), eye_r. */
	TArray<FBone> Face();

	/** The biped with a five-bone digit under hand_l, digit_01 to digit_05, 2 cm apart along +Y. */
	TArray<FBone> Paw();

	/** Root and pelvis (at 100 cm) and a five-bone tail, tail_01 to tail_05, 10 cm apart, upwards. */
	TArray<FBone> Tail();

	FReferenceSkeleton MakeReferenceSkeleton(TConstArrayView<FBone> Bones);

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

		/** Creates the instance from the asset's current modules. */
		void Build();

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
