// Copyright zutemiss & dshadykh. Educational project.

#include "Weapons/FRShotgun.h"

#include "Components/StaticMeshComponent.h"

#include "Core/FRVisualUtils.h"

AFRShotgun::AFRShotgun()
{
	DisplayName = TEXT("12 Gauge Pump");
	AmmoType = EFRAmmoType::Shell;

	// Ballistics: heavy, slow buckshot that loses the duel at long range.
	MuzzleSpeed = 11000.0f;
	DamagePerProjectile = 16.0f;

	// Fire: one trigger pull throws a whole cone of pellets.
	FireInterval = 0.85f;
	bAutomatic = false;
	ProjectilesPerShot = 8;
	HipSpreadDegrees = 4.2f;
	AimSpreadDegrees = 2.6f;
	MovementSpreadPenalty = 1.2f;

	// Magazine: a short tube refilled shell by shell.
	MagazineSize = 6;
	bReloadsOneRoundAtATime = true;
	SingleRoundReloadDuration = 0.42f;
	ReloadDuration = 0.42f;

	// Optics: a bead sight tightens the cone a little, it does not magnify.
	AimFieldOfView = 76.0f;

	// Recoil: the hardest kick of the three, and only partly given back.
	RecoilPitchMin = 2.4f;
	RecoilPitchMax = 3.4f;
	RecoilYawMax = 0.9f;
	RecoilRiseSpeed = 48.0f;
	RecoilRecoverySpeed = 20.0f;
	RecoilRecoveryRatio = 0.6f;

	ViewKickBack = 7.5f;
	ViewKickPitch = 11.0f;

	HipLocation = FVector(20.0f, 11.5f, -11.0f);
	AimLocation = FVector(24.0f, 0.0f, -5.2f);
	ReloadDipDistance = 11.0f;
	ReloadRollAngle = 18.0f;
}

void AFRShotgun::BuildWeaponMesh()
{
	const FLinearColor Steel(0.15f, 0.15f, 0.17f);
	const FLinearColor Wood(0.22f, 0.13f, 0.07f);

	// Receiver and stock in one long block, the silhouette of a long gun.
	FRVisual::BuildMesh(BodyMesh, EFRBasicShape::Cube, Wood, false);
	BodyMesh->SetRelativeLocation(FVector(6.0f, 0.0f, -1.0f));
	BodyMesh->SetRelativeScale3D(FRVisual::SizeToScale(FVector(34.0f, 5.0f, 7.0f)));

	// A long barrel reaching well past the receiver.
	FRVisual::BuildMesh(BarrelMesh, EFRBasicShape::Cylinder, Steel, false);
	BarrelMesh->SetRelativeLocation(FVector(38.0f, 0.0f, 1.0f));
	BarrelMesh->SetRelativeRotation(FRotator(90.0f, 0.0f, 0.0f));
	BarrelMesh->SetRelativeScale3D(FRVisual::SizeToScale(FVector(3.4f, 3.4f, 40.0f)));

	// The pump under the barrel, the detail that names the weapon.
	FRVisual::BuildMesh(GripMesh, EFRBasicShape::Cube, Wood, false);
	GripMesh->SetRelativeLocation(FVector(30.0f, 0.0f, -3.4f));
	GripMesh->SetRelativeScale3D(FRVisual::SizeToScale(FVector(13.0f, 4.4f, 4.2f)));

	MuzzlePoint->SetRelativeLocation(FVector(59.0f, 0.0f, 1.0f));
}

void AFRShotgun::HandleFireDenied()
{
	// Shell by shell reloading has to be interruptible, otherwise a player who
	// started topping up with one shell left in the tube would be helpless for
	// several seconds. Pulling the trigger aborts the reload and fires what is
	// already loaded.
	if (bReloading && AmmoInMagazine > 0)
	{
		CancelReload();
		TryFire();
		return;
	}

	Super::HandleFireDenied();
}
