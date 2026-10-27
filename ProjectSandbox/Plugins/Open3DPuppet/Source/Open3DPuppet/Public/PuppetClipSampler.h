// Copyright 2026 Lifelike & Believable. All Rights Reserved.

#pragma once

#include "BoneContainer.h"
#include "Containers/Array.h"
#include "Containers/ArrayView.h"
#include "Containers/UnrealString.h"
#include "Math/Rotator.h"
#include "Math/Transform.h"
#include "UObject/NameTypes.h"

class UAnimSequence;
struct FPuppetPerformerCalibration;
struct FReferenceSkeleton;

/**
 * Plays an AnimSequence of human motion as a performer (ADR 0008, item 1): the bone names and
 * parents of the sequence's own skeleton, and local transforms at a time, the same data a Live
 * Link Animation-role subject gives. Bones only; the sequence's curves are not read.
 */
class OPEN3DPUPPET_API FPuppetClipSampler
{
public:
	/**
	 * Binds to Sequence, or to nothing when it is null. False, with OutError, when the sequence
	 * cannot play as a performer (no skeleton, or additive); the sampler is then unbound. The
	 * caller keeps Sequence alive while the sampler uses it. Game thread; allocates.
	 */
	bool SetSequence(const UAnimSequence* InSequence, FString& OutError);

	bool IsBound() const { return Sequence != nullptr; }
	const UAnimSequence* GetSequence() const { return Sequence; }
	TConstArrayView<FName> GetBoneNames() const { return Names; }
	TConstArrayView<int32> GetParents() const { return Parents; }
	/** The sequence's length in seconds; 0 when unbound. */
	double GetPlayLength() const { return PlayLength; }

	/**
	 * Fills OutLocal with one local transform per bone at Time (seconds, clamped to the clip).
	 * False when unbound. Any thread; allocates only when OutLocal must grow.
	 */
	bool Sample(double Time, TArray<FTransform>& OutLocal) const;

	/**
	 * Fills OutLocal with Target's reference pose, each bone the clip's skeleton also has (by name)
	 * turned to the clip's local rotation at Time; translations and scales stay Target's. For a
	 * puppet's rest pose authored on its skeleton. False when unbound. Allocates.
	 */
	bool SampleOnto(double Time, const FReferenceSkeleton& Target, TArray<FTransform>& OutLocal) const;

	/** The skeleton's reference pose, one local transform per bone. */
	TConstArrayView<FTransform> GetReferencePose() const { return ReferencePose; }

	/**
	 * A calibration of the clip (ADR 0008): its pose at Time, or its skeleton's reference pose when
	 * bReferencePose is set, with Alignment. False when unbound. Allocates.
	 */
	bool CaptureCalibration(bool bReferencePose, double Time, const FRotator& Alignment, FPuppetPerformerCalibration& OutCalibration) const;

	/**
	 * The playback time after DeltaTime at PlayRate (negative plays backwards), from Time. A looping
	 * clip wraps into [0, Length); otherwise it holds at the ends. A clip of length 0 stays at 0.
	 */
	static double AdvanceTime(double Time, double DeltaTime, float PlayRate, bool bLoop, double Length);

	/** Time placed in the clip as AdvanceTime places it: wrapped when looping, else clamped. */
	static double PlaceTime(double Time, bool bLoop, double Length);

private:
	const UAnimSequence* Sequence = nullptr;
	FBoneContainer BoneContainer;
	TArray<FName> Names;
	TArray<int32> Parents;
	TArray<FTransform> ReferencePose;
	double PlayLength = 0.0;
};
