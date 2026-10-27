// Copyright 2026 Lifelike & Believable. All Rights Reserved.

// UPuppetModule_PhysicsRegion and UPuppetPhysicsLibrary: a region's physics weight and extra
// damping as puppet curves, and those curves as Rigid Body With Control parameters (ADR 0007,
// Tier 2). Every curve at 0 means the physics as authored.

#include "PuppetTestHarness.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Harness/PuppetTestFixtures.h"
#include "PuppetModule_PhysicsRegion.h"
#include "PuppetPhysicsLibrary.h"
#include "PuppetPose.h"

namespace PuppetPhysicsRegionTests
{
	/** Calibrates the performer with Curve at 0, then solves one frame with it at Value. */
	void SolveWith(PuppetTests::FPuppet& Puppet, const TCHAR* Curve, float Value)
	{
		PuppetTests::FPerformer Performer;
		Performer.SetCurve(Curve, 0.0f);
		Puppet.Build();
		Puppet.Drive(0, Performer);
		Puppet.Calibrate(0, Performer);
		Performer.SetCurve(Curve, Value);
		Puppet.Drive(0, Performer);
		Puppet.Solve();
	}

	float Output(const PuppetTests::FPuppet& Puppet, FName Curve)
	{
		const float* Value = Puppet.Instance.GetPose().FindCurve(Curve);
		return Value ? *Value : -1.0f;
	}

	FPhysicsControlControlAndModifierParameters Make(TConstArrayView<FName> Regions, const TMap<FName, float>& Curves)
	{
		return UPuppetPhysicsLibrary::MakeRegionParametersFromCurves(Regions, [&Curves](FName Curve)
		{
			const float* Value = Curves.Find(Curve);
			return Value ? *Value : 0.0f;
		});
	}

	/** Checks one region's multipliers: strength on both drives, damping ratio on both. */
	void TestMultiplier(FAutomationTestBase& Test, const FPhysicsControlControlAndModifierParameters& Parameters, int32 Index, FName Region, float Strength, float Damping)
	{
		if (!Parameters.ControlMultiplierParameters.IsValidIndex(Index))
		{
			Test.AddError(FString::Printf(TEXT("No multiplier %d"), Index));
			return;
		}
		const FPhysicsControlNamedControlMultiplierParameters& Entry = Parameters.ControlMultiplierParameters[Index];
		const FPhysicsControlSparseMultiplier& Data = Entry.Data;
		Test.TestEqual(TEXT("Set"), Entry.Name, Region);
		Test.TestTrue(TEXT("Strength flags"), Data.bEnableAngularStrengthMultiplier && Data.bEnableLinearStrengthMultiplier);
		Test.TestTrue(TEXT("Damping flags"), Data.bEnableAngularDampingRatioMultiplier && Data.bEnableLinearDampingRatioMultiplier);
		Test.TestNearlyEqual(TEXT("Angular strength"), Data.AngularStrengthMultiplier, Strength, 1.e-5f);
		Test.TestTrue(TEXT("Linear strength"), Data.LinearStrengthMultiplier.Equals(FVector(Strength), 1.e-5));
		Test.TestNearlyEqual(TEXT("Angular damping"), Data.AngularDampingRatioMultiplier, Damping, 1.e-5f);
		Test.TestTrue(TEXT("Linear damping"), Data.LinearDampingRatioMultiplier.Equals(FVector(Damping), 1.e-5));
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetPhysicsRegionCurveNamesTest, "Open3DPuppet.Modules.PhysicsRegion.CurvesAreNamedForTheRegion", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetPhysicsRegionCurveNamesTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("Weight"), PuppetPhysics::WeightCurve(TEXT("Tail")), FName(TEXT("Physics_Tail")));
	TestEqual(TEXT("Damping"), PuppetPhysics::DampingCurve(TEXT("Tail")), FName(TEXT("Physics_Tail_Damping")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetPhysicsRegionPublishTest, "Open3DPuppet.Modules.PhysicsRegion.PublishesWeightAndDamping", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetPhysicsRegionPublishTest::RunTest(const FString& Parameters)
{
	PuppetTests::FPuppet Puppet(PuppetTests::Tail());
	UPuppetModule_PhysicsRegion& Region = Puppet.Add<UPuppetModule_PhysicsRegion>();
	Region.PhysicsWeight.Weight = 0.6f;
	Region.Damping.Weight = 0.25f;
	PuppetPhysicsRegionTests::SolveWith(Puppet, TEXT("jawOpen"), 0.0f);
	TestNearlyEqual(TEXT("Weight"), PuppetPhysicsRegionTests::Output(Puppet, TEXT("Physics_Tail")), 0.6f, 1.e-5f);
	TestNearlyEqual(TEXT("Damping"), PuppetPhysicsRegionTests::Output(Puppet, TEXT("Physics_Tail_Damping")), 0.25f, 1.e-5f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetPhysicsRegionDefaultTest, "Open3DPuppet.Modules.PhysicsRegion.DefaultsLeaveThePhysicsAsAuthored", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetPhysicsRegionDefaultTest::RunTest(const FString& Parameters)
{
	PuppetTests::FPuppet Puppet(PuppetTests::Tail());
	Puppet.Add<UPuppetModule_PhysicsRegion>();
	PuppetPhysicsRegionTests::SolveWith(Puppet, TEXT("jawOpen"), 0.0f);
	TestEqual(TEXT("Weight"), PuppetPhysicsRegionTests::Output(Puppet, TEXT("Physics_Tail")), 0.0f);
	TestEqual(TEXT("Damping"), PuppetPhysicsRegionTests::Output(Puppet, TEXT("Physics_Tail_Damping")), 0.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetPhysicsRegionCurveTest, "Open3DPuppet.Modules.PhysicsRegion.PerformerCurvePlaysTheWeight", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetPhysicsRegionCurveTest::RunTest(const FString& Parameters)
{
	// The curve's change from neutral sets the weight in place of the fixed value.
	PuppetTests::FPuppet Puppet(PuppetTests::Tail());
	UPuppetModule_PhysicsRegion& Region = Puppet.Add<UPuppetModule_PhysicsRegion>();
	Region.PhysicsWeight.Weight = 0.9f;
	Region.PhysicsWeight.Curve = TEXT("jawOpen");
	PuppetPhysicsRegionTests::SolveWith(Puppet, TEXT("jawOpen"), 0.25f);
	TestNearlyEqual(TEXT("Weight"), PuppetPhysicsRegionTests::Output(Puppet, TEXT("Physics_Tail")), 0.25f, 1.e-5f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetPhysicsRegionWeightTest, "Open3DPuppet.Modules.PhysicsRegion.ModuleWeightScalesTowardsAuthored", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetPhysicsRegionWeightTest::RunTest(const FString& Parameters)
{
	PuppetTests::FPuppet Puppet(PuppetTests::Tail());
	UPuppetModule_PhysicsRegion& Region = Puppet.Add<UPuppetModule_PhysicsRegion>();
	Region.Weight = 0.5f;
	Region.PhysicsWeight.Weight = 0.6f;
	Region.Damping.Weight = 1.0f;
	PuppetPhysicsRegionTests::SolveWith(Puppet, TEXT("jawOpen"), 0.0f);
	TestNearlyEqual(TEXT("Weight"), PuppetPhysicsRegionTests::Output(Puppet, TEXT("Physics_Tail")), 0.3f, 1.e-5f);
	TestNearlyEqual(TEXT("Damping"), PuppetPhysicsRegionTests::Output(Puppet, TEXT("Physics_Tail_Damping")), 0.5f, 1.e-5f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetPhysicsRegionBonesTest, "Open3DPuppet.Modules.PhysicsRegion.WritesNoBones", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetPhysicsRegionBonesTest::RunTest(const FString& Parameters)
{
	PuppetTests::FPuppet Puppet(PuppetTests::Tail());
	UPuppetModule_PhysicsRegion& Region = Puppet.Add<UPuppetModule_PhysicsRegion>();
	Region.PhysicsWeight.Weight = 1.0f;
	PuppetPhysicsRegionTests::SolveWith(Puppet, TEXT("jawOpen"), 0.0f);
	const TArray<FTransform> Input = Puppet.InputWith(TEXT("tail_02"), FTransform(PuppetTests::Rotation(FVector::ZAxisVector, 40.0), FVector(3.0, 0.0, 0.0)));
	Puppet.Solve(Input);
	const TConstArrayView<FTransform> Output = Puppet.Instance.GetPose().GetLocalTransforms();
	if (!TestEqual(TEXT("Bones"), Output.Num(), Input.Num()))
	{
		return true;
	}
	for (int32 Bone = 0; Bone < Input.Num(); ++Bone)
	{
		TestTrue(*FString::Printf(TEXT("Bone %d"), Bone), Output[Bone].Equals(Input[Bone], 1.e-6));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetPhysicsRegionNoRegionTest, "Open3DPuppet.Modules.PhysicsRegion.NoRegionSkipsTheModule", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetPhysicsRegionNoRegionTest::RunTest(const FString& Parameters)
{
	AddExpectedMessagePlain(TEXT("it names no region"), ELogVerbosity::Warning);
	PuppetTests::FPuppet Puppet(PuppetTests::Tail());
	UPuppetModule_PhysicsRegion& Region = Puppet.Add<UPuppetModule_PhysicsRegion>();
	Region.Region = NAME_None;
	PuppetPhysicsRegionTests::SolveWith(Puppet, TEXT("jawOpen"), 0.0f);
	TestTrue(TEXT("No curve written"), Puppet.Instance.GetPose().FindCurve(TEXT("Physics_None")) == nullptr);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetPhysicsRegionMissingCurveTest, "Open3DPuppet.Modules.PhysicsRegion.MissingCurveSkipsTheModule", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetPhysicsRegionMissingCurveTest::RunTest(const FString& Parameters)
{
	AddExpectedMessagePlain(TEXT("the performer has no curve 'tailLimp' for the physics weight"), ELogVerbosity::Warning);
	PuppetTests::FPuppet Puppet(PuppetTests::Tail());
	UPuppetModule_PhysicsRegion& Region = Puppet.Add<UPuppetModule_PhysicsRegion>();
	Region.Damping.Curve = TEXT("tailLimp");
	PuppetPhysicsRegionTests::SolveWith(Puppet, TEXT("jawOpen"), 0.0f);
	TestTrue(TEXT("No curve written"), Puppet.Instance.GetPose().FindCurve(TEXT("Physics_Tail")) == nullptr);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetPhysicsLibraryNeutralTest, "Open3DPuppet.Modules.PhysicsRegion.LibraryKeepsAuthoredWithoutCurves", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetPhysicsLibraryNeutralTest::RunTest(const FString& Parameters)
{
	// No curves (a module at weight 0, or blended away) and curves at 0 both mean the authored physics.
	const FName Regions[] = { TEXT("Tail"), TEXT("FrontLegs") };
	const FPhysicsControlControlAndModifierParameters Result = PuppetPhysicsRegionTests::Make(Regions, { { TEXT("Physics_FrontLegs"), 0.0f } });
	TestEqual(TEXT("One multiplier per region"), Result.ControlMultiplierParameters.Num(), 2);
	TestEqual(TEXT("No control parameters"), Result.ControlParameters.Num(), 0);
	TestEqual(TEXT("No modifier parameters"), Result.ModifierParameters.Num(), 0);
	PuppetPhysicsRegionTests::TestMultiplier(*this, Result, 0, TEXT("Tail"), 1.0f, 1.0f);
	PuppetPhysicsRegionTests::TestMultiplier(*this, Result, 1, TEXT("FrontLegs"), 1.0f, 1.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetPhysicsLibraryCurvesTest, "Open3DPuppet.Modules.PhysicsRegion.LibraryTurnsCurvesIntoMultipliers", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetPhysicsLibraryCurvesTest::RunTest(const FString& Parameters)
{
	const FName Regions[] = { TEXT("Tail") };
	const FPhysicsControlControlAndModifierParameters Result = PuppetPhysicsRegionTests::Make(Regions, { { TEXT("Physics_Tail"), 0.75f }, { TEXT("Physics_Tail_Damping"), 0.5f } });
	PuppetPhysicsRegionTests::TestMultiplier(*this, Result, 0, TEXT("Tail"), 0.25f, 1.5f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetPhysicsLibraryClampTest, "Open3DPuppet.Modules.PhysicsRegion.LibraryClampsTheCurves", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetPhysicsLibraryClampTest::RunTest(const FString& Parameters)
{
	// A weight past 1 stays limp, not negative; damping below 0 stays as authored.
	const FName Regions[] = { TEXT("Tail") };
	const FPhysicsControlControlAndModifierParameters Result = PuppetPhysicsRegionTests::Make(Regions, { { TEXT("Physics_Tail"), 1.5f }, { TEXT("Physics_Tail_Damping"), -0.5f } });
	PuppetPhysicsRegionTests::TestMultiplier(*this, Result, 0, TEXT("Tail"), 0.0f, 1.0f);
	return true;
}

#endif
