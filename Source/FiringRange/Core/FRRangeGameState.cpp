// Copyright zutemiss & dshadykh. Educational project.

#include "Core/FRRangeGameState.h"

AFRRangeGameState::AFRRangeGameState()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AFRRangeGameState::RegisterShots(int32 ProjectileCount)
{
	if (ProjectileCount <= 0)
	{
		return;
	}

	// Only the denominator of the accuracy figure moves here. Whether these
	// projectiles hit anything is not known yet: they are still in the air.
	Stats.ShotsFired += ProjectileCount;

	OnStatsChanged.Broadcast();
}

void AFRRangeGameState::RegisterHit(EFRHitZone Zone, float DistanceMetres, int32 Points)
{
	if (Zone == EFRHitZone::None)
	{
		return;
	}

	// The streak is not touched here. It belongs to the trigger pull as a whole
	// and is settled by RegisterShotOutcome once every projectile has landed.
	++Stats.ShotsHit;
	Stats.Score += Points;

	if (Zone == EFRHitZone::Head)
	{
		++Stats.Headshots;
	}

	OnHitRegistered.Broadcast(Zone, DistanceMetres, Points);
	OnStatsChanged.Broadcast();
}

void AFRRangeGameState::RegisterShotOutcome(bool bScored)
{
	// Accuracy is untouched here: every projectile of this shot was already
	// counted when it left the muzzle, and the ones that connected were counted
	// by RegisterHit. Only the streak is decided at this point.
	if (bScored)
	{
		++Stats.CurrentStreak;
		Stats.BestStreak = FMath::Max(Stats.BestStreak, Stats.CurrentStreak);
	}
	else
	{
		Stats.CurrentStreak = 0;
	}

	OnStatsChanged.Broadcast();
}

void AFRRangeGameState::RegisterTargetDown()
{
	++Stats.TargetsDown;
	OnStatsChanged.Broadcast();
}

void AFRRangeGameState::ResetStats()
{
	Stats.Reset();
	ChallengeTimeRemaining = 0.0f;
	ChallengeDuration = 0.0f;

	OnStatsChanged.Broadcast();
}

void AFRRangeGameState::SetSessionState(EFRSessionState NewState)
{
	if (SessionState == NewState)
	{
		return;
	}

	SessionState = NewState;
	OnSessionStateChanged.Broadcast(NewState);
}

void AFRRangeGameState::SetChallengeTimeRemaining(float NewTime)
{
	ChallengeTimeRemaining = FMath::Max(0.0f, NewTime);
}
