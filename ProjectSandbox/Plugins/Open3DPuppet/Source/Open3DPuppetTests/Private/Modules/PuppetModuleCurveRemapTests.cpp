// Copyright 2026 Lifelike & Believable. All Rights Reserved.

// UPuppetModule_CurveRemap: performer curves onto puppet curves (morph targets), with ranges,
// combination and smoothing (ADR 0006).

#include "PuppetTestHarness.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Harness/PuppetTestFixtures.h"
#include "PuppetModules/PuppetModule_CurveRemap.h"
#include "PuppetPose.h"

namespace PuppetCurveRemapTests
{
	FPuppetCurveMapping& AddMapping(UPuppetModule_CurveRemap& Remap, const TCHAR* PerformerCurve, const TCHAR* PuppetCurve)
	{
		FPuppetCurveMapping& Mapping = Remap.Mappings.AddDefaulted_GetRef();
		Mapping.PerformerCurve = PerformerCurve;
		Mapping.PuppetCurve = PuppetCurve;
		return Mapping;
	}

	struct FCurveValue
	{
		const TCHAR* Curve;
		float Value;
	};

	/** Calibrates the face with every curve at 0, then solves one frame with the curves at Values. */
	void SolveWith(PuppetTests::FPuppet& Puppet, PuppetTests::FPerformer& Face, const TArray<FCurveValue>& Values)
	{
		for (const FCurveValue& Value : Values)
		{
			Face.SetCurve(Value.Curve, 0.0f);
		}
		Puppet.Build();
		Puppet.Drive(0, Face);
		Puppet.Calibrate(0, Face);
		for (const FCurveValue& Value : Values)
		{
			Face.SetCurve(Value.Curve, Value.Value);
		}
		Puppet.Drive(0, Face);
		Puppet.Solve();
	}

	float Output(const PuppetTests::FPuppet& Puppet, const TCHAR* Curve)
	{
		const float* Value = Puppet.Instance.GetPose().FindCurve(Curve);
		return Value ? *Value : -1.0f;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetCurveRemapRangeTest, "Open3DPuppet.Modules.CurveRemap.MapsInputRangeToOutputRange", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetCurveRemapRangeTest::RunTest(const FString& Parameters)
{
	// jawOpen 0 to 0.5 onto Jaw 0 to 2: 0.25 is halfway, so 1.
	PuppetTests::FPuppet Puppet(PuppetTests::Face());
	UPuppetModule_CurveRemap& Remap = Puppet.Add<UPuppetModule_CurveRemap>();
	FPuppetCurveMapping& Jaw = PuppetCurveRemapTests::AddMapping(Remap, TEXT("jawOpen"), TEXT("Jaw"));
	Jaw.InputMax = 0.5f;
	Jaw.OutputMax = 2.0f;
	PuppetTests::FPerformer Face;
	PuppetCurveRemapTests::SolveWith(Puppet, Face, { { TEXT("jawOpen"), 0.25f } });
	TestNearlyEqual(TEXT("Jaw"), PuppetCurveRemapTests::Output(Puppet, TEXT("Jaw")), 1.0f, 1.e-5f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetCurveRemapCombineTest, "Open3DPuppet.Modules.CurveRemap.MappingsToOneCurveAddUp", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetCurveRemapCombineTest::RunTest(const FString& Parameters)
{
	// A wolf's snarl from both a nose sneer and a raised upper lip.
	PuppetTests::FPuppet Puppet(PuppetTests::Face());
	UPuppetModule_CurveRemap& Remap = Puppet.Add<UPuppetModule_CurveRemap>();
	PuppetCurveRemapTests::AddMapping(Remap, TEXT("noseSneerLeft"), TEXT("Snarl"));
	PuppetCurveRemapTests::AddMapping(Remap, TEXT("mouthUpperUpLeft"), TEXT("Snarl"));
	PuppetTests::FPerformer Face;
	PuppetCurveRemapTests::SolveWith(Puppet, Face, { { TEXT("noseSneerLeft"), 0.25f }, { TEXT("mouthUpperUpLeft"), 0.5f } });
	TestNearlyEqual(TEXT("Snarl"), PuppetCurveRemapTests::Output(Puppet, TEXT("Snarl")), 0.75f, 1.e-5f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetCurveRemapSmoothingTest, "Open3DPuppet.Modules.CurveRemap.SmoothingFollowsOverTime", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetCurveRemapSmoothingTest::RunTest(const FString& Parameters)
{
	// A time constant of one frame (1/30 s): a jump from 0 to 1 covers 1 - 1/e of the way in a frame.
	PuppetTests::FPuppet Puppet(PuppetTests::Face());
	UPuppetModule_CurveRemap& Remap = Puppet.Add<UPuppetModule_CurveRemap>();
	Remap.SmoothingTime = 1.0f / 30.0f;
	PuppetCurveRemapTests::AddMapping(Remap, TEXT("jawOpen"), TEXT("Jaw"));
	PuppetTests::FPerformer Face;
	PuppetCurveRemapTests::SolveWith(Puppet, Face, { { TEXT("jawOpen"), 0.0f } });
	Face.SetCurve(TEXT("jawOpen"), 1.0f);
	Puppet.Drive(0, Face);
	Puppet.Solve();
	TestNearlyEqual(TEXT("Jaw"), PuppetCurveRemapTests::Output(Puppet, TEXT("Jaw")), 1.0f - FMath::Exp(-1.0f), 1.e-5f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetCurveRemapWeightTest, "Open3DPuppet.Modules.CurveRemap.WeightScalesTheCurves", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetCurveRemapWeightTest::RunTest(const FString& Parameters)
{
	PuppetTests::FPuppet Puppet(PuppetTests::Face());
	UPuppetModule_CurveRemap& Remap = Puppet.Add<UPuppetModule_CurveRemap>();
	Remap.Weight = 0.5f;
	PuppetCurveRemapTests::AddMapping(Remap, TEXT("jawOpen"), TEXT("Jaw"));
	PuppetTests::FPerformer Face;
	PuppetCurveRemapTests::SolveWith(Puppet, Face, { { TEXT("jawOpen"), 0.8f } });
	TestNearlyEqual(TEXT("Jaw"), PuppetCurveRemapTests::Output(Puppet, TEXT("Jaw")), 0.4f, 1.e-5f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetCurveRemapMissingTest, "Open3DPuppet.Modules.CurveRemap.MissingCurveSkipsTheModule", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetCurveRemapMissingTest::RunTest(const FString& Parameters)
{
	AddExpectedMessagePlain(TEXT("the performer has no curve 'beakOpen'"), ELogVerbosity::Warning);
	PuppetTests::FPuppet Puppet(PuppetTests::Face());
	UPuppetModule_CurveRemap& Remap = Puppet.Add<UPuppetModule_CurveRemap>();
	PuppetCurveRemapTests::AddMapping(Remap, TEXT("jawOpen"), TEXT("Jaw"));
	PuppetCurveRemapTests::AddMapping(Remap, TEXT("beakOpen"), TEXT("Beak"));
	PuppetTests::FPerformer Face;
	PuppetCurveRemapTests::SolveWith(Puppet, Face, { { TEXT("jawOpen"), 0.5f } });
	TestTrue(TEXT("No curve written"), Puppet.Instance.GetPose().FindCurve(TEXT("Jaw")) == nullptr);
	return true;
}

#endif
