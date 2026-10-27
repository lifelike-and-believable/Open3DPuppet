// Copyright 2026 Lifelike & Believable. All Rights Reserved.

#include "PuppetPerformerProfile.h"

#include "Animation/Skeleton.h"
#include "PuppetLiveLink.h"
#include "PuppetSettings.h"
#include "Roles/LiveLinkAnimationTypes.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(PuppetPerformerProfile)

void UPuppetPerformerProfile::MapFromSkeleton()
{
	const USkeleton* Skeleton = SourceSkeleton.LoadSynchronous();
	if (!Skeleton)
	{
		MappingResult = TEXT("Set Source Skeleton to a skeleton named as the source names its bones.");
		return;
	}
	const FReferenceSkeleton& RefSkeleton = Skeleton->GetReferenceSkeleton();
	TArray<FName> Names;
	TArray<int32> Parents;
	for (int32 Bone = 0; Bone < RefSkeleton.GetNum(); ++Bone)
	{
		Names.Add(RefSkeleton.GetBoneName(Bone));
		Parents.Add(RefSkeleton.GetParentIndex(Bone));
	}
	MapBones(Names, Parents, GetDefault<UPuppetSettings>()->NamingSchemes);
}

void UPuppetPerformerProfile::MapFromLiveLinkSubject()
{
	ILiveLinkClient* Client = PuppetLiveLink::GetClient();
	FLiveLinkSubjectFrameData Frame;
	PuppetLiveLink::FSubjectData Data;
	if (!Client || !PuppetLiveLink::EvaluateSubject(*Client, SourceSubject, Frame, Data) || !Data.HasBones())
	{
		MappingResult = FString::Printf(TEXT("Live Link subject '%s' has sent no bones yet."), *SourceSubject.ToString());
		return;
	}
	MapBones(Data.BoneNames, Data.BoneParents, GetDefault<UPuppetSettings>()->NamingSchemes);
}

bool UPuppetPerformerProfile::MapBones(TConstArrayView<FName> SourceBones, TConstArrayView<int32> SourceParents, TConstArrayView<FPuppetNamingScheme> Schemes)
{
	const PuppetNaming::FMatch Match = PuppetNaming::Match(SourceBones, SourceParents, Schemes);
	if (Match.Scheme == INDEX_NONE)
	{
		MappingResult = FString::Printf(TEXT("No naming scheme names any of the %d bones. Map them by hand, or add a scheme in Project Settings > Open3DPuppet."),
			SourceBones.Num());
		return false;
	}

	Modify();
	CanonicalToSource = Match.CanonicalToSource;
	TArray<FString> Names;
	for (const int32 Scheme : Match.Equivalent)
	{
		Names.Add(Schemes[Scheme].Name);
	}
	const FString SchemeNames = FString::Join(Names, TEXT(" / "));
	TArray<FString> Missing;
	for (const FName Joint : Match.Missing)
	{
		Missing.Add(Joint.ToString());
	}
	MappingResult = Match.CanonicalToSource.Num() == 0
		? FString::Printf(TEXT("The bones already use canonical names (%s); no bone mapping is needed."), *SchemeNames)
		: FString::Printf(TEXT("Mapped %d joints as %s."), Match.CanonicalToSource.Num(), *SchemeNames);
	if (Missing.Num() > 0)
	{
		MappingResult += FString::Printf(TEXT(" Not found: %s."), *FString::Join(Missing, TEXT(", ")));
	}
	return true;
}
