// Copyright zutemiss & dshadykh. Educational project.

#include "Pickups/FRAmmoPickup.h"

#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

#include "Core/FRVisualUtils.h"
#include "FiringRange.h"
#include "Player/FRCharacter.h"

AFRAmmoPickup::AFRAmmoPickup()
{
	PrimaryActorTick.bCanEverTick = true;

	PickupRoot = CreateDefaultSubobject<USceneComponent>(TEXT("PickupRoot"));
	SetRootComponent(PickupRoot);

	CollectionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollectionSphere"));
	CollectionSphere->SetupAttachment(PickupRoot);
	CollectionSphere->InitSphereRadius(CollectionRadius);

	// Overlap only: the crate must never stop a bullet or block the player.
	CollectionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollectionSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	CollectionSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	CollectionSphere->SetGenerateOverlapEvents(true);
	CollectionSphere->OnComponentBeginOverlap.AddDynamic(this, &AFRAmmoPickup::HandleBeginOverlap);

	CrateMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CrateMesh"));
	CrateMesh->SetupAttachment(PickupRoot);
	CrateMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	BandMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BandMesh"));
	BandMesh->SetupAttachment(CrateMesh);
	BandMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AFRAmmoPickup::BeginPlay()
{
	Super::BeginPlay();

	BuildPickupMesh();

	CollectionSphere->SetSphereRadius(CollectionRadius);
	BaseHeight = CrateMesh->GetRelativeLocation().Z;

	// Spread the hover of neighbouring crates so a row of them does not pulse in
	// unison like a single object.
	HoverPhase = FMath::FRandRange(0.0f, 2.0f * PI);
}

void AFRAmmoPickup::Configure(EFRAmmoType InAmmoType, int32 InAmount)
{
	AmmoType = InAmmoType;
	AmmoAmount = FMath::Max(1, InAmount);
}

FLinearColor AFRAmmoPickup::GetBandColor() const
{
	switch (AmmoType)
	{
	case EFRAmmoType::Shell:  return FLinearColor(0.85f, 0.35f, 0.08f);
	case EFRAmmoType::Rifle:  return FLinearColor(0.25f, 0.65f, 0.95f);
	case EFRAmmoType::Pistol:
	default:                  return FLinearColor(0.95f, 0.80f, 0.15f);
	}
}

void AFRAmmoPickup::BuildPickupMesh()
{
	FRVisual::BuildMesh(CrateMesh, EFRBasicShape::Cube, FLinearColor(0.12f, 0.14f, 0.11f), false);
	CrateMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 22.0f));
	CrateMesh->SetRelativeScale3D(FRVisual::SizeToScale(FVector(38.0f, 26.0f, 24.0f)));

	// The band is a child of the crate, so its scale is expressed as a fraction
	// of the crate rather than in centimetres.
	FRVisual::BuildMesh(BandMesh, EFRBasicShape::Cube, GetBandColor(), false);
	BandMesh->SetRelativeScale3D(FVector(1.04f, 1.06f, 0.22f));
	BandMesh->SetRelativeLocation(FVector::ZeroVector);
}

void AFRAmmoPickup::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!bAvailable)
	{
		return;
	}

	// Spinning and hovering is how a pickup says "I am here and I am not scenery".
	HoverPhase += DeltaSeconds * 2.0f;

	FVector Location = CrateMesh->GetRelativeLocation();
	Location.Z = BaseHeight + FMath::Sin(HoverPhase) * HoverAmplitude;
	CrateMesh->SetRelativeLocation(Location);

	CrateMesh->AddLocalRotation(FRotator(0.0f, SpinSpeed * DeltaSeconds, 0.0f));
}

void AFRAmmoPickup::HandleBeginOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComponent,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	if (!bAvailable)
	{
		return;
	}

	AFRCharacter* Character = Cast<AFRCharacter>(OtherActor);
	if (!Character)
	{
		return;
	}

	// AddReserveAmmo returns what was actually taken. A player who is already
	// carrying the maximum takes nothing, and the crate stays where it is for
	// when they come back needing it.
	const int32 Collected = Character->AddReserveAmmo(AmmoType, AmmoAmount);
	if (Collected <= 0)
	{
		return;
	}

	UE_LOG(LogFiringRange, Verbose, TEXT("Picked up %d rounds of %s."), Collected, *FRTypes::GetAmmoTypeName(AmmoType));

	if (CollectSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, CollectSound, GetActorLocation());
	}

	ConsumePickup();
}

void AFRAmmoPickup::ConsumePickup()
{
	bAvailable = false;

	SetActorHiddenInGame(true);
	CollectionSphere->SetGenerateOverlapEvents(false);

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(RespawnTimerHandle, this, &AFRAmmoPickup::RestorePickup, RespawnDelay, false);
	}
}

void AFRAmmoPickup::RestorePickup()
{
	bAvailable = true;

	SetActorHiddenInGame(false);
	CollectionSphere->SetGenerateOverlapEvents(true);
}

void AFRAmmoPickup::ResetPickup()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(RespawnTimerHandle);
	}

	RestorePickup();
}
