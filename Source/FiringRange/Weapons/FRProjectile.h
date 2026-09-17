// Copyright zutemiss & dshadykh. Educational project.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "FRProjectile.generated.h"

class UProjectileMovementComponent;
class USoundBase;
class USphereComponent;
class UStaticMeshComponent;

/**
 * Bullet fired by every weapon of the project.
 *
 * The brief asks for real projectile physics rather than a hitscan trace, so the
 * bullet is a swept sphere driven by UProjectileMovementComponent and pulled down
 * by gravity. The gravity scale is deliberately small: a full 1.0 would drop the
 * bullet far below the crosshair, and the HUD is required to mark the exact point
 * of impact. The weapon compensates for the remaining drop when it aims the shot.
 */
UCLASS()
class FIRINGRANGE_API AFRProjectile : public AActor
{
	GENERATED_BODY()

public:
	AFRProjectile();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/**
	 * Prepares a freshly spawned bullet. Must be called between SpawnActorDeferred
	 * and FinishSpawning, because the movement component reads Velocity while it
	 * is being registered.
	 *
	 * @param InLaunchVelocity  world space velocity in centimetres per second
	 * @param InDamage          damage handed to the actor that is struck
	 * @param InShooter         actor to ignore, so the muzzle never hits its owner
	 * @param InImpactSound     optional sound played where the bullet lands
	 */
	void InitialiseShot(const FVector& InLaunchVelocity, float InDamage, AActor* InShooter, USoundBase* InImpactSound);

	/** Gravity scale of the class default object, read by the ballistic solver. */
	float GetGravityScale() const;

	/** Colour of the tracer, also used by the impact flash. */
	UPROPERTY(EditDefaultsOnly, Category = "Firing Range|Projectile")
	FLinearColor TracerColor = FLinearColor(1.0f, 0.78f, 0.35f);

protected:
	/** Swept collision shape. Root of the actor and the component the movement drives. */
	UPROPERTY(VisibleAnywhere, Category = "Firing Range|Projectile")
	TObjectPtr<USphereComponent> CollisionComponent;

	/** Elongated primitive that reads as a tracer in flight. */
	UPROPERTY(VisibleAnywhere, Category = "Firing Range|Projectile")
	TObjectPtr<UStaticMeshComponent> TracerMesh;

	UPROPERTY(VisibleAnywhere, Category = "Firing Range|Projectile")
	TObjectPtr<UProjectileMovementComponent> ProjectileMovement;

	/** Radius of the bullet in centimetres. Small enough to pass between target rings. */
	UPROPERTY(EditDefaultsOnly, Category = "Firing Range|Projectile")
	float BulletRadius = 2.0f;

	/** Length of the tracer streak in centimetres. */
	UPROPERTY(EditDefaultsOnly, Category = "Firing Range|Projectile")
	float TracerLength = 34.0f;

	/** Damage handed to whatever the bullet hits. Overwritten by the weapon. */
	UPROPERTY(EditDefaultsOnly, Category = "Firing Range|Projectile")
	float Damage = 25.0f;

	/** Seconds a bullet may stay alive before it is cleaned up. */
	UPROPERTY(EditDefaultsOnly, Category = "Firing Range|Projectile")
	float MaxFlightTime = 5.0f;

	/**
	 * Tells the game mode that this bullet is done.
	 *
	 * Reported exactly once, whether the bullet struck something or simply ran
	 * out of flight time, so the shot it belongs to can be closed.
	 */
	void ReportOutcome();

	UFUNCTION()
	void HandleHit(
		UPrimitiveComponent* HitComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComponent,
		FVector NormalImpulse,
		const FHitResult& Hit);

private:
	/** Sound played at the impact point. Optional, assigned by the weapon. */
	UPROPERTY(Transient)
	TObjectPtr<USoundBase> ImpactSound = nullptr;

	/** Guards against reporting the outcome of the same bullet twice. */
	bool bOutcomeReported = false;
};
