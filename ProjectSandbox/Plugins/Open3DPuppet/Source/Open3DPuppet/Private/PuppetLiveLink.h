// Copyright 2026 Lifelike & Believable. All Rights Reserved.

#pragma once

#include "LiveLinkTypes.h"

class ILiveLinkClient;
struct FLiveLinkAnimationFrameData;
struct FLiveLinkSkeletonStaticData;

namespace PuppetLiveLink
{
	/** One evaluated subject. The pointers point into the frame data the evaluation filled. */
	struct FSubjectData
	{
		/** Curve names (every role has them). */
		const FLiveLinkBaseStaticData* Static = nullptr;
		/** Curve values. */
		const FLiveLinkBaseFrameData* Frame = nullptr;
		/** Bones; null for a curve-only (Basic-role) subject. */
		const FLiveLinkSkeletonStaticData* Skeleton = nullptr;
		/** Bone transforms; null for a curve-only subject. */
		const FLiveLinkAnimationFrameData* Animation = nullptr;
	};

	/** The Live Link client, or null when the Live Link plugin is not loaded. Game thread. */
	ILiveLinkClient* GetClient();

	/**
	 * Evaluates Subject into OutFrame with the Animation role (ADR 0003), or, for a subject without
	 * it, the Basic role (curves only, ADR 0006), as FAnimNode_LiveLinkPose does. False when the
	 * subject does not exist or has no frame yet. Any thread.
	 */
	bool EvaluateSubject(ILiveLinkClient& Client, const FLiveLinkSubjectName& Subject, FLiveLinkSubjectFrameData& OutFrame, FSubjectData& OutData);
}
