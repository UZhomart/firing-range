// Copyright zutemiss & dshadykh. Educational project.

#include "Weapons/FRPistol.h"

#include "Components/StaticMeshComponent.h"

#include "Core/FRVisualUtils.h"

AFRPistol::AFRPistol()
{
	DisplayName = TEXT("M9 Sidearm");
	AmmoType = EFRAmmoType::Pistol;

	// Ballistics: a fast bullet that still visibly travels across the range.
	MuzzleSpeed = 17000.0f;
	DamagePerProjectile = 34.0f;

	// Fire: semi automatic, one round per trigger pull.
	FireInterval = 0.17f;
	bAutomatic = false;
	ProjectilesPerShot = 1;
	HipSpreadDegrees = 1.15f;
	AimSpreadDegrees = 0.10f;
	MovementSpreadPenalty = 1.7f;

	// Magazine: fifteen rounds and a single motion reload.
	MagazineSize = 15;
	ReloadDuration = 1.55f;
	bReloadsOneRoundAtATime = false;

	// Optics: iron sights, so aiming steadies the weapon more than it magnifies.
	AimFieldOfView = 64.0f;

	// Recoil: noticeable but easy to control on a steady trigger finger.
	RecoilPitchMin = 0.55f;
	RecoilPitchMax = 1.05f;
	RecoilYawMax = 0.28f;
	RecoilRiseSpeed = 32.0f;
	RecoilRecoverySpeed = 18.0f;
	RecoilRecoveryRatio = 0.85f;

	ViewKickBack = 2.6f;
	ViewKickPitch = 5.0f;

	HipLocation = FVector(21.0f, 10.0f, -9.5f);
	AimLocation = FVector(25.0f, 0.0f, -4.4f);
}

void AFRPistol::BuildWeaponMesh()
{
	const FLinearColor Slide(0.13f, 0.14f, 0.16f);
	const FLinearColor Frame(0.07f, 0.07f, 0.08f);

	// Slide: short and boxy, sitting right above the hand.
	FRVisual::BuildMesh(BodyMesh, EFRBasicShape::Cube, Slide, false);
	BodyMesh->SetRelativeLocation(FVector(9.0f, 0.0f, 0.0f));
	BodyMesh->SetRelativeScale3D(FRVisual::SizeToScale(FVector(17.0f, 3.6f, 5.4f)));

	// Barrel: barely protrudes past the slide, as on a compact sidearm.
	FRVisual::BuildMesh(BarrelMesh, EFRBasicShape::Cylinder, Slide, false);
	BarrelMesh->SetRelativeLocation(FVector(19.0f, 0.0f, 0.4f));
	BarrelMesh->SetRelativeRotation(FRotator(90.0f, 0.0f, 0.0f));
	BarrelMesh->SetRelativeScale3D(FRVisual::SizeToScale(FVector(2.0f, 2.0f, 5.0f)));

	// Grip: raked back, the angle that reads as a pistol at a glance.
	FRVisual::BuildMesh(GripMesh, EFRBasicShape::Cube, Frame, false);
	GripMesh->SetRelativeLocation(FVector(5.0f, 0.0f, -6.0f));
	GripMesh->SetRelativeRotation(FRotator(-15.0f, 0.0f, 0.0f));
	GripMesh->SetRelativeScale3D(FRVisual::SizeToScale(FVector(4.6f, 3.2f, 10.5f)));

	MuzzlePoint->SetRelativeLocation(FVector(22.0f, 0.0f, 0.4f));
}
