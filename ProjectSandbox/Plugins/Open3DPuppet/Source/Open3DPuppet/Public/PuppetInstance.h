// Copyright 2026 Lifelike & Believable. All Rights Reserved.

#pragma once

#include "Containers/Array.h"
#include "Containers/ArrayView.h"
#include "Misc/CoreMiscDefines.h"
#include "PuppetPerformerCalibration.h"
#include "PuppetPerformerPose.h"
#include "PuppetPose.h"
#include "Templates/UniquePtr.h"
#include "UObject/ObjectPtr.h"

class FPuppetModuleSolver;
class UPuppetAsset;
class UPuppetModule;
class UPuppetPerformerProfile;
struct FReferenceSkeleton;

/** Why a performer slot is or is not driving its modules. */
enum class EPuppetSlotState : uint8
{
	/** No performer data has arrived. */
	NoData,
	/** Data, but no calibration for the performer. */
	NotCalibrated,
	/** The calibration was captured from a different skeleton. */
	CalibrationMismatch,
	/** Driving its modules. */
	Ready,
};

/**
 * One running puppet: the puppet pose, the performers' poses and calibrations per slot, and a
 * solver per module (design overview §3). The Puppet Solve node owns one; tests drive it directly.
 * Not thread-safe: one thread at a time, the animation worker in a running node.
 */
class OPEN3DPUPPET_API FPuppetInstance
{
public:
	FPuppetInstance();
	~FPuppetInstance();
	UE_NONCOPYABLE(FPuppetInstance)

	/**
	 * Sets up the puppet for a mesh's reference skeleton and creates the modules' solvers. The
	 * caller keeps Asset and Modules alive while the instance uses them. Allocates.
	 */
	void Initialize(const UPuppetAsset& Asset, TConstArrayView<TObjectPtr<UPuppetModule>> Modules, const FReferenceSkeleton& RefSkeleton);

	bool IsInitialized() const { return Puppet.Num() > 0; }

	/** The slot's index in the asset's PerformerSlots, or INDEX_NONE. */
	int32 FindSlot(FName Slot) const;
	int32 NumSlots() const { return Slots.Num(); }
	FName GetSlotName(int32 Slot) const { return Slots[Slot].Name; }
	EPuppetSlotState GetSlotState(int32 Slot) const;

	/**
	 * Gives a slot its performer's skeleton (from Live Link static data). Does nothing when the
	 * skeleton is unchanged; otherwise the slot's modules bind again. False, with OutError, when
	 * the skeleton is invalid; the slot then has no data. The same invalid skeleton again returns
	 * false with OutError empty, so a caller logs each rejection once.
	 */
	bool SetPerformerSkeleton(int32 Slot, TConstArrayView<FName> BoneNames, TConstArrayView<int32> BoneParents, const UPuppetPerformerProfile* Profile, FString& OutError);

	/** Gives a slot this frame's local bone transforms. False on a count mismatch; the slot keeps its last pose. */
	bool SetPerformerTransforms(int32 Slot, TConstArrayView<FTransform> LocalTransforms);

	/** Gives a slot its performer's curve names (Live Link static data). The slot's modules bind again when they change. */
	void SetPerformerCurveNames(int32 Slot, TConstArrayView<FName> CurveNames, const UPuppetPerformerProfile* Profile);

	/** Gives a slot this frame's curve values. False on a count mismatch; the slot keeps its last values. */
	bool SetPerformerCurveValues(int32 Slot, TConstArrayView<float> Values);

	/**
	 * Sets a slot's control blend (ADR 0008): 0 leaves its modules' bones as the input pose (an
	 * animation), 1 is full performer control. It scales every module weight of the slot. Default 1.
	 */
	void SetControlBlend(int32 Slot, float Blend);
	float GetControlBlend(int32 Slot) const;

	/** Asks the slot's modules to re-centre on the next frame they run (a hand-off or a manual re-centre). */
	void RequestRecentre(int32 Slot);

	/** Sets or clears a slot's calibration; its modules bind again. */
	void SetCalibration(int32 Slot, const FPuppetPerformerCalibration& Calibration);
	void ClearCalibration(int32 Slot);

	/** The slot's current performer pose (for capturing a calibration). */
	const FPuppetPerformerPose& GetPerformerPose(int32 Slot) const { return Slots[Slot].Performer; }

	/**
	 * Solves one frame: starts from InputLocal (one local transform per reference-skeleton bone)
	 * and runs every enabled module whose slot is ready, binding first where needed.
	 * ComponentToWorld is the puppet component's world transform, for dynamic modules (ADR 0007).
	 */
	void Solve(TConstArrayView<FTransform> InputLocal, float DeltaTime, const FTransform& ComponentToWorld = FTransform::Identity);

	/** The solved pose. */
	const FPuppetPose& GetPose() const { return Puppet; }

private:
	struct FSlot
	{
		FName Name;
		FPuppetPerformerPose Performer;
		FPuppetPerformerCalibration Calibration;
		bool bHasCalibration = false;
		FQuat Alignment = FQuat::Identity;
		int32 PerformerPelvis = INDEX_NONE;
		double RootScale = 1.0;
		bool bNeedsBind = true;
		float ControlBlend = 1.0f;
		/** A re-centre waiting for the slot's modules to run. */
		bool bRecentre = false;
		bool bRecentreDelivered = false;
		/** Neutral value per performer curve, from the calibration (ADR 0006). */
		TArray<float> CurveNeutral;
		/** The skeleton last rejected by SetPerformerSkeleton, so it is reported once. */
		TArray<FName> RejectedNames;
		TArray<int32> RejectedParents;
	};

	struct FModuleEntry
	{
		const UPuppetModule* Module = nullptr;
		TUniquePtr<FPuppetModuleSolver> Solver;
		int32 Slot = INDEX_NONE;
		bool bBound = false;
	};

	void InvalidateSlot(int32 Slot);
	void BindSlot(int32 Slot);

	FPuppetPose Puppet;
	TArray<FSlot> Slots;
	TArray<FModuleEntry> Entries;
	int32 RootSlot = INDEX_NONE;
	/** The hand-off offset (ADR 0008), puppet-wide: written by the root slot's Root module, read by every module. */
	FTransform HandoffOffset = FTransform::Identity;
	int32 PuppetPelvis = INDEX_NONE;
	FName AssetName;
	double TeleportDistance = 0.0;
	/** The component's world transform at the last solve, and whether there was one. */
	FTransform PreviousComponentToWorld = FTransform::Identity;
	bool bHasPreviousComponentToWorld = false;
};
