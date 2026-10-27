// Copyright 2026 Lifelike & Believable. All Rights Reserved.

#include "PuppetAsset.h"

#include "Engine/SkeletalMesh.h"
#include "PuppetModule.h"
#include "PuppetNaming.h"

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

#if WITH_EDITOR
void UPuppetAsset::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	MarkChanged();
}
#endif
