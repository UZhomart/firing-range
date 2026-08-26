// Copyright zutemiss & dshadykh. Educational project.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"

#include "Core/FRTypes.h"

#include "FRTargetBase.generated.h"

class UMaterialInstanceDynamic;
class USoundBase;
class UStaticMeshComponent;

class AFRTargetBase;

/** Raised when a standing target is struck, with the zone and the range in metres. */
DECLARE_MULTICAST_DELEGATE_ThreeParams(FFROnTargetHit, AFRTargetBase* /*Target*/, EFRHitZone /*Zone*/, float /*DistanceMetres*/);

/** Life cycle of a target between being shot and standing up again. */
UENUM()
enum class EFRTargetState : uint8
{
	/** Upright and able to be hit. */
	Standing,
	/** Rotating backwards after being hit. */
	FallingDown,
	/** Flat, waiting for the respawn timer. */
	Down,
	/** Rotating back up into the standing pose. */
	RisingUp
};

/**
 * Base class of every target on the range.
 *
 * It is a pawn rather than a plain actor for one reason: the moving targets are
 * driven by an AAIController, and a controller in Unreal can only possess a
 * pawn. The stationary targets simply never get a controller.
 *
 * The board carries three scoring zones. The head is a separate component, so it
 * is recognised by the component that was struck. The rings on the board are not
 * separate geometry - they are resolved from how far the impact landed from the
 * centre of the board, measured in the plane of the board. That keeps the zones
 * exact under any non uniform scale, which a local space comparison would not.
 */
UCLASS(Abstract)
class FIRINGRANGE_API AFRTargetBase : public APawn
{
	GENERATED_BODY()

public:
	AFRTargetBase();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	virtual float TakeDamage(
		float DamageAmount,
		FDamageEvent const& DamageEvent,
		AController* EventInstigator,
		AActor* DamageCauser) override;

	/** True while the target is upright and worth shooting at. */
	bool IsStanding() const { return State == EFRTargetState::Standing; }

	/** Puts the target back upright at once, cancelling any pending respawn. */
	virtual void ResetTarget();

	/** Applies the difficulty chosen in the settings menu. */
	virtual void SetDifficulty(EFRDifficulty NewDifficulty);

	FFROnTargetHit OnTargetHit;

protected:
	// -- Components -----------------------------------------------------------

	/** Actor root, sitting at ground level. */
	UPROPERTY(VisibleAnywhere, Category = "Firing Range|Target")
	TObjectPtr<USceneComponent> TargetRoot;

	/**
	 * Everything that falls over is parented here.
	 *
	 * The pivot sits at the top of the post, so tipping it rotates the board
	 * around its mount the way a real pop up target hinges.
	 */
	UPROPERTY(VisibleAnywhere, Category = "Firing Range|Target")
	TObjectPtr<USceneComponent> PivotRoot;

	/** Round board the rings are scored on. */
	UPROPERTY(VisibleAnywhere, Category = "Firing Range|Target")
	TObjectPtr<UStaticMeshComponent> BoardMesh;

	/** Smaller sphere above the board that scores as a headshot. */
	UPROPERTY(VisibleAnywhere, Category = "Firing Range|Target")
	TObjectPtr<UStaticMeshComponent> HeadMesh;

	/** Post the board stands on. Does not score. */
	UPROPERTY(VisibleAnywhere, Category = "Firing Range|Target")
	TObjectPtr<UStaticMeshComponent> PostMesh;

	/**
	 * Painted inner ring and bullseye.
	 *
	 * Both are purely decorative and carry no collision at all. A bullet flies
	 * straight through them and lands on the board behind, which is what keeps
	 * the scoring geometry and the painted geometry from disagreeing.
	 */
	UPROPERTY(VisibleAnywhere, Category = "Firing Range|Target")
	TObjectPtr<UStaticMeshComponent> InnerRingMesh;

	UPROPERTY(VisibleAnywhere, Category = "Firing Range|Target")
	TObjectPtr<UStaticMeshComponent> BullseyeMesh;

	// -- Scoring geometry -----------------------------------------------------

	/** Radius of the board in centimetres. */
	UPROPERTY(EditAnywhere, Category = "Firing Range|Target", meta = (ClampMin = "5.0"))
	float BoardRadius = 34.0f;

	/** Impacts closer than this to the centre count as a bullseye. */
	UPROPERTY(EditAnywhere, Category = "Firing Range|Target", meta = (ClampMin = "1.0"))
	float BullseyeRadius = 9.0f;

	/** Impacts closer than this, but outside the bullseye, count as the inner ring. */
	UPROPERTY(EditAnywhere, Category = "Firing Range|Target", meta = (ClampMin = "1.0"))
	float InnerRingRadius = 20.0f;

	/** Radius of the head sphere in centimetres. */
	UPROPERTY(EditAnywhere, Category = "Firing Range|Target", meta = (ClampMin = "2.0"))
	float HeadRadius = 13.0f;

	/** Height of the post the board is mounted on. */
	UPROPERTY(EditAnywhere, Category = "Firing Range|Target", meta = (ClampMin = "0.0"))
	float PostHeight = 90.0f;

	// -- Timing ---------------------------------------------------------------

	/** Seconds a target stays flat before standing back up. */
	UPROPERTY(EditAnywhere, Category = "Firing Range|Target", meta = (ClampMin = "0.1"))
	float RespawnDelay = 2.6f;

	/** Seconds the falling and the rising animation each take. */
	UPROPERTY(EditAnywhere, Category = "Firing Range|Target", meta = (ClampMin = "0.05"))
	float TipDuration = 0.22f;

	/** Angle the target is tipped back by when it goes down. */
	UPROPERTY(EditAnywhere, Category = "Firing Range|Target", meta = (ClampMin = "10.0", ClampMax = "120.0"))
	float TipAngle = 88.0f;

	// -- Presentation ---------------------------------------------------------

	UPROPERTY(EditAnywhere, Category = "Firing Range|Target")
	FLinearColor BoardColor = FLinearColor(0.85f, 0.86f, 0.88f);

	UPROPERTY(EditAnywhere, Category = "Firing Range|Target")
	FLinearColor RingColor = FLinearColor(0.80f, 0.12f, 0.10f);

	UPROPERTY(EditAnywhere, Category = "Firing Range|Target")
	FLinearColor HeadColor = FLinearColor(0.78f, 0.80f, 0.84f);

	/** Optional sound played where the board was struck. */
	UPROPERTY(EditAnywhere, Category = "Firing Range|Target")
	TObjectPtr<USoundBase> HitSound;

	// -- Internals ------------------------------------------------------------

	/** Builds the board, head and post out of engine primitives. */
	virtual void BuildTargetMesh();

	/** Works out which scoring zone an impact belongs to. */
	EFRHitZone ResolveHitZone(const FHitResult& Hit) const;

	/** Starts the knockdown and schedules the respawn. */
	virtual void HandleKnockedDown(EFRHitZone Zone, const FVector& ImpactPoint);

	/** Starts the rising animation. Called by the respawn timer. */
	virtual void HandleRespawn();

	/** Enables or disables every scoring collider in one call. */
	void SetScoringCollisionEnabled(bool bEnabled);

	/** Advances the falling and rising animation. */
	void UpdateTipAnimation(float DeltaSeconds);

	/** Difficulty multiplier applied to the respawn delay and to movement speed. */
	float DifficultyScale = 1.0f;

	EFRTargetState State = EFRTargetState::Standing;

	/** Progress of the current tip animation, 0 upright and 1 flat. */
	float TipAlpha = 0.0f;

	FTimerHandle RespawnTimerHandle;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> BoardMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> HeadMaterial;
};
