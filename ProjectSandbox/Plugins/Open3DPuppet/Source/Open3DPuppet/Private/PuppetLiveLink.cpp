// Copyright 2026 Lifelike & Believable. All Rights Reserved.

#include "PuppetLiveLink.h"

#include "Features/IModularFeatures.h"
#include "ILiveLinkClient.h"
#include "Roles/LiveLinkAnimationRole.h"
#include "Roles/LiveLinkAnimationTypes.h"
#include "Roles/LiveLinkBasicRole.h"
#include "Roles/LiveLinkTransformRole.h"
#include "Roles/LiveLinkTransformTypes.h"

namespace PuppetLiveLink
{
	// As FAnimNode_LiveLinkPose::PreUpdate does (Engine/Source/Runtime/LiveLinkAnimationCore/Private/AnimNode_LiveLinkPose.cpp).
	ILiveLinkClient* GetClient()
	{
		IModularFeatures& ModularFeatures = IModularFeatures::Get();
		if (!ModularFeatures.IsModularFeatureAvailable(ILiveLinkClient::ModularFeatureName))
		{
			return nullptr;
		}
		return &ModularFeatures.GetModularFeature<ILiveLinkClient>(ILiveLinkClient::ModularFeatureName);
	}

	bool EvaluateSubject(ILiveLinkClient& Client, const FLiveLinkSubjectName& Subject, FLiveLinkSubjectFrameData& OutFrame, FSubjectData& OutData)
	{
		OutData = FSubjectData();
		if (Client.DoesSubjectSupportsRole_AnyThread(Subject, ULiveLinkAnimationRole::StaticClass()))
		{
			if (!Client.EvaluateFrame_AnyThread(Subject, ULiveLinkAnimationRole::StaticClass(), OutFrame))
			{
				return false;
			}
			const FLiveLinkSkeletonStaticData* Skeleton = OutFrame.StaticData.Cast<FLiveLinkSkeletonStaticData>();
			const FLiveLinkAnimationFrameData* Animation = OutFrame.FrameData.Cast<FLiveLinkAnimationFrameData>();
			if (Skeleton && Animation)
			{
				OutData.BoneNames = Skeleton->BoneNames;
				OutData.BoneParents = Skeleton->BoneParents;
				OutData.BoneTransforms = Animation->Transforms;
			}
		}
		// The Transform role derives from the Basic role, so it is asked about first.
		else if (Client.DoesSubjectSupportsRole_AnyThread(Subject, ULiveLinkTransformRole::StaticClass()))
		{
			if (!Client.EvaluateFrame_AnyThread(Subject, ULiveLinkTransformRole::StaticClass(), OutFrame))
			{
				return false;
			}
			static const FName PropNames[] = { FName(TEXT("prop")) };
			static const int32 PropParents[] = { INDEX_NONE };
			if (const FLiveLinkTransformFrameData* Transform = OutFrame.FrameData.Cast<FLiveLinkTransformFrameData>())
			{
				OutData.BoneNames = PropNames;
				OutData.BoneParents = PropParents;
				OutData.BoneTransforms = MakeArrayView(&Transform->Transform, 1);
			}
		}
		else if (!Client.DoesSubjectSupportsRole_AnyThread(Subject, ULiveLinkBasicRole::StaticClass())
			|| !Client.EvaluateFrame_AnyThread(Subject, ULiveLinkBasicRole::StaticClass(), OutFrame))
		{
			return false;
		}
		OutData.Static = OutFrame.StaticData.Cast<FLiveLinkBaseStaticData>();
		OutData.Frame = OutFrame.FrameData.Cast<FLiveLinkBaseFrameData>();
		return OutData.Static && OutData.Frame;
	}
}
