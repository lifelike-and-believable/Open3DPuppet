// Copyright 2026 Lifelike & Believable. All Rights Reserved.

#include "PuppetLiveLink.h"

#include "Features/IModularFeatures.h"
#include "ILiveLinkClient.h"
#include "Roles/LiveLinkAnimationRole.h"
#include "Roles/LiveLinkAnimationTypes.h"
#include "Roles/LiveLinkBasicRole.h"

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
			OutData.Skeleton = OutFrame.StaticData.Cast<FLiveLinkSkeletonStaticData>();
			OutData.Animation = OutFrame.FrameData.Cast<FLiveLinkAnimationFrameData>();
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
