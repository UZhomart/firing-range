// Copyright zutemiss & dshadykh. Educational project.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "Core/FRTypes.h"

#include "FRWeaponBase.generated.h"

class AFRCharacter;
class AFRProjectile;
class USoundBase;
class UStaticMeshComponent;

/** Raised once per trigger pull, carrying how many projectiles left the muzzle. */
DECLARE_MULTICAST_DELEGATE_OneParam(FFROnShotsFired, int32 /*ProjectileCount*/);

/** Raised whenever ammunition or reload state changed, so the HUD can refresh. */
DECLARE_MULTICAST_DELEGATE(FFROnWeaponStateChanged);

/**
 * Base class of every firearm on the range.
 *
 * The class owns the full weapon state machine: rate of fire, magazine, reload,
 * spread and the ballistic aiming solution. Subclasses only change data and the
 * shape of the view model; a pistol, a shotgun and a sniper rifle share all of
 * the logic below.
 *
 * Two requirements of the brief pull in opposite directions here. The crosshair
 * has to mark the exact point of impact, which argues for shooting straight from
 * the camera, while the bullets have to be real projectiles affected by gravity,
 * which makes them drop below that point. ComputeLaunchVelocity resolves the
 * conflict by aiming the muzzle slightly above the crosshair, by exactly the
 * amount the bullet is going to fall on the way there.
 */
UCLASS(Abstract)
class FIRINGRANGE_API AFRWeaponBase : public AActor
{
	GENERATED_BODY()

public:
	AFRWeaponBase();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	// -- Equipment ------------------------------------------------------------

	/** Attaches the weapon to the holder of a character and makes it visible. */
	virtual void OnEquipped(AFRCharacter* NewOwner);

	/** Hides the weapon and stops everything it was doing. */
	virtual void OnUnequipped();

	// -- Trigger --------------------------------------------------------------

	/** Pulls the trigger. Fires immediately when the weapon is ready. */
	void StartFire();

	/** Releases the trigger. Stops an automatic burst. */
	void StopFire();

	/** Begins a reload when one is possible. */
	void StartReload();

	/** Aborts a reload in progress, keeping whatever was already loaded. */
	void CancelReload();

	/** Refills the magazine and clears every timer. Used when a session restarts. */
	void ResetToFullMagazine();

	// -- Queries used by the HUD and by the character --------------------------

	int32 GetAmmoInMagazine() const { return AmmoInMagazine; }
	int32 GetMagazineSize() const { return MagazineSize; }
	EFRAmmoType GetAmmoType() const { return AmmoType; }
	const FString& GetDisplayName() const { return DisplayName; }
	bool IsReloading() const { return bReloading; }
	bool IsMagazineEmpty() const { return AmmoInMagazine <= 0; }
	float GetAimFieldOfView() const { return AimFieldOfView; }

	/** Reload completion between 0 and 1, for the progress bar on the HUD. */
	float GetReloadProgress() const;

	/** True when the rate of fire, the magazine and the reload state all allow a shot. */
	bool CanFire() const;

	/** True when there is room in the magazine and rounds left in the reserve. */
	bool CanReload() const;

	/**
	 * Half angle of the current spread cone in degrees.
	 *
	 * The HUD draws the crosshair gap from this value, which is what makes the
	 * crosshair honest: it opens up exactly when the weapon becomes less precise.
	 */
	float GetCurrentSpreadDegrees() const;

	FFROnShotsFired OnShotsFired;
	FFROnWeaponStateChanged OnWeaponStateChanged;

protected:
	// -- Components -----------------------------------------------------------

	UPROPERTY(VisibleAnywhere, Category = "Firing Range|Weapon")
	TObjectPtr<USceneComponent> WeaponRoot;

	/** Receiver of the firearm. */
	UPROPERTY(VisibleAnywhere, Category = "Firing Range|Weapon")
	TObjectPtr<UStaticMeshComponent> BodyMesh;

	/** Barrel. Its far end defines where the muzzle sits. */
	UPROPERTY(VisibleAnywhere, Category = "Firing Range|Weapon")
	TObjectPtr<UStaticMeshComponent> BarrelMesh;

	UPROPERTY(VisibleAnywhere, Category = "Firing Range|Weapon")
	TObjectPtr<UStaticMeshComponent> GripMesh;

	/** Point bullets are spawned from. */
	UPROPERTY(VisibleAnywhere, Category = "Firing Range|Weapon")
	TObjectPtr<USceneComponent> MuzzlePoint;

	// -- Identity -------------------------------------------------------------

	UPROPERTY(EditDefaultsOnly, Category = "Firing Range|Weapon")
	FString DisplayName = TEXT("Weapon");

	UPROPERTY(EditDefaultsOnly, Category = "Firing Range|Weapon")
	EFRAmmoType AmmoType = EFRAmmoType::Pistol;

	// -- Ballistics -----------------------------------------------------------

	UPROPERTY(EditDefaultsOnly, Category = "Firing Range|Ballistics")
	TSubclassOf<AFRProjectile> ProjectileClass;

	/** Muzzle velocity in centimetres per second. */
	UPROPERTY(EditDefaultsOnly, Category = "Firing Range|Ballistics", meta = (ClampMin = "1000.0"))
	float MuzzleSpeed = 18000.0f;

	/** Damage carried by one projectile. */
	UPROPERTY(EditDefaultsOnly, Category = "Firing Range|Ballistics")
	float DamagePerProjectile = 34.0f;

	/** How far the aiming trace looks for the point the crosshair covers. */
	UPROPERTY(EditDefaultsOnly, Category = "Firing Range|Ballistics")
	float MaxAimDistance = 30000.0f;

	// -- Rate of fire ---------------------------------------------------------

	/** Seconds between two shots. */
	UPROPERTY(EditDefaultsOnly, Category = "Firing Range|Fire", meta = (ClampMin = "0.02"))
	float FireInterval = 0.22f;

	/** When true the weapon keeps firing while the trigger is held. */
	UPROPERTY(EditDefaultsOnly, Category = "Firing Range|Fire")
	bool bAutomatic = false;

	/** Projectiles released by a single trigger pull. Above one for a shotgun. */
	UPROPERTY(EditDefaultsOnly, Category = "Firing Range|Fire", meta = (ClampMin = "1"))
	int32 ProjectilesPerShot = 1;

	/** Spread half angle while firing from the hip, in degrees. */
	UPROPERTY(EditDefaultsOnly, Category = "Firing Range|Fire", meta = (ClampMin = "0.0"))
	float HipSpreadDegrees = 1.1f;

	/** Spread half angle while aiming down sights, in degrees. */
	UPROPERTY(EditDefaultsOnly, Category = "Firing Range|Fire", meta = (ClampMin = "0.0"))
	float AimSpreadDegrees = 0.12f;

	/** Extra spread in degrees added at full running speed. */
	UPROPERTY(EditDefaultsOnly, Category = "Firing Range|Fire", meta = (ClampMin = "0.0"))
	float MovementSpreadPenalty = 1.8f;

	// -- Magazine -------------------------------------------------------------

	UPROPERTY(EditDefaultsOnly, Category = "Firing Range|Magazine", meta = (ClampMin = "1"))
	int32 MagazineSize = 15;

	/** Seconds a full magazine change takes. */
	UPROPERTY(EditDefaultsOnly, Category = "Firing Range|Magazine", meta = (ClampMin = "0.1"))
	float ReloadDuration = 1.8f;

	/**
	 * Shell by shell reloading.
	 *
	 * A shotgun tops its tube up one round at a time and can be interrupted by
	 * firing, which is what makes its reload feel different from a magazine swap.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Firing Range|Magazine")
	bool bReloadsOneRoundAtATime = false;

	/** Seconds each individual round takes when reloading shell by shell. */
	UPROPERTY(EditDefaultsOnly, Category = "Firing Range|Magazine", meta = (ClampMin = "0.05"))
	float SingleRoundReloadDuration = 0.45f;

	/** When true an empty weapon starts reloading by itself. */
	UPROPERTY(EditDefaultsOnly, Category = "Firing Range|Magazine")
	bool bAutoReloadWhenEmpty = true;

	// -- Optics ---------------------------------------------------------------

	/** Camera field of view while aiming. Lower values zoom in. */
	UPROPERTY(EditDefaultsOnly, Category = "Firing Range|Optics", meta = (ClampMin = "10.0", ClampMax = "120.0"))
	float AimFieldOfView = 62.0f;

	// -- Audio ----------------------------------------------------------------
	// Every slot is optional. The project ships without audio assets, so a build
	// that has none is silent rather than broken.

	UPROPERTY(EditDefaultsOnly, Category = "Firing Range|Audio")
	TObjectPtr<USoundBase> FireSound;

	UPROPERTY(EditDefaultsOnly, Category = "Firing Range|Audio")
	TObjectPtr<USoundBase> ReloadSound;

	UPROPERTY(EditDefaultsOnly, Category = "Firing Range|Audio")
	TObjectPtr<USoundBase> EmptySound;

	UPROPERTY(EditDefaultsOnly, Category = "Firing Range|Audio")
	TObjectPtr<USoundBase> ImpactSound;

	// -- Internals ------------------------------------------------------------

	/** Builds the view model out of engine primitives. Overridden per weapon. */
	virtual void BuildWeaponMesh();

	/** One complete trigger pull: spends a round and releases the projectiles. */
	virtual void FireOnce();

	/** Releases ProjectilesPerShot bullets towards the crosshair. */
	virtual void LaunchProjectiles();

	/** Runs when the trigger is pulled but the weapon cannot fire. */
	virtual void HandleFireDenied();

	/** World point the crosshair currently covers. */
	FVector ComputeAimPoint() const;

	/**
	 * Velocity for one bullet, already corrected for gravity drop and randomised
	 * inside the spread cone.
	 */
	FVector ComputeLaunchVelocity(const FVector& MuzzleLocation, const FVector& AimPoint, float SpreadDegrees) const;

	/** Chained by the fire timer while an automatic weapon holds its trigger. */
	void TryFire();

	/** Called when the reload timer elapses. */
	void FinishReload();

	/** Called for each round when reloading shell by shell. */
	void InsertSingleRound();

	/** Plays a sound at the weapon, ignoring null slots. */
	void PlayWeaponSound(USoundBase* Sound) const;

	/** Character currently holding the weapon. Weak, because the pawn can die. */
	TWeakObjectPtr<AFRCharacter> OwningCharacter;

	/** Rounds currently in the magazine. */
	UPROPERTY(Transient, VisibleInstanceOnly, Category = "Firing Range|Magazine")
	int32 AmmoInMagazine = 0;

	bool bTriggerHeld = false;
	bool bReloading = false;

	/** World time of the last shot, used to enforce the rate of fire. */
	float LastFireTime = -1000.0f;

	/** World time the current reload started, used by the HUD progress bar. */
	float ReloadStartTime = 0.0f;

	/** Duration of the reload step currently running. */
	float CurrentReloadStepDuration = 0.0f;

	FTimerHandle FireTimerHandle;
	FTimerHandle ReloadTimerHandle;
};
