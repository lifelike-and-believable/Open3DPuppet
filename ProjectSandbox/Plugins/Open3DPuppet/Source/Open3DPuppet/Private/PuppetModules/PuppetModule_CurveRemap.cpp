// Copyright 2026 Lifelike & Believable. All Rights Reserved.

#include "PuppetModules/PuppetModule_CurveRemap.h"

#include "PuppetPerformerPose.h"
#include "PuppetPose.h"
#include "PuppetRetargetMath.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(PuppetModule_CurveRemap)

namespace PuppetModuleCurveRemap
{
	class FSolver : public TPuppetModuleSolver<UPuppetModule_CurveRemap>
	{
	public:
		using TPuppetModuleSolver<UPuppetModule_CurveRemap>::TPuppetModuleSolver;

		virtual bool Bind(const FPuppetBindContext& Context, FString& OutError) override
		{
			if (Settings.Mappings.IsEmpty())
			{
				OutError = TEXT("it has no curve mappings");
				return false;
			}
			Curves.Reset();
			MappingOutputs.Reset();
			OutputNames.Reset();
			for (const FPuppetCurveMapping& Mapping : Settings.Mappings)
			{
				const int32 Curve = Context.Performer.FindCurve(Mapping.PerformerCurve);
				if (Curve == INDEX_NONE)
				{
					OutError = FString::Printf(TEXT("the performer has no curve '%s'"), *Mapping.PerformerCurve.ToString());
					return false;
				}
				Curves.Add(Curve);
				MappingOutputs.Add(OutputNames.AddUnique(Mapping.PuppetCurve));
			}
			Targets.SetNumZeroed(OutputNames.Num());
			Values.SetNumZeroed(OutputNames.Num());
			bHasValues = false;
			return true;
		}

		virtual void Solve(FPuppetSolveContext& Context) override
		{
			for (float& Target : Targets)
			{
				Target = 0.0f;
			}
			for (int32 Index = 0; Index < Curves.Num(); ++Index)
			{
				// The settings change size only when an edit re-binds the puppet; until then, skip what is gone.
				if (!Settings.Mappings.IsValidIndex(Index))
				{
					break;
				}
				const FPuppetCurveMapping& Mapping = Settings.Mappings[Index];
				const float Position = PuppetRetarget::RemapCurve(Context.GetCurveDelta(Curves[Index]), Mapping.InputMin, Mapping.InputMax, Mapping.DeadZone);
				Targets[MappingOutputs[Index]] += FMath::Lerp(Mapping.OutputMin, Mapping.OutputMax, Position);
			}
			for (int32 Output = 0; Output < OutputNames.Num(); ++Output)
			{
				// The first frame after a bind starts at the curves, not at 0.
				Values[Output] = bHasValues ? PuppetRetarget::SmoothTowards(Values[Output], Targets[Output], Settings.SmoothingTime, Context.DeltaTime) : Targets[Output];
				Context.Puppet.SetCurve(OutputNames[Output], Values[Output] * Context.Weight);
			}
			bHasValues = true;
		}

	private:
		/** Per mapping: the performer curve and the index of its puppet curve in OutputNames. */
		TArray<int32> Curves;
		TArray<int32> MappingOutputs;
		TArray<FName> OutputNames;
		TArray<float> Targets;
		/** Per puppet curve, after smoothing and before the weight. */
		TArray<float> Values;
		bool bHasValues = false;
	};
}

TUniquePtr<FPuppetModuleSolver> UPuppetModule_CurveRemap::CreateSolver() const
{
	return MakeUnique<PuppetModuleCurveRemap::FSolver>(*this);
}
