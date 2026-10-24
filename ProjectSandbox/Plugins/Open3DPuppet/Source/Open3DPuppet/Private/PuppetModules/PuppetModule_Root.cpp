// Copyright 2026 Lifelike & Believable. All Rights Reserved.

#include "PuppetModules/PuppetModule_Root.h"

#include "PuppetPose.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(PuppetModule_Root)

namespace PuppetModuleRoot
{
	class FSolver : public TPuppetModuleSolver<UPuppetModule_Root>
	{
	public:
		using TPuppetModuleSolver<UPuppetModule_Root>::TPuppetModuleSolver;

		virtual bool Bind(const FPuppetBindContext& Context, FString& OutError) override
		{
			if (Context.PuppetPelvis == INDEX_NONE)
			{
				OutError = TEXT("the puppet has no pelvis bone (UPuppetAsset::PelvisBone)");
				return false;
			}
			if (Context.PerformerPelvis == INDEX_NONE)
			{
				OutError = TEXT("the performer has no 'pelvis' joint; map it in the performer profile");
				return false;
			}
			if (Settings.bDriveTranslation && !Context.bOwnsRoot)
			{
				OutError = TEXT("only a module on the root slot may drive translation (ADR 0004)");
				return false;
			}
			return true;
		}

		virtual void Solve(FPuppetSolveContext& Context) override
		{
			const int32 Pelvis = Context.PuppetPelvis;
			const int32 Joint = Context.PerformerPelvis;
			const FTransform& Reference = Context.Puppet.GetReferenceComponent(Pelvis);
			const FTransform Original = Context.Puppet.GetComponent(Pelvis);

			FTransform Target = Original;
			if (Settings.bDriveRotation)
			{
				Target.SetRotation((Context.GetJointDelta(Joint) * Reference.GetRotation()).GetNormalized());
			}
			if (Settings.bDriveTranslation)
			{
				const FVector Moved = Context.GetJointTranslation(Joint);
				const double Horizontal = Context.RootScale * Settings.HorizontalScale;
				const double Vertical = Context.RootScale * Settings.VerticalScale;
				Target.SetTranslation(Reference.GetTranslation() + FVector(Moved.X * Horizontal, Moved.Y * Horizontal, Moved.Z * Vertical + Settings.HeightOffset));
			}

			FTransform Result;
			Result.Blend(Original, Target, Settings.Weight);
			Context.Puppet.SetComponent(Pelvis, Result);
		}
	};
}

TUniquePtr<FPuppetModuleSolver> UPuppetModule_Root::CreateSolver() const
{
	return MakeUnique<PuppetModuleRoot::FSolver>(*this);
}
