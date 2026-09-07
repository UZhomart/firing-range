// Copyright zutemiss & dshadykh. Educational project.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "Core/FRTypes.h"
#include "Core/FRVisualUtils.h"

#include "FRRangeBuilder.generated.h"

class AFRMovingTarget;
class AFRPatrolPath;
class AFRStationaryTarget;
class APlayerStart;
class UStaticMeshComponent;

/**
 * Builds the whole firing range in code.
 *
 * A level in Unreal is a .umap, which is a binary asset. This project commits
 * source only, so the two maps it ships with are empty and everything inside
 * them is created here: the ground, the earth berms, the canopy over the firing
 * line, the lane dividers, both target sections, the ammunition crates, the
 * lighting and even the player start.
 *
 * The range is deliberately outdoors. A sun, a sky atmosphere and a sky light
 * give correct, good looking lighting with no imported content at all, which an
 * indoor room would need light fixtures and a captured cubemap for.
 *
 * The game mode spawns this actor during InitGame, before the first player is
 * given a pawn, so the ground and the player start are already there when the
 * character appears.
 */
UCLASS()
class FIRINGRANGE_API AFRRangeBuilder : public AActor
{
	GENERATED_BODY()

public:
	AFRRangeBuilder();

	virtual void BeginPlay() override;

	/** Creates the range. Safe to call twice: the second call does nothing. */
	void BuildRange();

	/** Where the player starts, once the range exists. */
	FTransform GetPlayerSpawnTransform() const;

	/** Targets created by this builder, handed to the game mode for registration. */
	const TArray<TObjectPtr<AActor>>& GetSpawnedTargets() const { return SpawnedTargets; }

protected:
	// -- Sections of the range -------------------------------------------------

	void BuildGround();
	void BuildBerms();
	void BuildFiringLine();
	void BuildLaneDividers();
	void BuildDistanceMarkers();
	void BuildStationarySection();
	void BuildMovingSection();
	void BuildAmmoCrates();
	void BuildLighting();

	// -- Primitive helpers -----------------------------------------------------

	/**
	 * Adds one piece of scenery.
	 *
	 * Components created after the game has started must be movable: an actor
	 * cannot gain static geometry once the level is running, so mobility is set
	 * before the component is registered.
	 */
	UStaticMeshComponent* AddBlock(
		const FVector& Location,
		const FVector& Size,
		const FLinearColor& Color,
		const FRotator& Rotation = FRotator::ZeroRotator,
		bool bCollides = true,
		EFRBasicShape Shape = EFRBasicShape::Cube);

	// -- Layout ----------------------------------------------------------------
	// Every distance is in centimetres, with the player looking down positive X.

	/** Distance from the firing line to the far berm. */
	UPROPERTY(EditAnywhere, Category = "Firing Range|Layout")
	float RangeLength = 7000.0f;

	/** Half width of the range, from the centre line to a side berm. */
	UPROPERTY(EditAnywhere, Category = "Firing Range|Layout")
	float RangeHalfWidth = 2000.0f;

	/** How far behind the firing line the player spawns. */
	UPROPERTY(EditAnywhere, Category = "Firing Range|Layout")
	float PlayerSetback = 420.0f;

	/** Distances down range the stationary boards are placed at. */
	UPROPERTY(EditAnywhere, Category = "Firing Range|Layout")
	TArray<float> StationaryDistances;

	/** Distances down range the patrol lanes are placed at. */
	UPROPERTY(EditAnywhere, Category = "Firing Range|Layout")
	TArray<float> MovingDistances;

	// -- Palette ---------------------------------------------------------------

	UPROPERTY(EditAnywhere, Category = "Firing Range|Palette")
	FLinearColor GroundColor = FLinearColor(0.09f, 0.10f, 0.08f);

	UPROPERTY(EditAnywhere, Category = "Firing Range|Palette")
	FLinearColor BermColor = FLinearColor(0.16f, 0.13f, 0.09f);

	UPROPERTY(EditAnywhere, Category = "Firing Range|Palette")
	FLinearColor ConcreteColor = FLinearColor(0.23f, 0.23f, 0.22f);

	UPROPERTY(EditAnywhere, Category = "Firing Range|Palette")
	FLinearColor TimberColor = FLinearColor(0.20f, 0.13f, 0.07f);

	UPROPERTY(EditAnywhere, Category = "Firing Range|Palette")
	FLinearColor StaticSectionColor = FLinearColor(0.62f, 0.10f, 0.08f);

	UPROPERTY(EditAnywhere, Category = "Firing Range|Palette")
	FLinearColor MovingSectionColor = FLinearColor(0.09f, 0.32f, 0.52f);

private:
	/** Root every generated component hangs from. */
	UPROPERTY(VisibleAnywhere, Category = "Firing Range|Layout")
	TObjectPtr<USceneComponent> BuilderRoot;

	/** Guard so the range is only ever built once. */
	bool bRangeBuilt = false;

	/** Player start created together with the ground. */
	UPROPERTY(Transient)
	TObjectPtr<APlayerStart> GeneratedPlayerStart;

	/** Every target this builder created. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<AActor>> SpawnedTargets;
};
