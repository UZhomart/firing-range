// Copyright zutemiss & dshadykh. Educational project.

#include "Core/FRRangeGameMode.h"

#include "EngineUtils.h"
#include "GameFramework/PlayerStart.h"

#include "Core/FRGameInstance.h"
#include "Core/FRRangeGameState.h"
#include "FiringRange.h"
#include "Level/FRRangeBuilder.h"
#include "Pickups/FRAmmoPickup.h"
#include "Player/FRCharacter.h"
#include "Player/FRHUD.h"
#include "Player/FRPlayerController.h"
#include "Targets/FRTargetBase.h"
#include "Weapons/FRWeaponBase.h"

AFRRangeGameMode::AFRRangeGameMode()
{
	// The challenge clock runs on the game mode, so the mode has to tick.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	// Every framework class of the session is native C++. Nothing here points at
	// a Blueprint, which is what lets the project ship without binary assets.
	DefaultPawnClass = AFRCharacter::StaticClass();
	PlayerControllerClass = AFRPlayerController::StaticClass();
	GameStateClass = AFRRangeGameState::StaticClass();
	HUDClass = AFRHUD::StaticClass();

	// A tighter group is worth more. The head is the hardest zone on the board
	// and pays the most, which is what the bonus in the brief asks for.
	ZoneScores.Add(EFRHitZone::Body, 10);
	ZoneScores.Add(EFRHitZone::Inner, 25);
	ZoneScores.Add(EFRHitZone::Bullseye, 50);
	ZoneScores.Add(EFRHitZone::Head, 75);
}

void AFRRangeGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);

	EnsureRangeBuilt();
}

void AFRRangeGameMode::BeginPlay()
{
	Super::BeginPlay();

	RegisterExistingTargets();
	ApplyDifficultyToTargets();
}

void AFRRangeGameMode::EnsureRangeBuilt()
{
	UWorld* World = GetWorld();
	if (!World || RangeBuilder)
	{
		return;
	}

	// A builder placed by hand in the level wins, so the generated range can
	// always be replaced by an authored one without touching this class.
	for (TActorIterator<AFRRangeBuilder> It(World); It; ++It)
	{
		RangeBuilder = *It;
		break;
	}

	if (!RangeBuilder)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		RangeBuilder = World->SpawnActor<AFRRangeBuilder>(
			AFRRangeBuilder::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
	}

	if (RangeBuilder)
	{
		RangeBuilder->BuildRange();
	}
}

void AFRRangeGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!IsChallengeRunning())
	{
		return;
	}

	AFRRangeGameState* RangeState = GetRangeGameState();
	if (!RangeState)
	{
		return;
	}

	const float Remaining = RangeState->GetChallengeTimeRemaining() - DeltaSeconds;
	RangeState->SetChallengeTimeRemaining(Remaining);

	if (Remaining <= 0.0f)
	{
		EndTimedChallenge();
	}
}

bool AFRRangeGameMode::IsChallengeRunning() const
{
	const AFRRangeGameState* RangeState = GetRangeGameState();
	return RangeState && RangeState->GetSessionState() == EFRSessionState::Challenge;
}

void AFRRangeGameMode::StartTimedChallenge()
{
	AFRRangeGameState* RangeState = GetRangeGameState();
	if (!RangeState)
	{
		return;
	}

	// RestartRange clears the board first, so a challenge always begins from a
	// clean scoreboard and a full range whatever the player was doing before.
	RestartRange();

	RangeState->SetChallengeDuration(ChallengeDuration);
	RangeState->SetChallengeTimeRemaining(ChallengeDuration);
	RangeState->SetSessionState(EFRSessionState::Challenge);

	UE_LOG(LogFiringRange, Log, TEXT("Timed challenge started: %.0f seconds."), ChallengeDuration);
}

void AFRRangeGameMode::EndTimedChallenge()
{
	AFRRangeGameState* RangeState = GetRangeGameState();
	if (!RangeState)
	{
		return;
	}

	const FFRRangeStats& Stats = RangeState->GetStats();

	RangeState->SetChallengeTimeRemaining(0.0f);
	RangeState->SetLastChallengeScore(Stats.Score);
	RangeState->SetSessionState(EFRSessionState::ChallengeEnded);

	// The record is only kept when it is actually a record, which the game
	// instance decides, because it is the thing that owns the saved file.
	if (UFRGameInstance* GameInstance = UFRGameInstance::Get(this))
	{
		GameInstance->SubmitChallengeResult(Stats.Score, Stats.GetAccuracy());
	}

	UE_LOG(LogFiringRange, Log, TEXT("Timed challenge finished with %d points at %s accuracy."),
		Stats.Score, *Stats.GetAccuracyText());
}

AFRRangeGameState* AFRRangeGameMode::GetRangeGameState() const
{
	return GetWorld() ? GetWorld()->GetGameState<AFRRangeGameState>() : nullptr;
}

int32 AFRRangeGameMode::GetZoneScore(EFRHitZone Zone) const
{
	const int32* Found = ZoneScores.Find(Zone);
	return Found ? *Found : 0;
}

// -- Targets -----------------------------------------------------------------

void AFRRangeGameMode::RegisterExistingTargets()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	for (TActorIterator<AFRTargetBase> It(World); It; ++It)
	{
		RegisterTarget(*It);
	}

	UE_LOG(LogFiringRange, Log, TEXT("Range session registered %d targets."), RegisteredTargets.Num());
}

void AFRRangeGameMode::RegisterTarget(AFRTargetBase* Target)
{
	if (!Target || RegisteredTargets.Contains(Target))
	{
		return;
	}

	RegisteredTargets.Add(Target);
	Target->OnTargetHit.AddUObject(this, &AFRRangeGameMode::HandleTargetHit);

	if (const UFRGameInstance* GameInstance = UFRGameInstance::Get(this))
	{
		Target->SetDifficulty(GameInstance->GetDifficulty());
	}
}

void AFRRangeGameMode::ApplyDifficultyToTargets()
{
	const UFRGameInstance* GameInstance = UFRGameInstance::Get(this);
	if (!GameInstance)
	{
		return;
	}

	const EFRDifficulty Difficulty = GameInstance->GetDifficulty();
	for (AFRTargetBase* Target : RegisteredTargets)
	{
		if (Target)
		{
			Target->SetDifficulty(Difficulty);
		}
	}
}

void AFRRangeGameMode::HandleTargetHit(AFRTargetBase* Target, EFRHitZone Zone, float DistanceMetres)
{
	AFRRangeGameState* RangeState = GetRangeGameState();
	if (!RangeState)
	{
		return;
	}

	const int32 BaseScore = GetZoneScore(Zone);
	const int32 DistanceBonus = FMath::FloorToInt(DistanceMetres * ScorePerMetre);
	const int32 Points = BaseScore + DistanceBonus;

	RangeState->RegisterHit(Zone, DistanceMetres, Points);
	RangeState->RegisterTargetDown();

	// The target is the only reliable witness of a scoring hit. The damage the
	// bullet dealt says nothing on its own: every actor accepts damage by
	// default, so a bullet buried in a berm reports damage just like one in a
	// bullseye. The hit is announced while the bullet is still resolving, so the
	// shot it belongs to is still the oldest open one.
	if (PendingShots.Num() > 0)
	{
		PendingShots[0].bScored = true;
	}
}

// -- Player ------------------------------------------------------------------

void AFRRangeGameMode::HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer)
{
	// The pawn and its loadout only exist after the base implementation has run,
	// because that is what spawns and possesses the character.
	Super::HandleStartingNewPlayer_Implementation(NewPlayer);

	BindToPlayer(NewPlayer);
}

void AFRRangeGameMode::BindToPlayer(APlayerController* PlayerController)
{
	AFRCharacter* Character = PlayerController ? Cast<AFRCharacter>(PlayerController->GetPawn()) : nullptr;
	if (!Character)
	{
		return;
	}

	BoundCharacter = Character;

	Character->OnActiveWeaponChanged.AddUObject(this, &AFRRangeGameMode::HandleActiveWeaponChanged);

	// Only the weapon in hand can fire, so only the weapon in hand has to be
	// listened to. The subscription moves whenever the player switches.
	HandleActiveWeaponChanged(Character->GetActiveWeapon());
}

void AFRRangeGameMode::HandleActiveWeaponChanged(AFRWeaponBase* NewWeapon)
{
	if (AFRWeaponBase* Previous = BoundWeapon.Get())
	{
		Previous->OnShotsFired.RemoveAll(this);
	}

	BoundWeapon = NewWeapon;

	if (NewWeapon)
	{
		NewWeapon->OnShotsFired.AddUObject(this, &AFRRangeGameMode::HandleShotsFired);
	}
}

void AFRRangeGameMode::HandleShotsFired(int32 ProjectileCount)
{
	if (AFRRangeGameState* RangeState = GetRangeGameState())
	{
		RangeState->RegisterShots(ProjectileCount);
	}

	// Opened here and closed in NotifyProjectileResolved, once every projectile
	// of this pull has landed.
	FFRPendingShot Shot;
	Shot.ProjectilesInFlight = FMath::Max(1, ProjectileCount);
	PendingShots.Add(Shot);
}

void AFRRangeGameMode::NotifyProjectileResolved()
{
	if (PendingShots.Num() == 0)
	{
		return;
	}

	// Projectiles resolve in roughly the order they were fired, so the oldest
	// open shot is the one this bullet belongs to. Being off by one shot during
	// rapid fire costs nothing: the counter it feeds is a streak, not a score.
	FFRPendingShot& Shot = PendingShots[0];

	--Shot.ProjectilesInFlight;

	if (Shot.ProjectilesInFlight > 0)
	{
		return;
	}

	const bool bShotConnected = Shot.bScored;
	PendingShots.RemoveAt(0);

	if (AFRRangeGameState* RangeState = GetRangeGameState())
	{
		RangeState->RegisterShotOutcome(bShotConnected);
	}
}

// -- Session -----------------------------------------------------------------

void AFRRangeGameMode::RestartRange()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	if (AFRRangeGameState* RangeState = GetRangeGameState())
	{
		RangeState->ResetStats();
		RangeState->SetSessionState(EFRSessionState::FreePractice);
	}

	// Bullets fired before the restart belong to the session that just ended.
	PendingShots.Reset();

	for (AFRTargetBase* Target : RegisteredTargets)
	{
		if (Target)
		{
			Target->ResetTarget();
		}
	}

	// Crates that were collected come straight back, so a restart really is a
	// clean slate rather than a clean scoreboard on a stripped range.
	for (TActorIterator<AFRAmmoPickup> It(World); It; ++It)
	{
		It->ResetPickup();
	}

	if (AFRCharacter* Character = BoundCharacter.Get())
	{
		Character->ResetLoadout();

		// Put the player back on the firing line. Moving the existing pawn rather
		// than respawning it keeps every subscription of this game mode intact.
		if (const AActor* StartSpot = FindPlayerStart(Character->GetController()))
		{
			Character->SetActorLocationAndRotation(StartSpot->GetActorLocation(), StartSpot->GetActorRotation());

			if (AController* Controller = Character->GetController())
			{
				Controller->SetControlRotation(StartSpot->GetActorRotation());
			}
		}
	}

	ApplyDifficultyToTargets();

	UE_LOG(LogFiringRange, Log, TEXT("Range session restarted."));
}
