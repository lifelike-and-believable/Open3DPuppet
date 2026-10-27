// Copyright 2026 Lifelike & Believable. All Rights Reserved.

#include "PuppetModule_ControlRig.h"

#include "ControlRig.h"
#include "PuppetAsset.h"
#include "PuppetPerformerPose.h"
#include "PuppetPose.h"
#include "ReferenceSkeleton.h"
#include "Rigs/RigHierarchy.h"
#include "Units/Execution/RigUnit_BeginExecution.h"
#include "Units/Execution/RigUnit_PrepareForExecution.h"
#include "UObject/GarbageCollection.h"
#include "UObject/Package.h"
#include "UObject/StrongObjectPtr.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(PuppetModule_ControlRig)

namespace PuppetModuleControlRig
{
	bool TakesJointRotation(ERigControlType Type)
	{
		return Type == ERigControlType::Transform || Type == ERigControlType::TransformNoScale || Type == ERigControlType::EulerTransform
			|| Type == ERigControlType::Rotator;
	}

	bool TakesJointTransform(ERigControlType Type)
	{
		return Type == ERigControlType::Transform || Type == ERigControlType::TransformNoScale || Type == ERigControlType::EulerTransform;
	}

	/** The puppet's rest pose as a reference skeleton, for UControlRig::SetBoneInitialTransformsFromRefSkeleton. */
	FReferenceSkeleton RestSkeleton(const FPuppetPose& Puppet)
	{
		FReferenceSkeleton Rest;
		// No USkeleton: the modifier rebuilds the name map when it goes out of scope.
		FReferenceSkeletonModifier Modifier(Rest, nullptr);
		for (int32 Bone = 0; Bone < Puppet.Num(); ++Bone)
		{
			const int32 Parent = Puppet.GetParent(Bone);
			const FTransform Local = Parent == INDEX_NONE ? Puppet.GetReferenceComponent(Bone)
				: Puppet.GetReferenceComponent(Bone).GetRelativeTransform(Puppet.GetReferenceComponent(Parent));
			Modifier.Add(FMeshBoneInfo(Puppet.GetBoneName(Bone), Puppet.GetBoneName(Bone).ToString(), Parent), Local);
		}
		return Rest;
	}

	class FSolver : public TPuppetModuleSolver<UPuppetModule_ControlRig>
	{
	public:
		using TPuppetModuleSolver<UPuppetModule_ControlRig>::TPuppetModuleSolver;

		virtual bool Bind(const FPuppetBindContext& Context, FString& OutError) override
		{
			Rig.Reset();
			PoseIn.Reset();
			Inputs.Reset();
			Outputs.Reset();
			Curves.Reset();
			VariableCopies.Reset();
			VariablesStruct = nullptr;
			if (!Settings.Rig.IsValid())
			{
				OutError = TEXT("no Control Rig is set");
				return false;
			}

			// The puppet and performer first: they need no rig instance.
			for (const FName Name : Settings.Bones)
			{
				const int32 Bone = Context.Puppet.FindBone(Name);
				if (Bone == INDEX_NONE)
				{
					OutError = FString::Printf(TEXT("the puppet has no bone '%s'"), *Name.ToString());
					return false;
				}
				Outputs.Add({ Bone, INDEX_NONE });
			}
			for (const FPuppetControlRigInput& Entry : Settings.Inputs)
			{
				FInput& Input = Inputs.AddDefaulted_GetRef();
				Input.Kind = Entry.Input;
				if (Entry.Input == EPuppetControlRigInput::Curve)
				{
					Input.Source = Context.Performer.FindCurve(Entry.PerformerCurve);
					if (Input.Source == INDEX_NONE)
					{
						OutError = FString::Printf(TEXT("the performer has no curve '%s'"), *Entry.PerformerCurve.ToString());
						return false;
					}
				}
				else
				{
					Input.Source = Context.Performer.FindJoint(Entry.PerformerJoint);
					if (Input.Source == INDEX_NONE)
					{
						OutError = FString::Printf(TEXT("the performer has no joint '%s'"), *Entry.PerformerJoint.ToString());
						return false;
					}
				}
			}

			// Binding runs on an animation worker thread. This follows the engine's Control Rig node,
			// which makes a rig there when its rig changes (AnimNode_ControlRig.cpp,
			// UpdateControlRigIfNeeded): no garbage collection while the object is made, then the
			// Async flag cleared so it can be collected later.
			{
				FGCScopeGuard GCGuard;
				UControlRig* Instance = Settings.Rig.CreateInstance(GetTransientPackage());
				if (!Instance)
				{
					OutError = FString::Printf(TEXT("the Control Rig %s cannot be instanced"), *Settings.Rig.GetPathName());
					return false;
				}
				(void)Instance->AtomicallyClearInternalFlags(EInternalObjectFlags::Async);
				Rig.Reset(Instance);
				Instance->Initialize(true);
				Instance->RequestInit();
			}

			// The rig's bones start at the puppet's rest pose, and construction places its controls there.
			Rig->SetBoneInitialTransformsFromRefSkeleton(RestSkeleton(Context.Puppet));
			Rig->Execute(FRigUnit_PrepareForExecution::EventName);
			URigHierarchy* Hierarchy = Rig->GetHierarchy();
			if (!Hierarchy)
			{
				OutError = FString::Printf(TEXT("the Control Rig %s has no hierarchy"), *Settings.Rig.GetPathName());
				return false;
			}

			for (int32 Bone = 0; Bone < Context.Puppet.Num(); ++Bone)
			{
				const int32 Element = Hierarchy->GetIndex(FRigElementKey(Context.Puppet.GetBoneName(Bone), ERigElementType::Bone));
				if (Element != INDEX_NONE)
				{
					PoseIn.Add({ Bone, Element });
				}
			}
			for (FBonePair& Output : Outputs)
			{
				const FName Name = Context.Puppet.GetBoneName(Output.PuppetBone);
				Output.Element = Hierarchy->GetIndex(FRigElementKey(Name, ERigElementType::Bone));
				if (Output.Element == INDEX_NONE)
				{
					OutError = FString::Printf(TEXT("the Control Rig has no bone '%s'"), *Name.ToString());
					return false;
				}
			}
			// Parents before children, as the reference skeleton orders them.
			Outputs.Sort([](const FBonePair& A, const FBonePair& B) { return A.PuppetBone < B.PuppetBone; });

			for (const FName Name : Settings.Curves)
			{
				const int32 Element = Hierarchy->GetIndex(FRigElementKey(Name, ERigElementType::Curve));
				if (Element == INDEX_NONE)
				{
					OutError = FString::Printf(TEXT("the Control Rig has no curve '%s'"), *Name.ToString());
					return false;
				}
				Curves.Add({ Name, Element });
			}

			VariablesStruct = Settings.Variables.GetPropertyBagStruct();
			if (VariablesStruct)
			{
				const TArray<FRigVMExternalVariable> Public = Rig->GetPublicVariables();
				for (const FPropertyBagPropertyDesc& Desc : VariablesStruct->GetPropertyDescs())
				{
					const FRigVMExternalVariable* Variable = Public.FindByPredicate([&Desc](const FRigVMExternalVariable& Candidate) { return Candidate.GetName() == Desc.Name; });
					if (!Variable || Variable->IsReadOnly() || !Variable->GetMemory())
					{
						OutError = FString::Printf(TEXT("the Control Rig has no variable '%s'"), *Desc.Name.ToString());
						return false;
					}
					if (!Desc.CachedProperty || !Variable->GetProperty() || !Variable->GetProperty()->SameType(Desc.CachedProperty))
					{
						OutError = FString::Printf(TEXT("the Control Rig's variable '%s' is not of the type the module sets"), *Desc.Name.ToString());
						return false;
					}
					VariableCopies.Add({ Desc.CachedProperty, const_cast<uint8*>(Variable->GetMemory()) });
				}
			}

			// The events the engine's Control Rig node runs around the forward solve, when the rig has them.
			bPreForwardsSolve = Rig->SupportsEvent(FRigUnit_PreBeginExecution::EventName);
			bPostForwardsSolve = Rig->SupportsEvent(FRigUnit_PostBeginExecution::EventName);

			for (int32 Index = 0; Index < Inputs.Num(); ++Index)
			{
				FInput& Input = Inputs[Index];
				const FName Name = Settings.Inputs[Index].Control;
				Input.Element = Hierarchy->GetIndex(FRigElementKey(Name, ERigElementType::Control));
				FRigControlElement* Control = Input.Element == INDEX_NONE ? nullptr : Hierarchy->Get<FRigControlElement>(Input.Element);
				if (!Control)
				{
					OutError = FString::Printf(TEXT("the Control Rig has no control '%s'"), *Name.ToString());
					return false;
				}
				const ERigControlType Type = Control->Settings.ControlType;
				if (Input.Kind == EPuppetControlRigInput::JointRotation && !TakesJointRotation(Type))
				{
					OutError = FString::Printf(TEXT("the Control Rig's control '%s' cannot take a joint rotation"), *Name.ToString());
					return false;
				}
				if (Input.Kind == EPuppetControlRigInput::JointTransform && !TakesJointTransform(Type))
				{
					OutError = FString::Printf(TEXT("the Control Rig's control '%s' cannot take a joint transform"), *Name.ToString());
					return false;
				}
				if (Input.Kind == EPuppetControlRigInput::Curve && Type != ERigControlType::Float && Type != ERigControlType::ScaleFloat)
				{
					OutError = FString::Printf(TEXT("the Control Rig's control '%s' is not a float control"), *Name.ToString());
					return false;
				}
				Input.Initial = Hierarchy->GetTransform(Control, ERigTransformType::InitialGlobal);
			}
			return true;
		}

		virtual void Solve(FPuppetSolveContext& Context) override
		{
			UControlRig* Instance = Rig.Get();
			// As the engine's Control Rig node runs a rig (AnimNode_ControlRigBase.cpp, ExecuteControlRig).
			UE::TScopeLock Lock(Instance->GetEvaluateMutex());
			URigHierarchy* Hierarchy = Instance->GetHierarchy();

			for (const FBonePair& Pair : PoseIn)
			{
				Hierarchy->SetTransform(Hierarchy->Get<FRigTransformElement>(Pair.Element), Context.Puppet.GetComponent(Pair.PuppetBone),
					ERigTransformType::CurrentGlobal, false);
			}

			for (int32 Index = 0; Index < Inputs.Num(); ++Index)
			{
				const FInput& Input = Inputs[Index];
				// Read each frame, so tuning applies live; any change to the list binds again, so the indices match.
				const float Scale = Settings.Inputs[Index].Scale;
				FRigControlElement* Control = Hierarchy->Get<FRigControlElement>(Input.Element);
				if (Input.Kind == EPuppetControlRigInput::Curve)
				{
					Hierarchy->SetControlValue(Control, FRigControlValue::Make<float>(Context.GetCurveDelta(Input.Source) * Scale), ERigControlValueType::Current);
					continue;
				}
				FTransform Target = Input.Initial;
				Target.SetRotation((Context.GetJointDelta(Input.Source) * Input.Initial.GetRotation()).GetNormalized());
				if (Input.Kind == EPuppetControlRigInput::JointTransform)
				{
					Target.AddToTranslation(Context.GetJointTranslation(Input.Source) * Context.RootScale * Scale);
				}
				Hierarchy->SetTransform(Control, Target, ERigTransformType::CurrentGlobal, false);
			}

			// Copied only when changed, so a value that allocates (a string) does not every frame. A
			// refreshed bag has new properties; the variables wait for the bind that follows.
			if (VariablesStruct && Settings.Variables.GetPropertyBagStruct() == VariablesStruct)
			{
				const uint8* Values = Settings.Variables.GetValue().GetMemory();
				for (const FVariableCopy& Copy : VariableCopies)
				{
					const void* Value = Copy.Property->ContainerPtrToValuePtr<void>(Values);
					if (!Copy.Property->Identical(Value, Copy.RigValue))
					{
						Copy.Property->CopyCompleteValue(Copy.RigValue, Value);
					}
				}
			}

			if (bPreForwardsSolve)
			{
				Instance->Execute(FRigUnit_PreBeginExecution::EventName);
			}
			Instance->Execute(FRigUnit_BeginExecution::EventName);
			if (bPostForwardsSolve)
			{
				Instance->Execute(FRigUnit_PostBeginExecution::EventName);
			}

			for (const FBonePair& Output : Outputs)
			{
				const FTransform Rigged = Hierarchy->GetTransform(Hierarchy->Get<FRigTransformElement>(Output.Element), ERigTransformType::CurrentGlobal);
				const FTransform& Original = Context.Puppet.GetComponent(Output.PuppetBone);
				const double Weight = Context.Weight;
				FTransform Blended;
				Blended.SetRotation(FQuat::Slerp(Original.GetRotation(), Rigged.GetRotation(), Context.Weight).GetNormalized());
				Blended.SetTranslation(FMath::Lerp(Original.GetTranslation(), Rigged.GetTranslation(), Weight));
				Blended.SetScale3D(FMath::Lerp(Original.GetScale3D(), Rigged.GetScale3D(), Weight));
				Context.Puppet.SetComponent(Output.PuppetBone, Blended);
			}

			// Weighted as Curve Remap's outputs are.
			for (const FCurveOutput& Curve : Curves)
			{
				Context.Puppet.SetCurve(Curve.Name, Hierarchy->GetCurveValue(Curve.Element) * Context.Weight);
			}
		}

	private:
		/** A puppet bone and the rig bone of the same name, by hierarchy element index. */
		struct FBonePair
		{
			int32 PuppetBone = INDEX_NONE;
			int32 Element = INDEX_NONE;
		};

		struct FInput
		{
			EPuppetControlRigInput Kind = EPuppetControlRigInput::JointRotation;
			/** The performer joint or curve. */
			int32 Source = INDEX_NONE;
			/** The control, by hierarchy element index. */
			int32 Element = INDEX_NONE;
			/** The control's global transform after construction. */
			FTransform Initial;
		};

		/** This instance's own rig: the module is shared by every puppet that uses it. */
		TStrongObjectPtr<UControlRig> Rig;
		/** Every puppet bone the rig has, which takes the incoming pose. */
		TArray<FBonePair> PoseIn;
		TArray<FInput> Inputs;
		/** The listed bones, parents first. */
		TArray<FBonePair> Outputs;

		/** A listed rig curve, by name and hierarchy element index. */
		struct FCurveOutput
		{
			FName Name;
			int32 Element = INDEX_NONE;
		};
		TArray<FCurveOutput> Curves;
		/** A module value for a rig variable: the bag's property and the rig variable's memory. */
		struct FVariableCopy
		{
			const FProperty* Property = nullptr;
			void* RigValue = nullptr;
		};
		TArray<FVariableCopy> VariableCopies;
		/** The module's Variables as bound; another struct means they were refreshed since. */
		const UPropertyBag* VariablesStruct = nullptr;
		bool bPreForwardsSolve = false;
		bool bPostForwardsSolve = false;
	};
}

TUniquePtr<FPuppetModuleSolver> UPuppetModule_ControlRig::CreateSolver() const
{
	return MakeUnique<PuppetModuleControlRig::FSolver>(*this);
}

void UPuppetModule_ControlRig::RefreshVariables()
{
	FInstancedPropertyBag Listed;
	UControlRig* Instance = Rig.IsValid() ? Rig.CreateInstance(GetTransientPackage()) : nullptr;
	if (Instance)
	{
		Instance->Initialize(true);
		const TArray<FRigVMExternalVariable> Public = Instance->GetPublicVariables();
		TArray<FPropertyBagPropertyDesc> Descs;
		for (const FRigVMExternalVariable& Variable : Public)
		{
			if (Variable.IsReadOnly() || !Variable.GetProperty() || !Variable.GetMemory())
			{
				continue;
			}
			FPropertyBagPropertyDesc& Desc = Descs.Emplace_GetRef(Variable.GetName(), Variable.GetProperty());
			Desc.PropertyFlags |= static_cast<uint64>(CPF_Edit);
			// The same ID keeps the value already set when the bags are migrated below.
			const FPropertyBagPropertyDesc* Existing = Variables.FindPropertyDescByName(Variable.GetName());
			if (Existing && Existing->CachedProperty && Existing->CachedProperty->SameType(Variable.GetProperty()))
			{
				Desc.ID = Existing->ID;
			}
		}
		Listed.AddProperties(Descs);
		for (const FRigVMExternalVariable& Variable : Public)
		{
			const FPropertyBagPropertyDesc* Desc = Listed.FindPropertyDescByName(Variable.GetName());
			if (Desc && Desc->CachedProperty && Variable.GetMemory())
			{
				Desc->CachedProperty->CopyCompleteValue(Desc->CachedProperty->ContainerPtrToValuePtr<void>(Listed.GetMutableValue().GetMemory()), Variable.GetMemory());
			}
		}
	}
	Variables.MigrateToNewBagInstance(Listed);

	// Solvers hold the old bag's properties, so running puppets bind again.
	if (UPuppetAsset* Asset = GetTypedOuter<UPuppetAsset>())
	{
		Asset->MarkChanged();
	}
}

#if WITH_EDITOR
void UPuppetModule_ControlRig::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	if (PropertyChangedEvent.GetMemberPropertyName() == GET_MEMBER_NAME_CHECKED(UPuppetModule_ControlRig, Rig))
	{
		RefreshVariables();
	}
	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif
