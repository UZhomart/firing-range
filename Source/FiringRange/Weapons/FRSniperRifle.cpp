// Copyright zutemiss & dshadykh. Educational project.

#include "Weapons/FRSniperRifle.h"

#include "Components/StaticMeshComponent.h"

#include "Core/FRVisualUtils.h"

AFRSniperRifle::AFRSniperRifle()
{
	DisplayName = TEXT("7.62 Bolt Action");
	AmmoType = EFRAmmoType::Rifle;

	ScopeMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ScopeMesh"));
	ScopeMesh->SetupAttachment(WeaponRoot);
	ScopeMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ScopeMesh->SetCastShadow(false);

	// Ballistics: the fastest and flattest round on the range. A high muzzle
	// speed shortens the flight time, which shrinks the gravity drop the aiming
	// solver has to compensate for, and that is what makes the rifle feel exact.
	MuzzleSpeed = 32000.0f;
	DamagePerProjectile = 90.0f;

	// Fire: one shot, then a full bolt cycle before the next one.
	FireInterval = 1.35f;
	bAutomatic = false;
	ProjectilesPerShot = 1;
	HipSpreadDegrees = 4.5f;
	AimSpreadDegrees = 0.02f;
	MovementSpreadPenalty = 4.0f;

	// Magazine: five rounds, slow to change.
	MagazineSize = 5;
	ReloadDuration = 2.9f;
	bReloadsOneRoundAtATime = false;

	// Optics: real magnification rather than a steadier hold.
	AimFieldOfView = 22.0f;

	// Recoil: a hard single kick that is almost entirely given back, because a
	// bolt action gives the player time to settle between shots anyway.
	RecoilPitchMin = 2.8f;
	RecoilPitchMax = 3.6f;
	RecoilYawMax = 0.4f;
	RecoilRiseSpeed = 55.0f;
	RecoilRecoveryDelay = 0.3f;
	RecoilRecoverySpeed = 14.0f;
	RecoilRecoveryRatio = 0.95f;

	ViewKickBack = 8.0f;
	ViewKickPitch = 9.0f;

	HipLocation = FVector(19.0f, 12.0f, -12.0f);

	// The scope has to sit exactly on the centre line of the screen, otherwise
	// the magnified view would not agree with the crosshair.
	AimLocation = FVector(20.0f, 0.0f, -7.4f);
	AimInterpSpeed = 9.0f;

	ReloadDipDistance = 13.0f;
	ReloadRollAngle = 22.0f;
}

bool AFRSniperRifle::IsScoped() const
{
	return AimAlpha >= ScopedAlphaThreshold;
}

void AFRSniperRifle::BuildWeaponMesh()
{
	const FLinearColor Steel(0.10f, 0.11f, 0.12f);
	const FLinearColor Stock(0.16f, 0.17f, 0.13f);

	// Long stock and receiver.
	FRVisual::BuildMesh(BodyMesh, EFRBasicShape::Cube, Stock, false);
	BodyMesh->SetRelativeLocation(FVector(4.0f, 0.0f, -1.5f));
	BodyMesh->SetRelativeScale3D(FRVisual::SizeToScale(FVector(40.0f, 4.6f, 7.5f)));

	// A very long, thin barrel.
	FRVisual::BuildMesh(BarrelMesh, EFRBasicShape::Cylinder, Steel, false);
	BarrelMesh->SetRelativeLocation(FVector(46.0f, 0.0f, 0.8f));
	BarrelMesh->SetRelativeRotation(FRotator(90.0f, 0.0f, 0.0f));
	BarrelMesh->SetRelativeScale3D(FRVisual::SizeToScale(FVector(2.6f, 2.6f, 52.0f)));

	// Pistol grip under the receiver.
	FRVisual::BuildMesh(GripMesh, EFRBasicShape::Cube, Stock, false);
	GripMesh->SetRelativeLocation(FVector(6.0f, 0.0f, -6.5f));
	GripMesh->SetRelativeRotation(FRotator(-14.0f, 0.0f, 0.0f));
	GripMesh->SetRelativeScale3D(FRVisual::SizeToScale(FVector(4.4f, 3.4f, 9.0f)));

	// Scope tube sitting above the receiver, on the centre line of the view.
	FRVisual::BuildMesh(ScopeMesh, EFRBasicShape::Cylinder, Steel, false);
	ScopeMesh->SetRelativeLocation(FVector(14.0f, 0.0f, 5.2f));
	ScopeMesh->SetRelativeRotation(FRotator(90.0f, 0.0f, 0.0f));
	ScopeMesh->SetRelativeScale3D(FRVisual::SizeToScale(FVector(4.2f, 4.2f, 22.0f)));

	MuzzlePoint->SetRelativeLocation(FVector(72.0f, 0.0f, 0.8f));
}
