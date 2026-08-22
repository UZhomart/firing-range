// Copyright zutemiss & dshadykh. Educational project.

#include "Weapons/FRWeaponBase.h"

#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

#include "Core/FRVisualUtils.h"
#include "FiringRange.h"
#include "Player/FRCharacter.h"
#include "Weapons/FRProjectile.h"

AFRWeaponBase::AFRWeaponBase()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	WeaponRoot = CreateDefaultSubobject<USceneComponent>(TEXT("WeaponRoot"));
	SetRootComponent(WeaponRoot);

	BodyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BodyMesh"));
	BodyMesh->SetupAttachment(WeaponRoot);

	BarrelMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BarrelMesh"));
	BarrelMesh->SetupAttachment(WeaponRoot);

	GripMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GripMesh"));
	GripMesh->SetupAttachment(WeaponRoot);

	MuzzlePoint = CreateDefaultSubobject<USceneComponent>(TEXT("MuzzlePoint"));
	MuzzlePoint->SetupAttachment(WeaponRoot);

	// The view model is cosmetic. It must never block a trace, take part in
	// physics, or cast a shadow across the player's own view.
	for (UStaticMeshComponent* Mesh : { BodyMesh.Get(), BarrelMesh.Get(), GripMesh.Get() })
	{
		Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Mesh->SetCastShadow(false);
	}

	ProjectileClass = AFRProjectile::StaticClass();
}

void AFRWeaponBase::BeginPlay()
{
	Super::BeginPlay();

	BuildWeaponMesh();

	AmmoInMagazine = MagazineSize;
	OnWeaponStateChanged.Broadcast();
}

void AFRWeaponBase::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
}

void AFRWeaponBase::BuildWeaponMesh()
{
	// A neutral firearm silhouette. Subclasses rebuild it with their own
	// proportions so the three weapons are told apart at a glance.
	const FLinearColor Metal(0.11f, 0.12f, 0.14f);
	const FLinearColor Polymer(0.06f, 0.07f, 0.08f);

	FRVisual::BuildMesh(BodyMesh, EFRBasicShape::Cube, Metal, false);
	BodyMesh->SetRelativeLocation(FVector(10.0f, 0.0f, 0.0f));
	BodyMesh->SetRelativeScale3D(FRVisual::SizeToScale(FVector(18.0f, 4.0f, 6.0f)));

	FRVisual::BuildMesh(BarrelMesh, EFRBasicShape::Cylinder, Metal, false);
	BarrelMesh->SetRelativeLocation(FVector(24.0f, 0.0f, 1.2f));
	BarrelMesh->SetRelativeRotation(FRotator(90.0f, 0.0f, 0.0f));
	BarrelMesh->SetRelativeScale3D(FRVisual::SizeToScale(FVector(2.4f, 2.4f, 14.0f)));

	FRVisual::BuildMesh(GripMesh, EFRBasicShape::Cube, Polymer, false);
	GripMesh->SetRelativeLocation(FVector(6.0f, 0.0f, -5.5f));
	GripMesh->SetRelativeRotation(FRotator(-12.0f, 0.0f, 0.0f));
	GripMesh->SetRelativeScale3D(FRVisual::SizeToScale(FVector(5.0f, 3.6f, 11.0f)));

	MuzzlePoint->SetRelativeLocation(FVector(32.0f, 0.0f, 1.2f));
}

void AFRWeaponBase::OnEquipped(AFRCharacter* NewOwner)
{
	OwningCharacter = NewOwner;

	if (!NewOwner)
	{
		return;
	}

	SetOwner(NewOwner);
	SetInstigator(NewOwner);

	AttachToComponent(NewOwner->GetWeaponHolder(), FAttachmentTransformRules::SnapToTargetNotIncludingScale);

	SetActorHiddenInGame(false);
	SetActorTickEnabled(true);

	OnWeaponStateChanged.Broadcast();
}

void AFRWeaponBase::OnUnequipped()
{
	StopFire();
	CancelReload();

	SetActorHiddenInGame(true);
	SetActorTickEnabled(false);
}

void AFRWeaponBase::ResetToFullMagazine()
{
	StopFire();
	CancelReload();

	AmmoInMagazine = MagazineSize;
	LastFireTime = -1000.0f;

	OnWeaponStateChanged.Broadcast();
}

bool AFRWeaponBase::CanFire() const
{
	if (bReloading || AmmoInMagazine <= 0)
	{
		return false;
	}

	const UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	return (World->GetTimeSeconds() - LastFireTime) >= FireInterval;
}

bool AFRWeaponBase::CanReload() const
{
	if (AmmoInMagazine >= MagazineSize || !OwningCharacter.IsValid())
	{
		return false;
	}

	return OwningCharacter->GetReserveAmmo(AmmoType) > 0;
}

float AFRWeaponBase::GetReloadProgress() const
{
	if (!bReloading || CurrentReloadStepDuration <= 0.0f)
	{
		return 0.0f;
	}

	const UWorld* World = GetWorld();
	if (!World)
	{
		return 0.0f;
	}

	return FMath::Clamp((World->GetTimeSeconds() - ReloadStartTime) / CurrentReloadStepDuration, 0.0f, 1.0f);
}

float AFRWeaponBase::GetCurrentSpreadDegrees() const
{
	const bool bAiming = OwningCharacter.IsValid() && OwningCharacter->IsAiming();
	float Spread = bAiming ? AimSpreadDegrees : HipSpreadDegrees;

	// Moving makes the weapon less precise, and the crosshair has to show it.
	if (OwningCharacter.IsValid())
	{
		const UCharacterMovementComponent* Movement = OwningCharacter->GetCharacterMovement();
		const float MaxSpeed = Movement ? FMath::Max(Movement->MaxWalkSpeed, 1.0f) : 1.0f;
		const float SpeedRatio = FMath::Clamp(OwningCharacter->GetVelocity().Size2D() / MaxSpeed, 0.0f, 1.0f);
		Spread += MovementSpreadPenalty * SpeedRatio;
	}

	return Spread;
}

void AFRWeaponBase::StartFire()
{
	bTriggerHeld = true;
	TryFire();
}

void AFRWeaponBase::StopFire()
{
	bTriggerHeld = false;

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(FireTimerHandle);
	}
}

void AFRWeaponBase::TryFire()
{
	if (!CanFire())
	{
		HandleFireDenied();
		return;
	}

	FireOnce();

	// An automatic weapon schedules its own next shot for as long as the trigger
	// stays down. A semi automatic one simply waits for the next click.
	if (bAutomatic && bTriggerHeld)
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimer(FireTimerHandle, this, &AFRWeaponBase::TryFire, FireInterval, false);
		}
	}
}

void AFRWeaponBase::HandleFireDenied()
{
	// Only an empty magazine deserves feedback. Being denied because the rate of
	// fire has not elapsed yet is normal and must stay silent.
	if (!bReloading && AmmoInMagazine <= 0)
	{
		PlayWeaponSound(EmptySound);

		if (bAutoReloadWhenEmpty)
		{
			StartReload();
		}
	}
}

void AFRWeaponBase::FireOnce()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	--AmmoInMagazine;
	LastFireTime = World->GetTimeSeconds();

	LaunchProjectiles();
	PlayWeaponSound(FireSound);

	// Accuracy is measured per projectile, so a shotgun blast counts as several
	// shots. Reporting it here keeps the game mode unaware of weapon internals.
	OnShotsFired.Broadcast(ProjectilesPerShot);
	OnWeaponStateChanged.Broadcast();
}

FVector AFRWeaponBase::ComputeAimPoint() const
{
	const UCameraComponent* Camera = OwningCharacter.IsValid() ? OwningCharacter->GetFirstPersonCamera() : nullptr;
	const UWorld* World = GetWorld();

	if (!Camera || !World)
	{
		return GetActorLocation() + GetActorForwardVector() * MaxAimDistance;
	}

	const FVector Start = Camera->GetComponentLocation();
	const FVector End = Start + Camera->GetForwardVector() * MaxAimDistance;

	FCollisionQueryParams QueryParams(TEXT("FRWeaponAim"), true, OwningCharacter.Get());
	QueryParams.AddIgnoredActor(this);

	FHitResult Hit;
	if (World->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, QueryParams))
	{
		return Hit.ImpactPoint;
	}

	// Nothing under the crosshair: aim at the far end of the trace so distant
	// shots still travel in the direction the player is looking.
	return End;
}

FVector AFRWeaponBase::ComputeLaunchVelocity(const FVector& MuzzleLocation, const FVector& AimPoint, float SpreadDegrees) const
{
	const UWorld* World = GetWorld();

	FVector ToTarget = AimPoint - MuzzleLocation;
	const float Distance = ToTarget.Size();

	if (Distance <= KINDA_SMALL_NUMBER)
	{
		ToTarget = GetActorForwardVector() * MaxAimDistance;
	}

	// Gravity compensation. The bullet needs Distance / MuzzleSpeed seconds to
	// arrive, and falls 0.5 * g * t^2 during that time, so the muzzle is aimed
	// that far above the crosshair. This is a first order solution: raising the
	// aim point lengthens the path slightly, but at range distances the error is
	// well under a millimetre.
	const AFRProjectile* ProjectileDefaults = ProjectileClass ? ProjectileClass->GetDefaultObject<AFRProjectile>() : nullptr;
	const float GravityScale = ProjectileDefaults ? ProjectileDefaults->GetGravityScale() : 0.0f;
	const float GravityMagnitude = World ? FMath::Abs(World->GetGravityZ()) * GravityScale : 0.0f;

	const float FlightTime = Distance / FMath::Max(MuzzleSpeed, 1.0f);
	const float Drop = 0.5f * GravityMagnitude * FlightTime * FlightTime;

	FVector Direction = ((AimPoint + FVector(0.0f, 0.0f, Drop)) - MuzzleLocation).GetSafeNormal();
	if (Direction.IsNearlyZero())
	{
		Direction = GetActorForwardVector();
	}

	if (SpreadDegrees > KINDA_SMALL_NUMBER)
	{
		Direction = FMath::VRandCone(Direction, FMath::DegreesToRadians(SpreadDegrees));
	}

	return Direction * MuzzleSpeed;
}

void AFRWeaponBase::LaunchProjectiles()
{
	UWorld* World = GetWorld();
	if (!World || !ProjectileClass)
	{
		UE_LOG(LogFiringRange, Warning, TEXT("%s has no projectile class assigned."), *DisplayName);
		return;
	}

	const FVector MuzzleLocation = MuzzlePoint->GetComponentLocation();
	const FVector AimPoint = ComputeAimPoint();
	const float Spread = GetCurrentSpreadDegrees();

	AActor* Shooter = OwningCharacter.IsValid() ? Cast<AActor>(OwningCharacter.Get()) : nullptr;

	for (int32 Index = 0; Index < ProjectilesPerShot; ++Index)
	{
		const FVector Velocity = ComputeLaunchVelocity(MuzzleLocation, AimPoint, Spread);
		const FTransform SpawnTransform(Velocity.Rotation(), MuzzleLocation);

		// Deferred spawning again: the movement component reads Velocity while it
		// registers, which happens inside FinishSpawning.
		AFRProjectile* Projectile = World->SpawnActorDeferred<AFRProjectile>(
			ProjectileClass,
			SpawnTransform,
			Shooter,
			OwningCharacter.Get(),
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn);

		if (!Projectile)
		{
			continue;
		}

		Projectile->InitialiseShot(Velocity, DamagePerProjectile, Shooter, ImpactSound);
		Projectile->FinishSpawning(SpawnTransform);
	}
}

void AFRWeaponBase::StartReload()
{
	UWorld* World = GetWorld();
	if (!World || bReloading || !CanReload())
	{
		return;
	}

	bReloading = true;
	ReloadStartTime = World->GetTimeSeconds();

	StopFire();
	PlayWeaponSound(ReloadSound);

	if (bReloadsOneRoundAtATime)
	{
		CurrentReloadStepDuration = SingleRoundReloadDuration;
		World->GetTimerManager().SetTimer(
			ReloadTimerHandle, this, &AFRWeaponBase::InsertSingleRound, SingleRoundReloadDuration, false);
	}
	else
	{
		CurrentReloadStepDuration = ReloadDuration;
		World->GetTimerManager().SetTimer(
			ReloadTimerHandle, this, &AFRWeaponBase::FinishReload, ReloadDuration, false);
	}

	OnWeaponStateChanged.Broadcast();
}

void AFRWeaponBase::CancelReload()
{
	if (!bReloading)
	{
		return;
	}

	bReloading = false;
	CurrentReloadStepDuration = 0.0f;

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ReloadTimerHandle);
	}

	OnWeaponStateChanged.Broadcast();
}

void AFRWeaponBase::FinishReload()
{
	bReloading = false;
	CurrentReloadStepDuration = 0.0f;

	if (OwningCharacter.IsValid())
	{
		const int32 Missing = MagazineSize - AmmoInMagazine;
		AmmoInMagazine += OwningCharacter->ConsumeReserveAmmo(AmmoType, Missing);
	}

	OnWeaponStateChanged.Broadcast();
}

void AFRWeaponBase::InsertSingleRound()
{
	UWorld* World = GetWorld();
	if (!World || !OwningCharacter.IsValid())
	{
		CancelReload();
		return;
	}

	AmmoInMagazine += OwningCharacter->ConsumeReserveAmmo(AmmoType, 1);
	OnWeaponStateChanged.Broadcast();

	// Keep topping the tube up until it is full or the pouch is empty.
	if (CanReload())
	{
		ReloadStartTime = World->GetTimeSeconds();
		World->GetTimerManager().SetTimer(
			ReloadTimerHandle, this, &AFRWeaponBase::InsertSingleRound, SingleRoundReloadDuration, false);
	}
	else
	{
		bReloading = false;
		CurrentReloadStepDuration = 0.0f;
		OnWeaponStateChanged.Broadcast();
	}
}

void AFRWeaponBase::PlayWeaponSound(USoundBase* Sound) const
{
	if (Sound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, Sound, GetActorLocation());
	}
}
