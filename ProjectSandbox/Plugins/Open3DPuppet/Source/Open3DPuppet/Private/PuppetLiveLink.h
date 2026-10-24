// Copyright 2026 Lifelike & Believable. All Rights Reserved.

#pragma once

#include "LiveLinkTypes.h"

class ILiveLinkClient;
struct FLiveLinkAnimationFrameData;
struct FLiveLinkSkeletonStaticData;

namespace PuppetLiveLink
{
	/** The Live Link client, or null when the Live Link plugin is not loaded. Game thread. */
	ILiveLinkClient* GetClient();

	/**
	 * Evaluates Subject with the Animation role into OutFrame (ADR 0003). On success, OutSkeleton
	 * and OutAnimation point into OutFrame. False when the subject does not exist, has no
	 * Animation role or has no frame yet. Any thread.
	 */
	bool EvaluateAnimation(ILiveLinkClient& Client, const FLiveLinkSubjectName& Subject, FLiveLinkSubjectFrameData& OutFrame,
		const FLiveLinkSkeletonStaticData*& OutSkeleton, const FLiveLinkAnimationFrameData*& OutAnimation);
}
