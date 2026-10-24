// Copyright 2026 Lifelike & Believable. All Rights Reserved.

// UPuppetModule_Digits: fingers onto digits with parent-relative deltas, curl and spread gains and
// chain resampling (ADR 0006). In the Hand fixture fingers point along +Y with the palm down, so a
// curl is a turn about X and a spread a turn about Z, the module's default Spread Axis.

#include "PuppetTestHarness.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Harness/PuppetTestFixtures.h"
#include "PuppetModules/PuppetModule_Digits.h"

namespace PuppetDigitsTests
{
	/** A curl of 30 degrees combined with a spread of 10: Swing * Twist about Z. */
	FQuat CurlAndSpread()
	{
		return PuppetTests::Rotation(FVector::XAxisVector, 30.0) * PuppetTests::Rotation(FVector::ZAxisVector, 10.0);
	}

	/** Calibrates slot 0 at rest, then drives it with index_01_l at Index's local rotation. */
	void SolveWithIndex(PuppetTests::FPuppet& Puppet, const FQuat& Index)
	{
		PuppetTests::FPerformer Performer(PuppetTests::Hand());
		Puppet.Build();
		Puppet.Drive(0, Performer);
		Puppet.Calibrate(0, Performer);
		Performer.SetLocalRotation(TEXT("index_01_l"), Index);
		Puppet.Drive(0, Performer);
		Puppet.Solve();
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetDigitsRestTest, "Open3DPuppet.Modules.Digits.RestMapsToReference", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetDigitsRestTest::RunTest(const FString& Parameters)
{
	PuppetTests::FPuppet Puppet(PuppetTests::Hand());
	Puppet.Add<UPuppetModule_Digits>();
	PuppetTests::FPerformer Performer(PuppetTests::Hand());
	Puppet.Build();
	Puppet.Drive(0, Performer);
	Puppet.Calibrate(0, Performer);
	Puppet.Solve();
	TestEqual(TEXT("index_03_l at its reference"), Puppet.Component(TEXT("index_03_l")), Puppet.Reference(TEXT("index_03_l")), 1.e-3f);
	TestEqual(TEXT("thumb_03_l at its reference"), Puppet.Component(TEXT("thumb_03_l")), Puppet.Reference(TEXT("thumb_03_l")), 1.e-3f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetDigitsRelativeTest, "Open3DPuppet.Modules.Digits.CurlIsRelativeToTheHand", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetDigitsRelativeTest::RunTest(const FString& Parameters)
{
	// The performer's wrist turns 90 degrees about the forearm (Y) and index_01_l curls 30 degrees
	// in the hand. No module moves the puppet's hand, so its index takes the curl alone: 30 about X,
	// not the 90-about-Y-then-30 a component-space delta would give.
	PuppetTests::FPuppet Puppet(PuppetTests::Hand());
	Puppet.Add<UPuppetModule_Digits>();
	PuppetTests::FPerformer Performer(PuppetTests::Hand());
	Puppet.Build();
	Puppet.Drive(0, Performer);
	Puppet.Calibrate(0, Performer);
	Performer.SetLocalRotation(TEXT("hand_l"), PuppetTests::Rotation(FVector::YAxisVector, 90.0));
	const FQuat Curl = PuppetTests::Rotation(FVector::XAxisVector, 30.0);
	Performer.SetLocalRotation(TEXT("index_01_l"), Curl);
	Puppet.Drive(0, Performer);
	Puppet.Solve();

	PuppetTests::TestRotation(*this, TEXT("index_01_l"), Puppet.Component(TEXT("index_01_l")).GetRotation(), Curl);
	PuppetTests::TestRotation(*this, TEXT("index_03_l"), Puppet.Component(TEXT("index_03_l")).GetRotation(), Curl);
	PuppetTests::TestRotation(*this, TEXT("middle_01_l, not curled"), Puppet.Component(TEXT("middle_01_l")).GetRotation(), FQuat::Identity);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetDigitsCurlGainTest, "Open3DPuppet.Modules.Digits.CurlGainScalesCurl", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetDigitsCurlGainTest::RunTest(const FString& Parameters)
{
	// Twice the curl, the spread unchanged: 60 about X, then 10 about Z.
	PuppetTests::FPuppet Puppet(PuppetTests::Hand());
	Puppet.Add<UPuppetModule_Digits>().Digits[1].CurlGain = 2.0f;
	PuppetDigitsTests::SolveWithIndex(Puppet, PuppetDigitsTests::CurlAndSpread());
	PuppetTests::TestRotation(*this, TEXT("index_01_l"), Puppet.Component(TEXT("index_01_l")).GetRotation(),
		PuppetTests::Rotation(FVector::XAxisVector, 60.0) * PuppetTests::Rotation(FVector::ZAxisVector, 10.0));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetDigitsSpreadGainTest, "Open3DPuppet.Modules.Digits.SpreadGainScalesSpread", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetDigitsSpreadGainTest::RunTest(const FString& Parameters)
{
	// No curl and twice the spread: 20 about Z.
	PuppetTests::FPuppet Puppet(PuppetTests::Hand());
	FPuppetDigit& Index = Puppet.Add<UPuppetModule_Digits>().Digits[1];
	Index.CurlGain = 0.0f;
	Index.SpreadGain = 2.0f;
	PuppetDigitsTests::SolveWithIndex(Puppet, PuppetDigitsTests::CurlAndSpread());
	PuppetTests::TestRotation(*this, TEXT("index_01_l"), Puppet.Component(TEXT("index_01_l")).GetRotation(), PuppetTests::Rotation(FVector::ZAxisVector, 20.0));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetDigitsSpreadAxisTest, "Open3DPuppet.Modules.Digits.SpreadAxisSeparatesSpreadFromCurl", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetDigitsSpreadAxisTest::RunTest(const FString& Parameters)
{
	// With X as the spread axis, the 10 degrees about Z of (10 about Z) * (30 about X) are curl and
	// the 30 about X are spread; with no spread the digit keeps 10 about Z.
	PuppetTests::FPuppet Puppet(PuppetTests::Hand());
	UPuppetModule_Digits& Digits = Puppet.Add<UPuppetModule_Digits>();
	Digits.SpreadAxis = FVector::XAxisVector;
	Digits.Digits[1].SpreadGain = 0.0f;
	PuppetDigitsTests::SolveWithIndex(Puppet, PuppetTests::Rotation(FVector::ZAxisVector, 10.0) * PuppetTests::Rotation(FVector::XAxisVector, 30.0));
	PuppetTests::TestRotation(*this, TEXT("index_01_l"), Puppet.Component(TEXT("index_01_l")).GetRotation(), PuppetTests::Rotation(FVector::ZAxisVector, 10.0));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetDigitsResampleTest, "Open3DPuppet.Modules.Digits.ResamplesOntoALongerDigit", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetDigitsResampleTest::RunTest(const FString& Parameters)
{
	// Each of the index's three joints curls 30 degrees, so they sit at 30, 60 and 90 about X at
	// positions 0, 0.5 and 1 along the finger. The paw's five-bone digit, at 0, 0.25, ..., 1, takes
	// 30, 45, 60, 75 and 90: the finger's whole curl, spread along more bones.
	PuppetTests::FPuppet Puppet(PuppetTests::Paw());
	UPuppetModule_Digits& Digits = Puppet.Add<UPuppetModule_Digits>();
	Digits.Digits.Reset();
	FPuppetDigit& Digit = Digits.Digits.AddDefaulted_GetRef();
	Digit.PerformerStart = TEXT("index_01_l");
	Digit.PerformerEnd = TEXT("index_03_l");
	Digit.PuppetStart = TEXT("digit_01");
	Digit.PuppetEnd = TEXT("digit_05");

	PuppetTests::FPerformer Performer(PuppetTests::Hand());
	Puppet.Build();
	Puppet.Drive(0, Performer);
	Puppet.Calibrate(0, Performer);
	const FQuat Curl = PuppetTests::Rotation(FVector::XAxisVector, 30.0);
	Performer.SetLocalRotation(TEXT("index_01_l"), Curl);
	Performer.SetLocalRotation(TEXT("index_02_l"), Curl);
	Performer.SetLocalRotation(TEXT("index_03_l"), Curl);
	Puppet.Drive(0, Performer);
	Puppet.Solve();

	PuppetTests::TestRotation(*this, TEXT("digit_01"), Puppet.Component(TEXT("digit_01")).GetRotation(), PuppetTests::Rotation(FVector::XAxisVector, 30.0));
	PuppetTests::TestRotation(*this, TEXT("digit_02"), Puppet.Component(TEXT("digit_02")).GetRotation(), PuppetTests::Rotation(FVector::XAxisVector, 45.0));
	PuppetTests::TestRotation(*this, TEXT("digit_03"), Puppet.Component(TEXT("digit_03")).GetRotation(), PuppetTests::Rotation(FVector::XAxisVector, 60.0));
	PuppetTests::TestRotation(*this, TEXT("digit_05"), Puppet.Component(TEXT("digit_05")).GetRotation(), PuppetTests::Rotation(FVector::XAxisVector, 90.0));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetDigitsWeightTest, "Open3DPuppet.Modules.Digits.WeightBlendsOverInput", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetDigitsWeightTest::RunTest(const FString& Parameters)
{
	PuppetTests::FPuppet Puppet(PuppetTests::Hand());
	Puppet.Add<UPuppetModule_Digits>().Weight = 0.5f;
	PuppetDigitsTests::SolveWithIndex(Puppet, PuppetTests::Rotation(FVector::XAxisVector, 30.0));
	PuppetTests::TestRotation(*this, TEXT("Half the curl"), Puppet.Component(TEXT("index_01_l")).GetRotation(), PuppetTests::Rotation(FVector::XAxisVector, 15.0));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPuppetDigitsMissingBoneTest, "Open3DPuppet.Modules.Digits.MissingBoneSkipsTheModule", OPEN3DPUPPET_TEST_FLAGS)
bool FPuppetDigitsMissingBoneTest::RunTest(const FString& Parameters)
{
	AddExpectedMessagePlain(TEXT("the puppet has no chain from 'index_01_l' to 'index_09_l'"), ELogVerbosity::Warning);
	PuppetTests::FPuppet Puppet(PuppetTests::Hand());
	Puppet.Add<UPuppetModule_Digits>().Digits[1].PuppetEnd = TEXT("index_09_l");
	PuppetDigitsTests::SolveWithIndex(Puppet, PuppetTests::Rotation(FVector::XAxisVector, 30.0));
	TestEqual(TEXT("index_01_l left as the input"), Puppet.Component(TEXT("index_01_l")), Puppet.Reference(TEXT("index_01_l")), 1.e-3f);
	return true;
}

#endif
