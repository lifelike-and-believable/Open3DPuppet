// Copyright 2026 Lifelike & Believable. All Rights Reserved.

#include "PuppetLiveLink.h"

#include "Features/IModularFeatures.h"
#include "ILiveLinkClient.h"
#include "Roles/LiveLinkAnimationRole.h"
#include "Roles/LiveLinkAnimationTypes.h"

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

	bool EvaluateAnimation(ILiveLinkClient& Client, const FLiveLinkSubjectName& Subject, FLiveLinkSubjectFrameData& OutFrame,
		const FLiveLinkSkeletonStaticData*& OutSkeleton, const FLiveLinkAnimationFrameData*& OutAnimation)
	{
		OutSkeleton = nullptr;
		OutAnimation = nullptr;
		if (!Client.DoesSubjectSupportsRole_AnyThread(Subject, ULiveLinkAnimationRole::StaticClass())
			|| !Client.EvaluateFrame_AnyThread(Subject, ULiveLinkAnimationRole::StaticClass(), OutFrame))
		{
			return false;
		}
		OutSkeleton = OutFrame.StaticData.Cast<FLiveLinkSkeletonStaticData>();
		OutAnimation = OutFrame.FrameData.Cast<FLiveLinkAnimationFrameData>();
		return OutSkeleton && OutAnimation;
	}
}
