// Copyright 2026 Lifelike & Believable. All Rights Reserved.

#include "PuppetModules/PuppetModule_Eyes.h"

#include "PuppetBoneOffset.h"
#include "PuppetPerformerPose.h"
#include "PuppetPose.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(PuppetModule_Eyes)

namespace PuppetModuleEyes
{
	/** ARKit's eye-look curves, in the order the solver reads them. */
	enum EGazeCurve : int32 { InLeft, OutLeft, UpLeft, DownLeft, InRight, OutRight, UpRight, DownRight, NumGazeCurves };

	const TCHAR* const GazeCurveNames[NumGazeCurves] = {
		TEXT("eyeLookInLeft"), TEXT("eyeLookOutLeft"), TEXT("eyeLookUpLeft"), TEXT("eyeLookDownLeft"),
		TEXT("eyeLookInRight"), TEXT("eyeLookOutRight"), TEXT("eyeLookUpRight"), TEXT("eyeLookDownRight"),
	};

	class FSolver : public TPuppetModuleSolver<UPuppetModule_Eyes>
	{
	public:
		using TPuppetModuleSolver<UPuppetModule_Eyes>::TPuppetModuleSolver;

		virtual bool Bind(const FPuppetBindContext& Context, FString& OutError) override
		{
			const FName PuppetEyes[2] = { Settings.PuppetLeftEye, Settings.PuppetRightEye };
			for (int32 Eye = 0; Eye < 2; ++Eye)
			{
				Bones[Eye] = Context.Puppet.FindBone(PuppetEyes[Eye]);
				if (Bones[Eye] == INDEX_NONE)
				{
					OutError = FString::Printf(TEXT("the puppet has no bone '%s'"), *PuppetEyes[Eye].ToString());
					return false;
				}
				References[Eye] = PuppetBoneOffset::ReferenceLocal(Context.Puppet, Bones[Eye]);
			}
			YawAxis = Settings.YawAxis.GetSafeNormal();
			PitchAxis = Settings.PitchAxis.GetSafeNormal();
			if (YawAxis.IsZero() || PitchAxis.IsZero())
			{
				OutError = TEXT("the yaw or pitch axis is zero");
				return false;
			}

			Input = Settings.Input;
			if (Input == EPuppetEyeInput::Curves)
			{
				for (int32 Curve = 0; Curve < NumGazeCurves; ++Curve)
				{
					Curves[Curve] = Context.Performer.FindCurve(GazeCurveNames[Curve]);
					if (Curves[Curve] == INDEX_NONE)
					{
						OutError = FString::Printf(TEXT("the performer has no curve '%s'"), GazeCurveNames[Curve]);
						return false;
					}
				}
			}
			else
			{
				const FName PerformerEyes[2] = { Settings.PerformerLeftEye, Settings.PerformerRightEye };
				for (int32 Eye = 0; Eye < 2; ++Eye)
				{
					Joints[Eye] = Context.Performer.FindJoint(PerformerEyes[Eye]);
					if (Joints[Eye] == INDEX_NONE)
					{
						OutError = FString::Printf(TEXT("the performer has no joint '%s'"), *PerformerEyes[Eye].ToString());
						return false;
					}
				}
			}
			return true;
		}

		virtual void Solve(FPuppetSolveContext& Context) override
		{
			// Degrees per eye, left then right: positive yaw looks to the left, positive pitch up.
			double Yaw[2];
			double Pitch[2];
			if (Input == EPuppetEyeInput::Curves)
			{
				// Looking left is the left eye looking out and the right eye looking in.
				Yaw[0] = Look(Context, OutLeft, InLeft) * Settings.MaxYaw;
				Yaw[1] = Look(Context, InRight, OutRight) * Settings.MaxYaw;
				Pitch[0] = Look(Context, UpLeft, DownLeft) * Settings.MaxPitch;
				Pitch[1] = Look(Context, UpRight, DownRight) * Settings.MaxPitch;
			}
			else
			{
				const double MaxYaw = Settings.MaxYaw;
				const double MaxPitch = Settings.MaxPitch;
				for (int32 Eye = 0; Eye < 2; ++Eye)
				{
					// Where the performer's eye now looks, from where it looked at calibration, relative to the head.
					const FVector Gaze = Context.GetJointDeltaInParent(Joints[Eye]).RotateVector(FVector::XAxisVector);
					Yaw[Eye] = FMath::Clamp(FMath::RadiansToDegrees(FMath::Atan2(Gaze.Y, Gaze.X)), -MaxYaw, MaxYaw);
					Pitch[Eye] = FMath::Clamp(FMath::RadiansToDegrees(FMath::Atan2(Gaze.Z, Gaze.Size2D())), -MaxPitch, MaxPitch);
				}
			}

			const double SharedYaw = 0.5 * (Yaw[0] + Yaw[1]);
			const double SharedPitch = 0.5 * (Pitch[0] + Pitch[1]);
			for (int32 Eye = 0; Eye < 2; ++Eye)
			{
				const double EyeYaw = FMath::Lerp(Yaw[Eye], SharedYaw, static_cast<double>(Settings.Convergence));
				const double EyePitch = FMath::Lerp(Pitch[Eye], SharedPitch, static_cast<double>(Settings.Convergence));
				const FQuat Turn = FQuat(YawAxis, FMath::DegreesToRadians(EyeYaw)) * FQuat(PitchAxis, FMath::DegreesToRadians(EyePitch));
				PuppetBoneOffset::Apply(Context.Puppet, Bones[Eye], References[Eye], Turn, FVector::ZeroVector, Context.Weight);
			}
		}

	private:
		/** One direction's curve minus the opposite one's, from neutral, within -1 to 1. */
		double Look(const FPuppetSolveContext& Context, EGazeCurve Towards, EGazeCurve Away) const
		{
			return FMath::Clamp(static_cast<double>(Context.GetCurveDelta(Curves[Towards]) - Context.GetCurveDelta(Curves[Away])), -1.0, 1.0);
		}

		EPuppetEyeInput Input = EPuppetEyeInput::Curves;
		int32 Bones[2] = { INDEX_NONE, INDEX_NONE };
		FTransform References[2];
		int32 Joints[2] = { INDEX_NONE, INDEX_NONE };
		int32 Curves[NumGazeCurves] = {};
		FVector YawAxis = FVector::UpVector;
		FVector PitchAxis = FVector(0.0, -1.0, 0.0);
	};
}

TUniquePtr<FPuppetModuleSolver> UPuppetModule_Eyes::CreateSolver() const
{
	return MakeUnique<PuppetModuleEyes::FSolver>(*this);
}
