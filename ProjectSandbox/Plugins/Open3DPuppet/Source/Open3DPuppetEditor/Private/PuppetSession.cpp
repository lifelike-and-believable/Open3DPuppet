// Copyright 2026 Lifelike & Believable. All Rights Reserved.

#include "PuppetSession.h"

#include "AnimNode_PuppetSolve.h"
#include "Animation/AnimClassInterface.h"
#include "Animation/AnimInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "Editor.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"
#include "ILiveLinkClient.h"
#include "PuppetCalibrationSubsystem.h"
#include "PuppetInstance.h"
#include "PuppetPerformanceComponent.h"

namespace PuppetSession
{
	TArray<FPerformer> GatherPerformers(const ILiveLinkClient* Client, const UPuppetCalibrationSubsystem* Calibrations)
	{
		TMap<FName, FPerformer> ByName;
		if (Client)
		{
			for (const FLiveLinkSubjectKey& Key : Client->GetSubjects(false, true))
			{
				FPerformer& Performer = ByName.FindOrAdd(Key.SubjectName.Name);
				Performer.Subject = Key.SubjectName.Name;
				Performer.bStreaming = true;
			}
		}
		if (Calibrations)
		{
			for (const FName Subject : Calibrations->GetCalibratedSubjects())
			{
				FPerformer& Performer = ByName.FindOrAdd(Subject);
				Performer.Subject = Subject;
				Performer.bCalibrated = true;
			}
		}
		TArray<FPerformer> Performers;
		ByName.GenerateValueArray(Performers);
		Performers.Sort([](const FPerformer& A, const FPerformer& B) { return A.Subject.LexicalLess(B.Subject); });
		return Performers;
	}

	const FAnimNode_PuppetSolve* FPuppet::FindNode() const
	{
		const UAnimInstance* Instance = AnimInstance.Get();
		const FStructProperty* Property = Instance ? FindFProperty<FStructProperty>(Instance->GetClass(), NodeProperty) : nullptr;
		return Property && Property->Struct == FAnimNode_PuppetSolve::StaticStruct() ? Property->ContainerPtrToValuePtr<FAnimNode_PuppetSolve>(Instance) : nullptr;
	}

	UPuppetAsset* FPuppet::GetPuppet() const
	{
		const FAnimNode_PuppetSolve* Node = FindNode();
		return Node ? Node->Puppet.Get() : nullptr;
	}

	namespace Gathering
	{
		void AddNodes(UAnimInstance* Instance, const AActor* Owner, TArray<FPuppet>& OutPuppets)
		{
			const IAnimClassInterface* AnimClass = Instance ? IAnimClassInterface::GetFromClass(Instance->GetClass()) : nullptr;
			if (!AnimClass)
			{
				return;
			}
			for (const FStructProperty* Property : AnimClass->GetAnimNodeProperties())
			{
				if (Property && Property->Struct == FAnimNode_PuppetSolve::StaticStruct())
				{
					FPuppet& Puppet = OutPuppets.AddDefaulted_GetRef();
					Puppet.AnimInstance = Instance;
					Puppet.NodeProperty = Property->GetFName();
					Puppet.Label = FString::Printf(TEXT("%s (%s)"), Owner ? *Owner->GetActorNameOrLabel() : TEXT("?"), *Instance->GetClass()->GetName());
					Puppet.Performance = Owner ? Owner->FindComponentByClass<UPuppetPerformanceComponent>() : nullptr;
				}
			}
		}
	}

	TArray<FPuppet> GatherPuppets(UWorld* World)
	{
		TArray<FPuppet> Puppets;
		if (!World)
		{
			return Puppets;
		}
		for (TObjectIterator<USkeletalMeshComponent> It; It; ++It)
		{
			USkeletalMeshComponent* Mesh = *It;
			if (Mesh->GetWorld() != World || Mesh->HasAnyFlags(RF_ClassDefaultObject | RF_ArchetypeObject))
			{
				continue;
			}
			Gathering::AddNodes(Mesh->GetAnimInstance(), Mesh->GetOwner(), Puppets);
			for (UAnimInstance* Linked : static_cast<const USkeletalMeshComponent*>(Mesh)->GetLinkedAnimInstances())
			{
				Gathering::AddNodes(Linked, Mesh->GetOwner(), Puppets);
			}
		}
		return Puppets;
	}

	UWorld* SessionWorld()
	{
		if (!GEditor)
		{
			return nullptr;
		}
		if (const FWorldContext* PlayWorld = GEditor->GetPIEWorldContext())
		{
			return PlayWorld->World();
		}
		return GEditor->GetEditorWorldContext().World();
	}

	FString SlotStateText(int32 State)
	{
		switch (static_cast<EPuppetSlotState>(State))
		{
		case EPuppetSlotState::NoData: return TEXT("No data");
		case EPuppetSlotState::NotCalibrated: return TEXT("Not calibrated");
		case EPuppetSlotState::CalibrationMismatch: return TEXT("Calibrated on another skeleton");
		case EPuppetSlotState::Ready: return TEXT("Driving");
		default: return TEXT("Unknown");
		}
	}
}
