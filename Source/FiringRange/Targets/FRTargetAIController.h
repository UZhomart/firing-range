// Copyright zutemiss & dshadykh. Educational project.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"

#include "Core/FRTypes.h"

#include "FRTargetAIController.generated.h"

class AFRMovingTarget;

/**
 * Brain of a moving target.
 *
 * The controller decides where the target should be at every moment and hands
 * that position to the pawn. Four patterns are supported, from a simple run
 * along a route to unpredictable strafing, and the pattern is chosen per target
 * so a single lane can mix easy and hard movement.
 *
 * It deliberately does not use MoveToLocation or any other navigation query. The
 * navigation mesh is data baked into a level asset, and this project builds its
 * level in code, so there is no mesh to query. Following a spline analytically
 * needs no navigation data and gives exact, repeatable motion, which is what a
 * training range wants anyway.
 */
UCLASS()
class FIRINGRANGE_API AFRTargetAIController : public AAIController
{
	GENERATED_BODY()

public:
	AFRTargetAIController();

	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;
	virtual void Tick(float DeltaSeconds) override;

protected:
	/** Advances along the patrol spline, looping or turning around at the ends. */
	void UpdatePathFollowing(float DeltaSeconds, bool bPingPong);

	/** Strafes between random points inside the extent of the route. */
	void UpdateRandomStrafe(float DeltaSeconds);

	/** Circles around the middle of the route. */
	void UpdateOrbit(float DeltaSeconds);

	/** Chooses the next random destination inside the lane. */
	void PickRandomDestination();

	/** Target this controller is driving. */
	TWeakObjectPtr<AFRMovingTarget> ControlledTarget;

	/** How far along the spline the target currently is, in centimetres. */
	float DistanceAlongPath = 0.0f;

	/** 1 while travelling forwards along the route, -1 while coming back. */
	int32 TravelDirection = 1;

	/** Seconds left of the pause at the end of an open route. */
	float PauseTimeRemaining = 0.0f;

	/** Current goal of the random strafe pattern, in world space. */
	FVector RandomDestination = FVector::ZeroVector;

	/** Angle of the orbit pattern, in radians. */
	float OrbitAngle = 0.0f;

	/** How close the target has to get before a random destination counts as reached. */
	static constexpr float RandomDestinationTolerance = 60.0f;
};
