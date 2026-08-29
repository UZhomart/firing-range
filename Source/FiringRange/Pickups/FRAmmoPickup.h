// Copyright zutemiss & dshadykh. Educational project.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "Core/FRTypes.h"

#include "FRAmmoPickup.generated.h"

class USoundBase;
class USphereComponent;
class UStaticMeshComponent;

/**
 * Crate of ammunition the player restocks from.
 *
 * Collection happens on overlap, and it only happens when the rounds actually
 * fit. Walking over a crate with a full pouch leaves it standing, so a player
 * cannot waste the restock they will need two magazines later.
 *
 * A collected crate does not disappear for good: it hides itself, waits, and
 * comes back, which keeps a practice session running indefinitely.
 */
UCLASS()
class FIRINGRANGE_API AFRAmmoPickup : public AActor
{
	GENERATED_BODY()

public:
	AFRAmmoPickup();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** Sets what the crate holds. Used by the range builder. */
	void Configure(EFRAmmoType InAmmoType, int32 InAmount);

	/** Makes the crate available again at once. Used when a session restarts. */
	void ResetPickup();

protected:
	UPROPERTY(VisibleAnywhere, Category = "Firing Range|Pickup")
	TObjectPtr<USceneComponent> PickupRoot;

	/** Overlap volume that detects the player walking into the crate. */
	UPROPERTY(VisibleAnywhere, Category = "Firing Range|Pickup")
	TObjectPtr<USphereComponent> CollectionSphere;

	/** Body of the crate. */
	UPROPERTY(VisibleAnywhere, Category = "Firing Range|Pickup")
	TObjectPtr<UStaticMeshComponent> CrateMesh;

	/** Coloured band that tells the ammunition family apart from a distance. */
	UPROPERTY(VisibleAnywhere, Category = "Firing Range|Pickup")
	TObjectPtr<UStaticMeshComponent> BandMesh;

	/** Family of ammunition handed out by this crate. */
	UPROPERTY(EditAnywhere, Category = "Firing Range|Pickup")
	EFRAmmoType AmmoType = EFRAmmoType::Pistol;

	/** Rounds handed out on collection. */
	UPROPERTY(EditAnywhere, Category = "Firing Range|Pickup", meta = (ClampMin = "1"))
	int32 AmmoAmount = 30;

	/** Seconds before a collected crate returns. */
	UPROPERTY(EditAnywhere, Category = "Firing Range|Pickup", meta = (ClampMin = "0.5"))
	float RespawnDelay = 12.0f;

	/** Radius of the overlap volume, in centimetres. */
	UPROPERTY(EditAnywhere, Category = "Firing Range|Pickup", meta = (ClampMin = "20.0"))
	float CollectionRadius = 95.0f;

	/** Degrees per second the crate spins while it is available. */
	UPROPERTY(EditAnywhere, Category = "Firing Range|Pickup")
	float SpinSpeed = 45.0f;

	/** Height of the hovering motion, in centimetres. */
	UPROPERTY(EditAnywhere, Category = "Firing Range|Pickup")
	float HoverAmplitude = 5.0f;

	UPROPERTY(EditAnywhere, Category = "Firing Range|Pickup")
	TObjectPtr<USoundBase> CollectSound;

	UFUNCTION()
	void HandleBeginOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComponent,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	/** Hides the crate and schedules its return. */
	void ConsumePickup();

	/** Shows the crate again. Called by the respawn timer. */
	void RestorePickup();

	/** Builds the crate out of engine primitives and tints it by ammunition family. */
	void BuildPickupMesh();

	/** Colour associated with the ammunition family of this crate. */
	FLinearColor GetBandColor() const;

private:
	bool bAvailable = true;
	float HoverPhase = 0.0f;
	float BaseHeight = 0.0f;

	FTimerHandle RespawnTimerHandle;
};
