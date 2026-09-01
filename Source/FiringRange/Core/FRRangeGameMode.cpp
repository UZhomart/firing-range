// Copyright zutemiss & dshadykh. Educational project.

#include "Core/FRRangeGameMode.h"

#include "EngineUtils.h"
#include "GameFramework/PlayerStart.h"

#include "Core/FRGameInstance.h"
#include "Core/FRRangeGameState.h"
#include "FiringRange.h"
#include "Pickups/FRAmmoPickup.h"
#include "Player/FRCharacter.h"
#include "Player/FRPlayerController.h"
#include "Targets/FRTargetBase.h"
#include "Weapons/FRWeaponBase.h"

AFRRangeGameMode::AFRRangeGameMode()
{
	// Every framework class of the session is native C++. Nothing here points at
	// a Blueprint, which is what lets the project ship without binary assets.
	DefaultPawnClass = AFRCharacter::StaticClass();
	PlayerControllerClass = AFRPlayerController::StaticClass();
	GameStateClass = AFRRangeGameState::StaticClass();

	// A tighter group is worth more. The head is the hardest zone on the board
	// and pays the most, which is what the bonus in the brief asks for.
	ZoneScores.Add(EFRHitZone::Body, 10);
	ZoneScores.Add(EFRHitZone::Inner, 25);
	ZoneScores.Add(EFRHitZone::Bullseye, 50);
	ZoneScores.Add(EFRHitZone::Head, 75);
}

void AFRRangeGameMode::BeginPlay()
{
	Super::BeginPlay();

	RegisterExistingTargets();
	ApplyDifficultyToTargets();
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
}

void AFRRangeGameMode::NotifyProjectileResolved(bool bScored)
{
	if (bScored)
	{
		// A scoring projectile was already accounted for by the target it struck.
		return;
	}

	if (AFRRangeGameState* RangeState = GetRangeGameState())
	{
		RangeState->RegisterMiss();
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
