// Copyright zutemiss & dshadykh. Educational project.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"

#include "Core/FRTypes.h"

#include "FRRangeGameState.generated.h"

/** Raised whenever any figure on the scoreboard changed. */
DECLARE_MULTICAST_DELEGATE(FFROnRangeStatsChanged);

/** Raised on every scoring hit, with the zone, the range in metres and the points. */
DECLARE_MULTICAST_DELEGATE_ThreeParams(FFROnHitRegistered, EFRHitZone /*Zone*/, float /*DistanceMetres*/, int32 /*Points*/);

/** Raised when the session moves between practice, challenge and result. */
DECLARE_MULTICAST_DELEGATE_OneParam(FFROnSessionStateChanged, EFRSessionState /*NewState*/);

/**
 * Scoreboard of the current range session.
 *
 * The game state is the object both sides of the session are allowed to touch:
 * the game mode writes to it because it owns the rules, and the HUD reads from
 * it because it owns the presentation. Neither ever talks to the other directly,
 * which is what keeps the scoring rules out of the drawing code.
 *
 * It stores figures and it announces changes. Every decision about what a hit is
 * worth, or when a challenge ends, belongs to the game mode.
 */
UCLASS()
class FIRINGRANGE_API AFRRangeGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	AFRRangeGameState();

	/** Current figures, read by the HUD every frame. */
	const FFRRangeStats& GetStats() const { return Stats; }

	/** Counts projectiles that left a muzzle. A shotgun blast counts as its pellets. */
	void RegisterShots(int32 ProjectileCount);

	/** Counts one scoring hit and adds its points. */
	void RegisterHit(EFRHitZone Zone, float DistanceMetres, int32 Points);

	/**
	 * Counts one projectile that finished its flight without scoring.
	 *
	 * A miss cannot be detected when the trigger is pulled, because the bullet is
	 * still travelling. It is reported by the projectile itself once it resolves,
	 * which is the only moment the outcome is actually known.
	 */
	void RegisterMiss();

	/** Counts a target that was knocked down. */
	void RegisterTargetDown();

	/** Clears the scoreboard. Used by the restart in the pause menu. */
	void ResetStats();

	// -- Session ---------------------------------------------------------------

	EFRSessionState GetSessionState() const { return SessionState; }
	void SetSessionState(EFRSessionState NewState);

	/** Seconds left of a timed challenge. Zero outside of one. */
	float GetChallengeTimeRemaining() const { return ChallengeTimeRemaining; }

	/** Written by the game mode while a challenge counts down. */
	void SetChallengeTimeRemaining(float NewTime);

	/** Total length of the challenge that is running, for the HUD progress bar. */
	float GetChallengeDuration() const { return ChallengeDuration; }
	void SetChallengeDuration(float NewDuration) { ChallengeDuration = NewDuration; }

	/** Score of the challenge that just finished, kept for the result panel. */
	int32 GetLastChallengeScore() const { return LastChallengeScore; }
	void SetLastChallengeScore(int32 NewScore) { LastChallengeScore = NewScore; }

	FFROnRangeStatsChanged OnStatsChanged;
	FFROnHitRegistered OnHitRegistered;
	FFROnSessionStateChanged OnSessionStateChanged;

protected:
	/** Every figure on the scoreboard, in one struct so a reset is one assignment. */
	UPROPERTY(Transient, VisibleInstanceOnly, Category = "Firing Range|Session")
	FFRRangeStats Stats;

	UPROPERTY(Transient, VisibleInstanceOnly, Category = "Firing Range|Session")
	EFRSessionState SessionState = EFRSessionState::FreePractice;

	UPROPERTY(Transient, VisibleInstanceOnly, Category = "Firing Range|Session")
	float ChallengeTimeRemaining = 0.0f;

	UPROPERTY(Transient, VisibleInstanceOnly, Category = "Firing Range|Session")
	float ChallengeDuration = 0.0f;

	UPROPERTY(Transient, VisibleInstanceOnly, Category = "Firing Range|Session")
	int32 LastChallengeScore = 0;
};
