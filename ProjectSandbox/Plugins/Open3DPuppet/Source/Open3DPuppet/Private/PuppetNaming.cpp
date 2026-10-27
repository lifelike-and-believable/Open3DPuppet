// Copyright 2026 Lifelike & Believable. All Rights Reserved.

#include "PuppetNaming.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(PuppetNaming)

namespace PuppetNaming
{
	namespace Matching
	{
		/** Each canonical joint's canonical parent, for the joints the schemes map (ADR 0003). The top of the spine has none: the spine's length varies. */
		const TMap<FName, FName>& CanonicalParents()
		{
			static const TMap<FName, FName> Parents = []()
			{
				TMap<FName, FName> Result;
				Result.Add(TEXT("spine_01"), TEXT("pelvis"));
				Result.Add(TEXT("neck_01"), TEXT("spine_05"));
				Result.Add(TEXT("head"), TEXT("neck_01"));
				for (const TCHAR* Side : { TEXT("_l"), TEXT("_r") })
				{
					const auto Joint = [Side](const TCHAR* Name) { return FName(FString(Name) + Side); };
					Result.Add(Joint(TEXT("eye")), TEXT("head"));
					Result.Add(Joint(TEXT("clavicle")), TEXT("spine_05"));
					Result.Add(Joint(TEXT("upperarm")), Joint(TEXT("clavicle")));
					Result.Add(Joint(TEXT("lowerarm")), Joint(TEXT("upperarm")));
					Result.Add(Joint(TEXT("hand")), Joint(TEXT("lowerarm")));
					Result.Add(Joint(TEXT("thigh")), TEXT("pelvis"));
					Result.Add(Joint(TEXT("calf")), Joint(TEXT("thigh")));
					Result.Add(Joint(TEXT("foot")), Joint(TEXT("calf")));
					Result.Add(Joint(TEXT("ball")), Joint(TEXT("foot")));
					for (const TCHAR* Finger : { TEXT("thumb"), TEXT("index"), TEXT("middle"), TEXT("ring"), TEXT("pinky") })
					{
						const auto Segment = [Finger, Side](int32 Number) { return FName(FString::Printf(TEXT("%s_%02d%s"), Finger, Number, Side)); };
						Result.Add(Segment(1), Joint(TEXT("hand")));
						Result.Add(Segment(2), Segment(1));
						Result.Add(Segment(3), Segment(2));
					}
				}
				return Result;
			}();
			return Parents;
		}

		/** A name as matched: without a namespace ("mixamorig:"), in lower case. */
		FString Key(FName Name)
		{
			FString Text = Name.ToString();
			int32 Colon = INDEX_NONE;
			if (Text.FindLastChar(TEXT(':'), Colon))
			{
				Text.RightChopInline(Colon + 1);
			}
			return Text.ToLower();
		}
	}

	TArray<FName> CanonicalJoints()
	{
		return {};
	}

	FMatch Match(TConstArrayView<FName> SourceBones, TConstArrayView<int32> SourceParents, TConstArrayView<FPuppetNamingScheme> Schemes)
	{
		TArray<FString> Keys;
		for (const FName Bone : SourceBones)
		{
			Keys.Add(Matching::Key(Bone));
		}
		// A prefix every bone shares is a skeleton or actor name ("Actor_"); one bone has no such prefix.
		int32 Prefix = 0;
		if (Keys.Num() >= 2)
		{
			Prefix = Keys[0].Len();
			for (const FString& Key : Keys)
			{
				int32 Common = 0;
				while (Common < Prefix && Common < Key.Len() && Key[Common] == Keys[0][Common])
				{
					++Common;
				}
				Prefix = FMath::Min(Common, Key.Len() - 1);
			}
		}
		TMap<FString, int32> BoneByKey;
		for (int32 Bone = 0; Bone < Keys.Num(); ++Bone)
		{
			if (!BoneByKey.Contains(Keys[Bone].RightChop(Prefix)))
			{
				BoneByKey.Add(Keys[Bone].RightChop(Prefix), Bone);
			}
		}

		FMatch Result;
		int32 BestScore = 0;
		TArray<TMap<FName, int32>> FoundByScheme;
		TArray<int32> Scores;
		for (int32 Scheme = 0; Scheme < Schemes.Num(); ++Scheme)
		{
			TMap<FName, int32>& Found = FoundByScheme.AddDefaulted_GetRef();
			for (const TPair<FName, FName>& Row : Schemes[Scheme].CanonicalToSource)
			{
				if (const int32* Bone = BoneByKey.Find(Matching::Key(Row.Value)))
				{
					Found.Add(Row.Key, *Bone);
				}
			}
			// A joint whose source parent is not the source of its canonical parent counts against
			// the scheme, so a scheme that misreads the skeleton (the wrong top of the spine) loses.
			int32 Contradicted = 0;
			for (const TPair<FName, int32>& Joint : Found)
			{
				const FName* CanonicalParent = Matching::CanonicalParents().Find(Joint.Key);
				const int32* ParentBone = CanonicalParent ? Found.Find(*CanonicalParent) : nullptr;
				if (ParentBone && (!SourceParents.IsValidIndex(Joint.Value) || SourceParents[Joint.Value] != *ParentBone))
				{
					++Contradicted;
				}
			}
			Scores.Add(Found.Num() - Contradicted);
			if (Scores.Last() > BestScore)
			{
				BestScore = Scores.Last();
				Result.Scheme = Scheme;
			}
		}
		if (Result.Scheme == INDEX_NONE)
		{
			return Result;
		}
		const TMap<FName, int32>& BestFound = FoundByScheme[Result.Scheme];
		const auto SameBones = [&BestFound](const TMap<FName, int32>& Found)
		{
			if (Found.Num() != BestFound.Num())
			{
				return false;
			}
			for (const TPair<FName, int32>& Joint : Found)
			{
				const int32* Bone = BestFound.Find(Joint.Key);
				if (!Bone || *Bone != Joint.Value)
				{
					return false;
				}
			}
			return true;
		};
		for (int32 Scheme = Result.Scheme; Scheme < Schemes.Num(); ++Scheme)
		{
			if (Scores[Scheme] == BestScore && SameBones(FoundByScheme[Scheme]))
			{
				Result.Equivalent.Add(Scheme);
			}
		}

		for (const TPair<FName, FName>& Row : Schemes[Result.Scheme].CanonicalToSource)
		{
			const int32* Bone = BestFound.Find(Row.Key);
			if (!Bone)
			{
				Result.Missing.Add(Row.Key);
			}
			// A bone already named canonically needs no entry; FName comparison ignores case, as the lookup does.
			else if (SourceBones[*Bone] != Row.Key)
			{
				Result.CanonicalToSource.Add(Row.Key, SourceBones[*Bone]);
			}
		}
		return Result;
	}
}
