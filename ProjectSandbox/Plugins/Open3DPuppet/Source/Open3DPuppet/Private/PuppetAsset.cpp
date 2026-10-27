// Copyright 2026 Lifelike & Believable. All Rights Reserved.

#include "PuppetAsset.h"

#include "Engine/SkeletalMesh.h"
#include "PuppetModule.h"
#include "PuppetNaming.h"
#include "PuppetSettings.h"
#include "PuppetSetup.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(PuppetAsset)

TArray<FName> UPuppetAsset::GetPuppetBoneOptions() const
{
	const USkeletalMesh* Mesh = TargetMesh.LoadSynchronous();
	if (!Mesh)
	{
		return PuppetNaming::CanonicalJoints();
	}
	TArray<FName> Names;
	for (const FMeshBoneInfo& Bone : Mesh->GetRefSkeleton().GetRefBoneInfo())
	{
		Names.Add(Bone.Name);
	}
	return Names;
}

TArray<FName> UPuppetAsset::GetPerformerSlotOptions() const
{
	return PerformerSlots;
}

void UPuppetAsset::ProposeModules()
{
	PuppetSetup::ProposeModules(*this, GetDefault<UPuppetSettings>()->NamingSchemes, SetupResult);
}

#if WITH_EDITOR
void UPuppetAsset::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	// A number set in a module, edited through Modules, applies live as it does on the module itself;
	// the asset's own settings are read when the puppet initialises, so they bind again.
	const FProperty* Member = PropertyChangedEvent.MemberProperty;
	if (!(Member && Member->GetFName() == GET_MEMBER_NAME_CHECKED(UPuppetAsset, Modules) && PuppetEditing::IsLiveEdit(PropertyChangedEvent)))
	{
		MarkChanged();
	}
}
#endif
