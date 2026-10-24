// Copyright 2026 Lifelike & Believable. All Rights Reserved.

#include "PuppetInstance.h"

#include "Open3DPuppetLog.h"
#include "PuppetAsset.h"
#include "PuppetModule.h"

namespace PuppetInstance
{
	const FName PerformerPelvisJoint(TEXT("pelvis"));

	template <typename T>
	bool SameElements(const TArray<T>& A, TConstArrayView<T> B)
	{
		if (A.Num() != B.Num())
		{
			return false;
		}
		for (int32 Index = 0; Index < A.Num(); ++Index)
		{
			if (A[Index] != B[Index])
			{
				return false;
			}
		}
		return true;
	}
}

FPuppetInstance::FPuppetInstance() = default;
FPuppetInstance::~FPuppetInstance() = default;

void FPuppetInstance::Initialize(const UPuppetAsset& Asset, TConstArrayView<TObjectPtr<UPuppetModule>> Modules, const FReferenceSkeleton& RefSkeleton)
{
	AssetName = Asset.GetFName();
	Puppet.Initialize(RefSkeleton);
	PuppetPelvis = Puppet.FindBone(Asset.PelvisBone);
	if (PuppetPelvis == INDEX_NONE)
	{
		UE_LOG(LogOpen3DPuppet, Warning, TEXT("Puppet %s: the mesh has no pelvis bone '%s'; root scale is 1."), *AssetName.ToString(), *Asset.PelvisBone.ToString());
	}

	Slots.Reset();
	for (const FName SlotName : Asset.PerformerSlots)
	{
		Slots.AddDefaulted_GetRef().Name = SlotName;
	}
	RootSlot = FindSlot(Asset.RootSlot);
	HandoffOffset = FTransform::Identity;

	Entries.Reset();
	for (const TObjectPtr<UPuppetModule>& Module : Modules)
	{
		if (!Module)
		{
			continue;
		}
		FModuleEntry& Entry = Entries.AddDefaulted_GetRef();
		Entry.Module = Module.Get();
		Entry.Solver = Module->CreateSolver();
		Entry.Slot = FindSlot(Module->PerformerSlot);
		if (Entry.Slot == INDEX_NONE)
		{
			UE_LOG(LogOpen3DPuppet, Warning, TEXT("Puppet %s: module %s reads slot '%s', which the puppet does not declare; it will not run."),
				*AssetName.ToString(), *Module->GetName(), *Module->PerformerSlot.ToString());
		}
	}
}

int32 FPuppetInstance::FindSlot(FName Slot) const
{
	return Slots.IndexOfByPredicate([Slot](const FSlot& Candidate) { return Candidate.Name == Slot; });
}

EPuppetSlotState FPuppetInstance::GetSlotState(int32 Slot) const
{
	const FSlot& State = Slots[Slot];
	if (!State.Performer.HasData())
	{
		return EPuppetSlotState::NoData;
	}
	if (!State.bHasCalibration)
	{
		return EPuppetSlotState::NotCalibrated;
	}
	if (!State.Calibration.Matches(State.Performer))
	{
		return EPuppetSlotState::CalibrationMismatch;
	}
	return EPuppetSlotState::Ready;
}

bool FPuppetInstance::SetPerformerSkeleton(int32 Slot, TConstArrayView<FName> BoneNames, TConstArrayView<int32> BoneParents, const UPuppetPerformerProfile* Profile, FString& OutError)
{
	FSlot& State = Slots[Slot];
	if (State.Performer.HasSkeleton(BoneNames, BoneParents))
	{
		return true;
	}
	if (PuppetInstance::SameElements(State.RejectedNames, BoneNames) && PuppetInstance::SameElements(State.RejectedParents, BoneParents))
	{
		OutError.Reset();
		return false;
	}
	InvalidateSlot(Slot);
	if (!State.Performer.SetSkeleton(BoneNames, BoneParents, Profile, OutError))
	{
		State.RejectedNames = BoneNames;
		State.RejectedParents = BoneParents;
		return false;
	}
	State.RejectedNames.Reset();
	State.RejectedParents.Reset();
	return true;
}

bool FPuppetInstance::SetPerformerTransforms(int32 Slot, TConstArrayView<FTransform> LocalTransforms)
{
	return Slots[Slot].Performer.SetLocalTransforms(LocalTransforms);
}

void FPuppetInstance::SetPerformerCurveNames(int32 Slot, TConstArrayView<FName> CurveNames, const UPuppetPerformerProfile* Profile)
{
	FSlot& State = Slots[Slot];
	if (State.Performer.HasCurveNames(CurveNames))
	{
		return;
	}
	InvalidateSlot(Slot);
	State.Performer.SetCurveNames(CurveNames, Profile);
}

bool FPuppetInstance::SetPerformerCurveValues(int32 Slot, TConstArrayView<float> Values)
{
	return Slots[Slot].Performer.SetCurveValues(Values);
}

void FPuppetInstance::SetControlBlend(int32 Slot, float Blend)
{
	Slots[Slot].ControlBlend = FMath::Clamp(Blend, 0.0f, 1.0f);
}

float FPuppetInstance::GetControlBlend(int32 Slot) const
{
	return Slots[Slot].ControlBlend;
}

void FPuppetInstance::RequestRecentre(int32 Slot)
{
	Slots[Slot].bRecentre = true;
}

void FPuppetInstance::SetCalibration(int32 Slot, const FPuppetPerformerCalibration& Calibration)
{
	FSlot& State = Slots[Slot];
	State.Calibration = Calibration;
	State.bHasCalibration = true;
	State.Alignment = Calibration.Alignment.Quaternion();
	InvalidateSlot(Slot);
}

void FPuppetInstance::ClearCalibration(int32 Slot)
{
	Slots[Slot].bHasCalibration = false;
	InvalidateSlot(Slot);
}

void FPuppetInstance::InvalidateSlot(int32 Slot)
{
	Slots[Slot].bNeedsBind = true;
	for (FModuleEntry& Entry : Entries)
	{
		if (Entry.Slot == Slot)
		{
			Entry.bBound = false;
		}
	}
}

void FPuppetInstance::BindSlot(int32 Slot)
{
	FSlot& State = Slots[Slot];
	State.bNeedsBind = false;
	if (Slot == RootSlot)
	{
		// The Root module's convergence state starts over with the new binding.
		HandoffOffset = FTransform::Identity;
	}
	const TConstArrayView<FName> CurveNames = State.Performer.GetCurveNames();
	State.CurveNeutral.SetNum(CurveNames.Num());
	for (int32 Curve = 0; Curve < CurveNames.Num(); ++Curve)
	{
		State.CurveNeutral[Curve] = State.Calibration.GetRestCurve(CurveNames[Curve]);
	}
	State.PerformerPelvis = State.Performer.FindJoint(PuppetInstance::PerformerPelvisJoint);
	State.RootScale = 1.0;
	if (PuppetPelvis != INDEX_NONE && State.PerformerPelvis != INDEX_NONE)
	{
		const double PuppetHeight = Puppet.GetReferenceComponent(PuppetPelvis).GetTranslation().Z;
		const double PerformerHeight = State.Calibration.RestComponent[State.PerformerPelvis].GetTranslation().Z;
		if (PuppetHeight > UE_KINDA_SMALL_NUMBER && PerformerHeight > UE_KINDA_SMALL_NUMBER)
		{
			State.RootScale = PuppetHeight / PerformerHeight;
		}
	}

	const FPuppetBindContext Context{ Puppet, State.Performer, State.Calibration, PuppetPelvis, State.PerformerPelvis, State.RootScale, Slot == RootSlot };
	for (FModuleEntry& Entry : Entries)
	{
		if (Entry.Slot != Slot)
		{
			continue;
		}
		FString Error;
		Entry.bBound = Entry.Solver && Entry.Solver->Bind(Context, Error);
		if (!Entry.bBound)
		{
			UE_LOG(LogOpen3DPuppet, Warning, TEXT("Puppet %s: module %s cannot run: %s"), *AssetName.ToString(), *Entry.Module->GetName(), *Error);
		}
	}
}

void FPuppetInstance::Solve(TConstArrayView<FTransform> InputLocal, float DeltaTime)
{
	Puppet.SetLocalTransforms(InputLocal);
	Puppet.ResetCurves();

	for (int32 Slot = 0; Slot < Slots.Num(); ++Slot)
	{
		if (GetSlotState(Slot) != EPuppetSlotState::Ready)
		{
			continue;
		}
		if (Slots[Slot].bNeedsBind)
		{
			BindSlot(Slot);
		}
		Slots[Slot].Performer.UpdateDerivedCurves(Slots[Slot].Calibration);
	}

	for (FModuleEntry& Entry : Entries)
	{
		if (!Entry.bBound || !Entry.Module->bEnabled || GetSlotState(Entry.Slot) != EPuppetSlotState::Ready)
		{
			continue;
		}
		FSlot& State = Slots[Entry.Slot];
		const float Weight = Entry.Module->Weight * State.ControlBlend;
		if (Weight <= 0.0f)
		{
			continue;
		}
		FPuppetSolveContext Context{ Puppet, State.Performer, State.Calibration, State.Alignment, PuppetPelvis, State.PerformerPelvis, State.RootScale, DeltaTime,
			State.CurveNeutral, Weight, State.bRecentre, HandoffOffset };
		Entry.Solver->Solve(Context);
		if (Entry.Slot == RootSlot)
		{
			HandoffOffset = Context.HandoffOffset;
		}
		State.bRecentreDelivered |= State.bRecentre;
	}

	// Cleared only once every module of the slot that runs this frame has seen it.
	for (FSlot& State : Slots)
	{
		if (State.bRecentreDelivered)
		{
			State.bRecentre = false;
			State.bRecentreDelivered = false;
		}
	}
}
