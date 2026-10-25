// Copyright 2026 Lifelike & Believable. All Rights Reserved.

#include "PuppetRetargetMath.h"

#include "Algo/Reverse.h"

namespace PuppetRetarget
{
	FQuat RotationDelta(const FQuat& Current, const FQuat& Rest, const FQuat& Alignment)
	{
		return (Alignment * Current * Rest.Inverse() * Alignment.Inverse()).GetNormalized();
	}

	FQuat RotationDeltaInParent(const FQuat& Current, const FQuat& Rest, const FQuat& ParentCurrent, const FQuat& ParentRest, const FQuat& Alignment)
	{
		return (Alignment * ParentRest * ParentCurrent.Inverse() * Current * Rest.Inverse() * Alignment.Inverse()).GetNormalized();
	}

	FQuat ScaleRotation(const FQuat& Delta, double Amount)
	{
		// The shortest arc, so that scaling a small rotation never takes the long way round.
		const FQuat Shortest = Delta.W < 0.0 ? -Delta : Delta;
		FVector Axis;
		double Angle = 0.0;
		Shortest.ToAxisAndAngle(Axis, Angle);
		return FQuat(Axis, Angle * Amount);
	}

	void ChainPositions(TConstArrayView<FVector> JointPositions, TArray<float>& OutPositions)
	{
		const int32 NumJoints = JointPositions.Num();
		OutPositions.SetNum(NumJoints);
		if (NumJoints == 0)
		{
			return;
		}
		const double Length = ChainLength(JointPositions);
		double Distance = 0.0;
		for (int32 Joint = 0; Joint < NumJoints; ++Joint)
		{
			if (Joint > 0)
			{
				Distance += FVector::Dist(JointPositions[Joint - 1], JointPositions[Joint]);
			}
			if (Length > UE_KINDA_SMALL_NUMBER)
			{
				OutPositions[Joint] = static_cast<float>(Distance / Length);
			}
			else
			{
				OutPositions[Joint] = NumJoints > 1 ? static_cast<float>(Joint) / static_cast<float>(NumJoints - 1) : 0.0f;
			}
		}
	}

	double ChainLength(TConstArrayView<FVector> JointPositions)
	{
		double Length = 0.0;
		for (int32 Joint = 1; Joint < JointPositions.Num(); ++Joint)
		{
			Length += FVector::Dist(JointPositions[Joint - 1], JointPositions[Joint]);
		}
		return Length;
	}

	FQuat SampleChain(TConstArrayView<FQuat> Rotations, TConstArrayView<float> Positions, float T)
	{
		check(Rotations.Num() == Positions.Num() && Rotations.Num() > 0);
		const int32 Last = Rotations.Num() - 1;
		if (T <= Positions[0])
		{
			return Rotations[0];
		}
		for (int32 Joint = 0; Joint < Last; ++Joint)
		{
			const float From = Positions[Joint];
			const float To = Positions[Joint + 1];
			if (T <= To)
			{
				const float Alpha = To > From ? (T - From) / (To - From) : 1.0f;
				return FQuat::Slerp(Rotations[Joint], Rotations[Joint + 1], Alpha);
			}
		}
		return Rotations[Last];
	}

	bool CollectChain(TConstArrayView<int32> Parents, int32 Start, int32 End, TArray<int32>& OutChain)
	{
		OutChain.Reset();
		if (!Parents.IsValidIndex(Start) || !Parents.IsValidIndex(End))
		{
			return false;
		}
		for (int32 Bone = End; Bone != INDEX_NONE && OutChain.Num() <= Parents.Num(); Bone = Parents[Bone])
		{
			OutChain.Add(Bone);
			if (Bone == Start)
			{
				Algo::Reverse(OutChain);
				return true;
			}
		}
		OutChain.Reset();
		return false;
	}

	float RemapCurve(float Value, float InMin, float InMax, float DeadZone)
	{
		const float Position = InMax == InMin
			? (Value >= InMax ? 1.0f : 0.0f)
			: FMath::Clamp((Value - InMin) / (InMax - InMin), 0.0f, 1.0f);
		// Below 1, so the rest of the range never has zero width.
		const float Dead = FMath::Clamp(DeadZone, 0.0f, 0.95f);
		return Position <= Dead ? 0.0f : (Position - Dead) / (1.0f - Dead);
	}

	float SmoothTowards(float Current, float Target, float SmoothingTime, float DeltaTime)
	{
		if (SmoothingTime <= 0.0f)
		{
			return Target;
		}
		return Current + (Target - Current) * (1.0f - FMath::Exp(-DeltaTime / SmoothingTime));
	}
}
