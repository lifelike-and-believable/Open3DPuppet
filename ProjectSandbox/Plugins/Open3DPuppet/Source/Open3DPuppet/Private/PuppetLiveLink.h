// Copyright 2026 Lifelike & Believable. All Rights Reserved.

#pragma once

#include "Containers/ArrayView.h"
#include "LiveLinkTypes.h"
#include "Math/Transform.h"

class ILiveLinkClient;

namespace PuppetLiveLink
{
	/**
	 * One evaluated subject. The pointers and views point into the frame data the evaluation filled,
	 * so they last as long as it does.
	 */
	struct FSubjectData
	{
		/** Curve names (every role has them). */
		const FLiveLinkBaseStaticData* Static = nullptr;
		/** Curve values. */
		const FLiveLinkBaseFrameData* Frame = nullptr;
		/**
		 * Bones, parents and local transforms: an Animation-role subject's skeleton, or one joint
		 * named `prop` for a Transform-role subject (a tracked prop, WP-7). Empty for a curve-only
		 * (Basic-role) subject.
		 */
		TConstArrayView<FName> BoneNames;
		TConstArrayView<int32> BoneParents;
		TConstArrayView<FTransform> BoneTransforms;

		bool HasBones() const { return BoneNames.Num() > 0; }
	};

	/** The Live Link client, or null when the Live Link plugin is not loaded. Game thread. */
	ILiveLinkClient* GetClient();

	/**
	 * Evaluates Subject into OutFrame with the Animation role (ADR 0003), else the Transform role (a
	 * tracked prop, one joint), else the Basic role (curves only, ADR 0006), as
	 * FAnimNode_LiveLinkPose does. False when the subject does not exist or has no frame yet. Any
	 * thread.
	 */
	bool EvaluateSubject(ILiveLinkClient& Client, const FLiveLinkSubjectName& Subject, FLiveLinkSubjectFrameData& OutFrame, FSubjectData& OutData);
}
