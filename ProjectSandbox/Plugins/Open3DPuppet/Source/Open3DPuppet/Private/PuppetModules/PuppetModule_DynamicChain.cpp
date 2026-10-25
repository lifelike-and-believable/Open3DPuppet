// Copyright 2026 Lifelike & Believable. All Rights Reserved.

#include "PuppetModules/PuppetModule_DynamicChain.h"

#include "PuppetPerformerCalibration.h"
#include "PuppetPerformerPose.h"
#include "PuppetPose.h"
#include "PuppetRetargetMath.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(PuppetModule_DynamicChain)

namespace PuppetModuleDynamicChain
{
	/** Constraint passes per step: enough for a few bones to keep their lengths. */
	constexpr int32 Iterations = 4;

	class FSolver : public TPuppetModuleSolver<UPuppetModule_DynamicChain>
	{
	public:
		using TPuppetModuleSolver<UPuppetModule_DynamicChain>::TPuppetModuleSolver;

		virtual bool Bind(const FPuppetBindContext& Context, FString& OutError) override
		{
			if (!Context.Puppet.GetChain(Context.Puppet.FindBone(Settings.PuppetStart), Context.Puppet.FindBone(Settings.PuppetEnd), PuppetChain) || PuppetChain.Num() < 2)
			{
				OutError = FString::Printf(TEXT("the puppet has no chain from '%s' to '%s'"), *Settings.PuppetStart.ToString(), *Settings.PuppetEnd.ToString());
				return false;
			}

			TArray<FVector> Positions;
			for (const int32 Bone : PuppetChain)
			{
				Positions.Add(Context.Puppet.GetReferenceComponent(Bone).GetTranslation());
			}
			PuppetRetarget::ChainPositions(Positions, PuppetPositions);

			PerformerChain.Reset();
			if (!Settings.PerformerStart.IsNone())
			{
				const int32 Start = Context.Performer.FindJoint(Settings.PerformerStart);
				const int32 End = Context.Performer.FindJoint(Settings.PerformerEnd);
				if (!PuppetRetarget::CollectChain(Context.Performer.GetParents(), Start, End, PerformerChain))
				{
					OutError = FString::Printf(TEXT("the performer has no chain from '%s' to '%s'"), *Settings.PerformerStart.ToString(), *Settings.PerformerEnd.ToString());
					return false;
				}
				TArray<FVector> PerformerJointPositions;
				for (const int32 Joint : PerformerChain)
				{
					PerformerJointPositions.Add(Context.Calibration.RestComponent[Joint].GetTranslation());
				}
				PuppetRetarget::ChainPositions(PerformerJointPositions, PerformerPositions);
				Deltas.SetNum(PerformerChain.Num());
			}

			CollisionBones.Reset();
			for (const FPuppetCollisionBone& Collision : Settings.CollisionBones)
			{
				const int32 Bone = Context.Puppet.FindBone(Collision.Bone);
				if (Bone == INDEX_NONE)
				{
					OutError = FString::Printf(TEXT("the puppet has no bone '%s' to collide with"), *Collision.Bone.ToString());
					return false;
				}
				CollisionBones.Add(Bone);
			}
			Shapes.Spheres.SetNum(CollisionBones.Num());

			if (!PuppetDynamics::BindPhysicsWeight(Settings.PhysicsWeight, Context.Performer, PhysicsWeightCurve, OutError))
			{
				return false;
			}

			// Sized here, so the resets in Solve do not allocate.
			Original.SetNum(PuppetChain.Num());
			Solved.SetNum(PuppetChain.Num());
			Follow.SetNum(PuppetChain.Num());
			Output.SetNum(PuppetChain.Num());
			Chain.Reset(Positions);
			Stepper.Reset();
			bStarted = false;
			return true;
		}

		virtual void Solve(FPuppetSolveContext& Context) override
		{
			FPuppetPose& Puppet = Context.Puppet;
			const int32 Num = PuppetChain.Num();
			for (int32 Index = 0; Index < Num; ++Index)
			{
				Original[Index] = Puppet.GetComponent(PuppetChain[Index]).GetRotation();
			}

			if (PerformerChain.Num() > 0 && Settings.InputMix > 0.0f)
			{
				for (int32 Index = 0; Index < PerformerChain.Num(); ++Index)
				{
					Deltas[Index] = Context.GetJointDelta(PerformerChain[Index]);
				}
				for (int32 Index = 0; Index < Num; ++Index)
				{
					const int32 Bone = PuppetChain[Index];
					const FQuat Target = PuppetRetarget::SampleChain(Deltas, PerformerPositions, PuppetPositions[Index]) * Puppet.GetReferenceComponent(Bone).GetRotation();
					Puppet.SetComponentRotation(Bone, FQuat::Slerp(Original[Index], Target.GetNormalized(), Settings.InputMix));
				}
			}
			for (int32 Index = 0; Index < Num; ++Index)
			{
				Follow[Index] = Puppet.GetComponent(PuppetChain[Index]).GetTranslation();
			}

			Simulate(Context);

			// The swing, scaled by Amplitude, then blended in by Physics Weight.
			const double Weight = PuppetDynamics::GetPhysicsWeight(Settings.PhysicsWeight, PhysicsWeightCurve, Context);
			for (int32 Index = 0; Index < Num; ++Index)
			{
				Output[Index] = Follow[Index] + (Output[Index] - Follow[Index]) * (Settings.Amplitude * Weight);
			}

			// Each bone turns to point at where its child should be; the last bone turns with its parent.
			for (int32 Index = 0; Index + 1 < Num; ++Index)
			{
				const FTransform& Bone = Puppet.GetComponent(PuppetChain[Index]);
				const FVector From = (Puppet.GetComponent(PuppetChain[Index + 1]).GetTranslation() - Bone.GetTranslation()).GetSafeNormal();
				const FVector To = (Output[Index + 1] - Bone.GetTranslation()).GetSafeNormal();
				if (!From.IsZero() && !To.IsZero())
				{
					Puppet.SetComponentRotation(PuppetChain[Index], (FQuat::FindBetweenNormals(From, To) * Bone.GetRotation()).GetNormalized());
				}
			}

			if (Context.Weight < 1.0f)
			{
				// Every solved rotation is read before any is blended: setting a bone moves its descendants.
				for (int32 Index = 0; Index < Num; ++Index)
				{
					Solved[Index] = Puppet.GetComponent(PuppetChain[Index]).GetRotation();
				}
				for (int32 Index = 0; Index < Num; ++Index)
				{
					Puppet.SetComponentRotation(PuppetChain[Index], FQuat::Slerp(Original[Index], Solved[Index], Context.Weight).GetNormalized());
				}
			}
		}

	private:
		/** Steps the chain to this frame and writes its positions, between the last two steps, to Output. */
		void Simulate(const FPuppetSolveContext& Context)
		{
			if (!bStarted || Context.bTeleport)
			{
				Chain.Reset(Follow);
				Stepper.Reset();
				bStarted = true;
			}
			else
			{
				// Keep the chain where it was in the world, so it lags when the puppet moves or turns.
				Chain.Transform(PuppetDynamics::ComponentMotion(Context.PreviousComponentToWorld, Context.ComponentToWorld));
			}

			for (int32 Index = 0; Index < CollisionBones.Num(); ++Index)
			{
				Shapes.Spheres[Index] = { Context.Puppet.GetComponent(CollisionBones[Index]).GetTranslation(), static_cast<double>(Settings.CollisionBones.IsValidIndex(Index) ? Settings.CollisionBones[Index].Radius : 0.0f) };
			}
			Shapes.bGround = Settings.bCollideWithGround;
			Shapes.GroundZ = Settings.GroundHeight;

			for (int32 Steps = Stepper.Advance(Context.DeltaTime); Steps > 0; --Steps)
			{
				Chain.Step(Follow, Settings.Gravity, Settings.Stiffness, Settings.Damping, Stepper.StepTime, Shapes, Settings.Radius, Iterations);
			}
			Chain.Interpolate(Stepper.GetAlpha(), Output);
		}

		TArray<int32> PuppetChain;
		TArray<float> PuppetPositions;
		TArray<int32> PerformerChain;
		TArray<float> PerformerPositions;
		TArray<FQuat> Deltas;
		TArray<int32> CollisionBones;
		int32 PhysicsWeightCurve = INDEX_NONE;

		TArray<FQuat> Original;
		TArray<FQuat> Solved;
		/** The pose the chain follows this frame, and the simulated positions. */
		TArray<FVector> Follow;
		TArray<FVector> Output;

		PuppetDynamics::FDynamicChain Chain;
		PuppetDynamics::FFixedStep Stepper;
		PuppetDynamics::FCollisionShapes Shapes;
		bool bStarted = false;
	};
}

TUniquePtr<FPuppetModuleSolver> UPuppetModule_DynamicChain::CreateSolver() const
{
	return MakeUnique<PuppetModuleDynamicChain::FSolver>(*this);
}
