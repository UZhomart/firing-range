// Copyright zutemiss & dshadykh. Educational project.

#include "Weapons/FRProjectile.h"

#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/DamageType.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"

#include "Core/FRRangeGameMode.h"
#include "Core/FRVisualUtils.h"
#include "Weapons/FRImpactEffect.h"

AFRProjectile::AFRProjectile()
{
	PrimaryActorTick.bCanEverTick = false;

	CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionComponent"));
	CollisionComponent->InitSphereRadius(BulletRadius);
	CollisionComponent->SetCollisionProfileName(TEXT("FRProjectile"));
	CollisionComponent->SetGenerateOverlapEvents(false);
	CollisionComponent->CanCharacterStepUpOn = ECB_No;
	CollisionComponent->SetCastShadow(false);
	CollisionComponent->OnComponentHit.AddDynamic(this, &AFRProjectile::HandleHit);
	SetRootComponent(CollisionComponent);

	TracerMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TracerMesh"));
	TracerMesh->SetupAttachment(CollisionComponent);
	TracerMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	TracerMesh->SetCastShadow(false);

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->SetUpdatedComponent(CollisionComponent);

	// Velocity is written directly by the weapon, so the component must not
	// rebuild it from InitialSpeed or reinterpret it in local space.
	ProjectileMovement->InitialSpeed = 0.0f;
	ProjectileMovement->MaxSpeed = 0.0f;
	ProjectileMovement->bInitialVelocityInLocalSpace = false;

	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->bShouldBounce = false;
	ProjectileMovement->ProjectileGravityScale = 0.12f;

	// A bullet crosses several metres per frame. Sub stepping keeps the sweep
	// accurate enough that a thin target board is never tunnelled through.
	ProjectileMovement->bForceSubStepping = true;
	ProjectileMovement->MaxSimulationTimeStep = 0.0166f;
	ProjectileMovement->MaxSimulationIterations = 8;

	InitialLifeSpan = MaxFlightTime;
}

void AFRProjectile::InitialiseShot(const FVector& InLaunchVelocity, float InDamage, AActor* InShooter, USoundBase* InImpactSound)
{
	Damage = InDamage;
	ImpactSound = InImpactSound;

	ProjectileMovement->Velocity = InLaunchVelocity;

	if (InShooter)
	{
		SetOwner(InShooter);
		SetInstigator(Cast<APawn>(InShooter));

		// The muzzle sits inside the shooter's own capsule. Without this the very
		// first sweep of the bullet would report a hit on the player.
		CollisionComponent->IgnoreActorWhenMoving(InShooter, true);
	}
}

void AFRProjectile::BeginPlay()
{
	Super::BeginPlay();

	CollisionComponent->SetSphereRadius(BulletRadius);

	// The cylinder primitive is built along its own Z axis while the projectile
	// flies along X, so the mesh is turned a quarter revolution to line up with
	// the direction of travel.
	FRVisual::BuildMesh(TracerMesh, EFRBasicShape::Cylinder, TracerColor, false);
	TracerMesh->SetRelativeRotation(FRotator(90.0f, 0.0f, 0.0f));
	TracerMesh->SetRelativeScale3D(FVector(BulletRadius * 2.0f / 100.0f, BulletRadius * 2.0f / 100.0f, TracerLength / 100.0f));
}

void AFRProjectile::ReportOutcome()
{
	if (bOutcomeReported)
	{
		return;
	}

	bOutcomeReported = true;

	if (UWorld* World = GetWorld())
	{
		if (AFRRangeGameMode* GameMode = World->GetAuthGameMode<AFRRangeGameMode>())
		{
			GameMode->NotifyProjectileResolved();
		}
	}
}

void AFRProjectile::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// A bullet that simply ran out of flight time never called HandleHit, and its
	// shot still has to be closed. Only a genuine destruction counts: a level
	// change must not resolve shots for everything that happened to be in the air.
	if (EndPlayReason == EEndPlayReason::Destroyed)
	{
		ReportOutcome();
	}

	Super::EndPlay(EndPlayReason);
}

float AFRProjectile::GetGravityScale() const
{
	return ProjectileMovement ? ProjectileMovement->ProjectileGravityScale : 0.0f;
}

void AFRProjectile::HandleHit(
	UPrimitiveComponent* HitComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComponent,
	FVector NormalImpulse,
	const FHitResult& Hit)
{
	const FVector ShotDirection = GetVelocity().GetSafeNormal();

	// Damage is routed through the engine damage pipeline instead of a direct
	// call. That keeps the bullet unaware of what a target is: anything that
	// overrides TakeDamage can react to being shot, and the target decides for
	// itself which of its zones was struck.
	//
	// The damage the call returns is deliberately ignored. Every actor accepts
	// damage by default, so a wall reports a hit just as a bullseye does. Whether
	// the shot scored is announced by the target itself, not inferred here.
	if (OtherActor && OtherActor != this)
	{
		UGameplayStatics::ApplyPointDamage(
			OtherActor,
			Damage,
			ShotDirection,
			Hit,
			GetInstigatorController(),
			this,
			UDamageType::StaticClass());
	}

	ReportOutcome();

	// FHitResult stores its normals as FVector_NetQuantizeNormal, a compressed
	// network type. Both branches of the conditional must agree on one type, so
	// the stored normal is converted back to a plain vector explicitly.
	const FVector ImpactNormal = Hit.ImpactNormal.IsNearlyZero() ? -ShotDirection : FVector(Hit.ImpactNormal);
	AFRImpactEffect::PlayImpactFeedback(this, Hit.ImpactPoint, ImpactNormal, TracerColor, ImpactSound);

	Destroy();
}
