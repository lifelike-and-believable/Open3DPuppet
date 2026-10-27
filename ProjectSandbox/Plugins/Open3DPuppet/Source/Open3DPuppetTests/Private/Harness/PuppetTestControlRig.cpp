// Copyright 2026 Lifelike & Believable. All Rights Reserved.

#include "Harness/PuppetTestControlRig.h"

#include "Rigs/RigHierarchyController.h"
#include "Units/Execution/RigUnit_BeginExecution.h"
#include "Units/Execution/RigUnit_PrepareForExecution.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(PuppetTestControlRig)

namespace PuppetTestControlRig
{
	const FRigElementKey HeadControl(TEXT("head_ctrl"), ERigElementType::Control);
	const FRigElementKey JawOpen(TEXT("jaw_open"), ERigElementType::Control);
	const FRigElementKey PreOpen(TEXT("pre_open"), ERigElementType::Control);
	const FRigElementKey PostTurn(TEXT("post_turn"), ERigElementType::Control);
	const FRigElementKey Head(TEXT("head"), ERigElementType::Bone);
	const FRigElementKey Jaw(TEXT("jaw"), ERigElementType::Bone);
	const FRigElementKey JawCurve(TEXT("jaw_curve"), ERigElementType::Curve);

	/** Amount times 30 degrees about Z. */
	FQuat Yaw(float Amount)
	{
		return FQuat(FVector::ZAxisVector, FMath::DegreesToRadians(30.0 * Amount));
	}
}

UPuppetTestControlRig::UPuppetTestControlRig()
{
	// The hierarchy is built in Initialize, not copied from the class default object.
	bCopyHierarchyBeforeConstruction = false;
	bResetInitialTransformsBeforeConstruction = false;
}

void UPuppetTestControlRig::Initialize(bool bInitRigUnits)
{
	PostInitInstanceIfRequired();
	Super::Initialize(bInitRigUnits);

	URigHierarchy* Hierarchy = GetHierarchy();
	if (!Hierarchy || Hierarchy->Contains(PuppetTestControlRig::Head))
	{
		return;
	}
	// Bones as a rig made on the mesh imports them; the adapter sets their initial transforms.
	URigHierarchyController* Controller = Hierarchy->GetController(true);
	FRigElementKey Parent;
	for (const TCHAR* Bone : { TEXT("root"), TEXT("pelvis"), TEXT("spine_01"), TEXT("spine_02"), TEXT("spine_03"), TEXT("neck_01"), TEXT("head"), TEXT("jaw") })
	{
		const FName Name(Bone);
		// The jaw hangs off the head, like every other bone off the one before it.
		Parent = Controller->AddBone(Name, Parent, FTransform::Identity, false, ERigBoneType::Imported, false);
	}

	FRigControlSettings Transform;
	Transform.ControlType = ERigControlType::Transform;
	Controller->AddControl(PuppetTestControlRig::HeadControl.Name, FRigElementKey(), Transform, FRigControlValue::Make<FRigControlValue::FTransform_Float>(FTransform::Identity), FTransform::Identity, FTransform::Identity, false);
	FRigControlSettings Float;
	Float.ControlType = ERigControlType::Float;
	for (const FRigElementKey& Key : { PuppetTestControlRig::JawOpen, PuppetTestControlRig::PreOpen, PuppetTestControlRig::PostTurn })
	{
		Controller->AddControl(Key.Name, FRigElementKey(), Float, FRigControlValue::Make<float>(0.0f), FTransform::Identity, FTransform::Identity, false);
	}
	Controller->AddCurve(PuppetTestControlRig::JawCurve.Name, 0.0f, false);
}

bool UPuppetTestControlRig::SupportsEvent(const FName& InEventName) const
{
	return InEventName == FRigUnit_PreBeginExecution::EventName || InEventName == FRigUnit_PostBeginExecution::EventName || Super::SupportsEvent(InEventName);
}

bool UPuppetTestControlRig::Execute_Internal(const FName& InEventName)
{
	URigHierarchy* Hierarchy = GetHierarchy();
	FRigTransformElement* Head = Hierarchy->Find<FRigTransformElement>(PuppetTestControlRig::Head);
	FRigTransformElement* Jaw = Hierarchy->Find<FRigTransformElement>(PuppetTestControlRig::Jaw);
	FRigControlElement* HeadControl = Hierarchy->Find<FRigControlElement>(PuppetTestControlRig::HeadControl);
	FRigControlElement* JawOpen = Hierarchy->Find<FRigControlElement>(PuppetTestControlRig::JawOpen);
	FRigControlElement* PreOpen = Hierarchy->Find<FRigControlElement>(PuppetTestControlRig::PreOpen);
	FRigControlElement* PostTurn = Hierarchy->Find<FRigControlElement>(PuppetTestControlRig::PostTurn);
	FRigCurveElement* JawCurve = Hierarchy->Find<FRigCurveElement>(PuppetTestControlRig::JawCurve);
	if (!Head || !Jaw || !HeadControl || !JawOpen || !PreOpen || !PostTurn || !JawCurve)
	{
		return false;
	}

	if (InEventName == FRigUnit_PrepareForExecution::EventName)
	{
		const FTransform HeadInitial = Hierarchy->GetTransform(Head, ERigTransformType::InitialGlobal);
		Hierarchy->SetControlOffsetTransform(HeadControl, HeadInitial, ERigTransformType::InitialGlobal, false);
		Hierarchy->SetControlOffsetTransform(HeadControl, HeadInitial, ERigTransformType::CurrentGlobal, false);
		return true;
	}
	if (InEventName == FRigUnit_PreBeginExecution::EventName)
	{
		const float Open = Hierarchy->GetControlValue(PreOpen, ERigControlValueType::Current).Get<float>();
		if (Open != 0.0f)
		{
			Hierarchy->SetControlValue(JawOpen, FRigControlValue::Make<float>(Open), ERigControlValueType::Current);
		}
		return true;
	}
	if (InEventName == FRigUnit_PostBeginExecution::EventName)
	{
		const float Turn = Hierarchy->GetControlValue(PostTurn, ERigControlValueType::Current).Get<float>();
		FTransform HeadPose = Hierarchy->GetTransform(Head, ERigTransformType::CurrentGlobal);
		HeadPose.SetRotation((PuppetTestControlRig::Yaw(Turn) * HeadPose.GetRotation()).GetNormalized());
		Hierarchy->SetTransform(Head, HeadPose, ERigTransformType::CurrentGlobal, true);
		return true;
	}
	if (InEventName != FRigUnit_BeginExecution::EventName)
	{
		return true;
	}

	const FTransform ControlInitial = Hierarchy->GetTransform(HeadControl, ERigTransformType::InitialGlobal);
	const FTransform Control = Hierarchy->GetTransform(HeadControl, ERigTransformType::CurrentGlobal);
	FTransform HeadPose = Hierarchy->GetTransform(Head, ERigTransformType::CurrentGlobal);
	HeadPose.SetRotation((Control.GetRotation() * ControlInitial.GetRotation().Inverse() * HeadPose.GetRotation()).GetNormalized());
	HeadPose.AddToTranslation(Control.GetTranslation() - ControlInitial.GetTranslation());
	Hierarchy->SetTransform(Head, HeadPose, ERigTransformType::CurrentGlobal, true);

	const float Open = Hierarchy->GetControlValue(JawOpen, ERigControlValueType::Current).Get<float>();
	FTransform JawLocal = Hierarchy->GetTransform(Jaw, ERigTransformType::InitialLocal);
	JawLocal.SetRotation(JawLocal.GetRotation() * PuppetTestControlRig::Yaw(Open));
	Hierarchy->SetTransform(Jaw, JawLocal, ERigTransformType::CurrentLocal, true);
	Hierarchy->SetCurveValue(JawCurve, Open);
	return true;
}
