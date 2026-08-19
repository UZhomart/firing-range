// Copyright zutemiss & dshadykh. Educational project.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "FRImpactEffect.generated.h"

class UMaterialInstanceDynamic;
class USoundBase;
class UStaticMeshComponent;

/**
 * Short lived actor that makes a bullet impact readable without any imported art.
 *
 * A Niagara system or a decal material would normally do this job, but both are
 * binary assets. Instead the effect is built from two engine primitives that are
 * animated in Tick: a flash that expands and darkens, and a flat scorch disc
 * aligned to the surface normal. The actor destroys itself when the animation
 * ends, so nothing has to track it.
 */
UCLASS()
class FIRINGRANGE_API AFRImpactEffect : public AActor
{
	GENERATED_BODY()

public:
	AFRImpactEffect();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/**
	 * Spawns the visual part of an impact and plays the sound that goes with it.
	 *
	 * @param WorldContext  any object that belongs to the world
	 * @param Location      world space impact point
	 * @param Normal        surface normal reported by the hit result
	 * @param Color         tint, usually derived from the hit zone
	 * @param ImpactSound   optional, silently skipped when null
	 * @param SizeScale     multiplier over the default flash size
	 */
	static void PlayImpactFeedback(
		const UObject* WorldContext,
		const FVector& Location,
		const FVector& Normal,
		const FLinearColor& Color,
		USoundBase* ImpactSound = nullptr,
		float SizeScale = 1.0f);

	/** Configures the colour and size before the animation starts. */
	void Configure(const FLinearColor& InColor, float InSizeScale);

protected:
	/** Bright sphere that expands during the first frames of the impact. */
	UPROPERTY(VisibleAnywhere, Category = "Firing Range|Impact")
	TObjectPtr<UStaticMeshComponent> FlashMesh;

	/** Flat disc laid onto the surface, standing in for a bullet hole decal. */
	UPROPERTY(VisibleAnywhere, Category = "Firing Range|Impact")
	TObjectPtr<UStaticMeshComponent> ScorchMesh;

	/** How long the whole effect lasts, in seconds. */
	UPROPERTY(EditDefaultsOnly, Category = "Firing Range|Impact")
	float Lifetime = 0.35f;

	/** Radius of the flash at the end of its expansion, in centimetres. */
	UPROPERTY(EditDefaultsOnly, Category = "Firing Range|Impact")
	float FlashEndRadius = 26.0f;

	/** Radius of the scorch disc, in centimetres. */
	UPROPERTY(EditDefaultsOnly, Category = "Firing Range|Impact")
	float ScorchRadius = 9.0f;

private:
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> FlashMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> ScorchMaterial;

	FLinearColor EffectColor = FLinearColor::White;
	float SizeScale = 1.0f;
	float Elapsed = 0.0f;
};
