// Copyright zutemiss & dshadykh. Educational project.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"

#include "Core/FRTypes.h"

#include "FRRangeGameMode.generated.h"

class AFRRangeBuilder;
class AFRRangeGameState;
class AFRTargetBase;
class AFRWeaponBase;

/**
 * Rules of a range session.
 *
 * The game mode decides what a hit is worth, when a target should come back and
 * what a restart means. It writes those decisions into the game state, which is
 * the only thing the HUD ever reads. Nothing here draws anything, and nothing in
 * the HUD scores anything.
 */
UCLASS()
class FIRINGRANGE_API AFRRangeGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AFRRangeGameMode();

	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer) override;

	/**
	 * Reports the outcome of a projectile that finished its flight.
	 *
	 * Called by the bullet itself, because the moment it resolves is the only
	 * moment anyone knows whether it scored.
	 */
	void NotifyProjectileResolved(bool bScored);

	/** Adds a target to the session after it was spawned by the range builder. */
	void RegisterTarget(AFRTargetBase* Target);

	/** Clears the scoreboard, stands every target back up and refills the player. */
	void RestartRange();

	/** Points a hit in this zone is worth before the distance bonus. */
	int32 GetZoneScore(EFRHitZone Zone) const;

	/**
	 * Starts a timed run.
	 *
	 * A challenge is a restart with a clock attached: the scoreboard is cleared,
	 * every target stands back up, and from then on the session is measured
	 * against ChallengeDuration rather than being open ended.
	 */
	void StartTimedChallenge();

	/** Ends the run, keeps the result and offers it as a new personal best. */
	void EndTimedChallenge();

	/** True while a timed run is counting down. */
	bool IsChallengeRunning() const;

protected:
	// -- Scoring rules ---------------------------------------------------------

	/** Base points per hit zone. */
	UPROPERTY(EditDefaultsOnly, Category = "Firing Range|Scoring")
	TMap<EFRHitZone, int32> ZoneScores;

	/** Length of one timed run, in seconds. */
	UPROPERTY(EditDefaultsOnly, Category = "Firing Range|Challenge", meta = (ClampMin = "5.0"))
	float ChallengeDuration = 60.0f;

	/**
	 * Extra points per metre of range.
	 *
	 * Shooting the near lane is easier than shooting the far one, so the score
	 * has to say so, otherwise the whole range collapses into its first lane.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Firing Range|Scoring", meta = (ClampMin = "0.0"))
	float ScorePerMetre = 0.8f;

	// -- Internals -------------------------------------------------------------

	/**
	 * Makes sure the range exists before the first player is given a pawn.
	 *
	 * A builder already placed in the level is used as it is; otherwise one is
	 * created. Running during InitGame is what guarantees the ground and the
	 * player start are there by the time the character spawns.
	 */
	void EnsureRangeBuilt();

	/** Finds every target already in the level and starts listening to it. */
	void RegisterExistingTargets();

	/** Pushes the difficulty from the settings into every registered target. */
	void ApplyDifficultyToTargets();

	/** Subscribes to the weapons and the loadout of the player pawn. */
	void BindToPlayer(APlayerController* PlayerController);

	/** Called when a registered target is struck. */
	void HandleTargetHit(AFRTargetBase* Target, EFRHitZone Zone, float DistanceMetres);

	/** Called once per trigger pull, with the number of projectiles released. */
	void HandleShotsFired(int32 ProjectileCount);

	/** Called when the player switches weapon, to move the shot subscription. */
	void HandleActiveWeaponChanged(AFRWeaponBase* NewWeapon);

	/** Convenience accessor for the typed game state. */
	AFRRangeGameState* GetRangeGameState() const;

	/**
	 * One trigger pull, waiting for its projectiles to land.
	 *
	 * A shot is only finished once every projectile it released has resolved, and
	 * it counts as a hit if any single one of them scored. Tracking that is what
	 * lets the streak mean "shots that connected" rather than "projectiles that
	 * connected", which for a shotgun are very different numbers.
	 */
	struct FFRPendingShot
	{
		/** Projectiles of this shot that have not resolved yet. */
		int32 ProjectilesInFlight = 0;

		/** True as soon as any projectile of this shot scored. */
		bool bScored = false;
	};

	/** Shots in flight, oldest first. */
	TArray<FFRPendingShot> PendingShots;

	/** Builder that created the range, whether it was placed or generated. */
	UPROPERTY(Transient)
	TObjectPtr<AFRRangeBuilder> RangeBuilder;

	/** Targets taking part in the session. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<AFRTargetBase>> RegisteredTargets;

	/** Weapon whose shots are currently being counted. */
	TWeakObjectPtr<AFRWeaponBase> BoundWeapon;

	/** Player pawn of the session. */
	TWeakObjectPtr<class AFRCharacter> BoundCharacter;
};
