// Copyright zutemiss & dshadykh. Educational project.

#pragma once

#include "CoreMinimal.h"
#include "Targets/FRTargetBase.h"

#include "FRMovingTarget.generated.h"

class AFRPatrolPath;
class UFloatingPawnMovement;

/**
 * Target that travels along a patrol path under the control of an AI controller.
 *
 * The pawn deliberately holds no decision making of its own. It exposes how fast
 * it may move and which pattern it was configured with, and it offers a single
 * MoveTowards call. Everything about where to go next lives in the controller,
 * which is the split the engine expects: the pawn is the body, the controller is
 * the brain.
 *
 * Movement is driven by a UFloatingPawnMovement rather than by writing the actor
 * location directly, so the target accelerates and decelerates instead of
 * teleporting, and so it remains a proper pawn as far as the engine is concerned.
 */
UCLASS()
class FIRINGRANGE_API AFRMovingTarget : public AFRTargetBase
{
	GENERATED_BODY()

public:
	AFRMovingTarget();

	virtual void BeginPlay() override;
	virtual void ResetTarget() override;

	/** Route the target follows, or nullptr when it was never assigned one. */
	AFRPatrolPath* GetPatrolPath() const { return PatrolPath; }

	/** Assigns the route. Used by the range builder when it lays out a lane. */
	void SetPatrolPath(AFRPatrolPath* NewPath) { PatrolPath = NewPath; }

	/** Pattern the controller should execute. */
	EFRTargetMotion GetMotionType() const { return MotionType; }

	void SetMotionType(EFRTargetMotion NewMotion) { MotionType = NewMotion; }

	/** Travel speed in centimetres per second, already scaled by difficulty. */
	float GetMoveSpeed() const;

	/** Seconds the target waits at the end of an open route before turning around. */
	float GetPauseDuration() const;

	/** Radius used by the orbit pattern, in centimetres. */
	float GetOrbitRadius() const { return OrbitRadius; }

	/** Steers the pawn towards a world location for one frame. */
	void MoveTowards(const FVector& TargetLocation);

	/** Stops the pawn where it is. Called while the target is down. */
	void StopMoving();

protected:
	/** Gives the pawn velocity without needing a navigation mesh. */
	UPROPERTY(VisibleAnywhere, Category = "Firing Range|Movement")
	TObjectPtr<UFloatingPawnMovement> MovementComponent;

	/** Route assigned in the level or by the range builder. */
	UPROPERTY(EditInstanceOnly, Category = "Firing Range|Movement")
	TObjectPtr<AFRPatrolPath> PatrolPath;

	UPROPERTY(EditAnywhere, Category = "Firing Range|Movement")
	EFRTargetMotion MotionType = EFRTargetMotion::PathPingPong;

	/** Base travel speed before the difficulty multiplier is applied. */
	UPROPERTY(EditAnywhere, Category = "Firing Range|Movement", meta = (ClampMin = "10.0"))
	float BaseMoveSpeed = 260.0f;

	/** Base pause at the ends of an open route, before the difficulty multiplier. */
	UPROPERTY(EditAnywhere, Category = "Firing Range|Movement", meta = (ClampMin = "0.0"))
	float BasePauseDuration = 0.7f;

	/** Radius of the circle used by the orbit pattern. */
	UPROPERTY(EditAnywhere, Category = "Firing Range|Movement", meta = (ClampMin = "20.0"))
	float OrbitRadius = 260.0f;
};
