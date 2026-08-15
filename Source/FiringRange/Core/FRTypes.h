// Copyright zutemiss & dshadykh. Educational project.

#pragma once

#include "CoreMinimal.h"

#include "FRTypes.generated.h"

/**
 * Ammunition families.
 *
 * Reserve ammunition is stored per family rather than per weapon, so picking up
 * a box of shells refills every weapon that feeds from shells.
 */
UENUM(BlueprintType)
enum class EFRAmmoType : uint8
{
	Pistol   UMETA(DisplayName = "9 mm"),
	Shell    UMETA(DisplayName = "12 Gauge"),
	Rifle    UMETA(DisplayName = "7.62 mm"),

	Count    UMETA(Hidden)
};

/** Where a projectile landed on a target. Drives both score and feedback. */
UENUM(BlueprintType)
enum class EFRHitZone : uint8
{
	None     UMETA(DisplayName = "Miss"),
	Body     UMETA(DisplayName = "Body"),
	Inner    UMETA(DisplayName = "Inner Ring"),
	Bullseye UMETA(DisplayName = "Bullseye"),
	Head     UMETA(DisplayName = "Head")
};

/** Movement pattern executed by the AI controller that drives a target. */
UENUM(BlueprintType)
enum class EFRTargetMotion : uint8
{
	/** Never moves. Used by the stationary lane. */
	Static       UMETA(DisplayName = "Static"),
	/** Follows the patrol spline and jumps back to the start when it ends. */
	PathLoop     UMETA(DisplayName = "Path Loop"),
	/** Follows the patrol spline forwards, then backwards. */
	PathPingPong UMETA(DisplayName = "Path Ping Pong"),
	/** Picks random points inside the lane and strafes between them. */
	RandomStrafe UMETA(DisplayName = "Random Strafe"),
	/** Circles around the spline origin. */
	Orbit        UMETA(DisplayName = "Orbit")
};

/** Difficulty scales target speed, respawn delay and pause length. */
UENUM(BlueprintType)
enum class EFRDifficulty : uint8
{
	Easy   UMETA(DisplayName = "Easy"),
	Normal UMETA(DisplayName = "Normal"),
	Hard   UMETA(DisplayName = "Hard")
};

/** High level state of the range session, owned by the game mode. */
UENUM(BlueprintType)
enum class EFRSessionState : uint8
{
	/** Endless practice: targets respawn forever and nothing is timed. */
	FreePractice   UMETA(DisplayName = "Free Practice"),
	/** Timed challenge is running. */
	Challenge      UMETA(DisplayName = "Challenge"),
	/** Timed challenge ended, the result screen is on the HUD. */
	ChallengeEnded UMETA(DisplayName = "Challenge Ended")
};

/**
 * Everything the HUD needs to know about the current session.
 *
 * The struct lives on the game state, which is the object both the game mode
 * (writer) and the HUD (reader) are allowed to talk to.
 */
USTRUCT(BlueprintType)
struct FFRRangeStats
{
	GENERATED_BODY()

	/** Number of projectiles that left a muzzle. A shotgun blast counts as one shot per pellet. */
	UPROPERTY(BlueprintReadOnly, Category = "Range")
	int32 ShotsFired = 0;

	/** Number of projectiles that touched a target. */
	UPROPERTY(BlueprintReadOnly, Category = "Range")
	int32 ShotsHit = 0;

	/** Accumulated score, weighted by hit zone and distance. */
	UPROPERTY(BlueprintReadOnly, Category = "Range")
	int32 Score = 0;

	/** Hits landed in a row without a miss. */
	UPROPERTY(BlueprintReadOnly, Category = "Range")
	int32 CurrentStreak = 0;

	/** Longest streak of the session. */
	UPROPERTY(BlueprintReadOnly, Category = "Range")
	int32 BestStreak = 0;

	/** How many of the hits were headshots. */
	UPROPERTY(BlueprintReadOnly, Category = "Range")
	int32 Headshots = 0;

	/** Targets knocked down during the session. */
	UPROPERTY(BlueprintReadOnly, Category = "Range")
	int32 TargetsDown = 0;

	/** Accuracy as a 0..1 ratio. Returns 0 before the first shot so the HUD never divides by zero. */
	float GetAccuracy() const
	{
		return ShotsFired > 0 ? static_cast<float>(ShotsHit) / static_cast<float>(ShotsFired) : 0.0f;
	}

	/** Accuracy formatted for the HUD, for example "87.5 %". */
	FString GetAccuracyText() const
	{
		return FString::Printf(TEXT("%.1f %%"), GetAccuracy() * 100.0f);
	}

	/** Resets the session back to a clean slate. Used by the pause menu restart. */
	void Reset()
	{
		*this = FFRRangeStats();
	}
};

/**
 * Free functions shared by several subsystems.
 *
 * They are `inline` inside a namespace instead of a UBlueprintFunctionLibrary
 * because no Blueprint ever needs them: the whole project is native C++.
 */
namespace FRTypes
{
	/** Human readable name of an ammunition family, used by the HUD and by pickups. */
	inline FString GetAmmoTypeName(EFRAmmoType AmmoType)
	{
		switch (AmmoType)
		{
		case EFRAmmoType::Pistol: return TEXT("9 mm");
		case EFRAmmoType::Shell:  return TEXT("12 Gauge");
		case EFRAmmoType::Rifle:  return TEXT("7.62 mm");
		default:                  return TEXT("Unknown");
		}
	}

	/** Short label drawn by the hit marker when a target is struck. */
	inline FString GetHitZoneName(EFRHitZone Zone)
	{
		switch (Zone)
		{
		case EFRHitZone::Head:     return TEXT("HEADSHOT");
		case EFRHitZone::Bullseye: return TEXT("BULLSEYE");
		case EFRHitZone::Inner:    return TEXT("INNER");
		case EFRHitZone::Body:     return TEXT("HIT");
		default:                   return FString();
		}
	}

	/** Colour associated with a hit zone. Keeps HUD and impact decals consistent. */
	inline FLinearColor GetHitZoneColor(EFRHitZone Zone)
	{
		switch (Zone)
		{
		case EFRHitZone::Head:     return FLinearColor(1.00f, 0.25f, 0.20f);
		case EFRHitZone::Bullseye: return FLinearColor(1.00f, 0.80f, 0.15f);
		case EFRHitZone::Inner:    return FLinearColor(0.60f, 0.90f, 1.00f);
		case EFRHitZone::Body:     return FLinearColor(0.85f, 0.90f, 0.95f);
		default:                   return FLinearColor::White;
		}
	}

	/** Multiplier applied to target speed and reaction time for a difficulty level. */
	inline float GetDifficultyScale(EFRDifficulty Difficulty)
	{
		switch (Difficulty)
		{
		case EFRDifficulty::Easy:   return 0.65f;
		case EFRDifficulty::Hard:   return 1.60f;
		case EFRDifficulty::Normal:
		default:                    return 1.0f;
		}
	}

	/** Number of entries in EFRAmmoType, used to size per family ammo arrays. */
	inline constexpr int32 AmmoTypeCount()
	{
		return static_cast<int32>(EFRAmmoType::Count);
	}
}
