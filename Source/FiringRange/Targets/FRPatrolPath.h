// Copyright zutemiss & dshadykh. Educational project.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "FRPatrolPath.generated.h"

class USplineComponent;

/**
 * Predefined route a moving target follows.
 *
 * The route is a spline rather than a list of waypoints. Waypoints would make
 * the target snap direction at every corner; a spline gives a continuous curve,
 * so the target sweeps across the lane at a steady speed and stays genuinely
 * hard to lead. The AI controller only ever asks for a position at a distance
 * along the path, so the same controller drives any shape of route.
 *
 * Points can be authored in the editor or handed over from code, which is how
 * the procedural range builder lays its lanes out.
 */
UCLASS()
class FIRINGRANGE_API AFRPatrolPath : public AActor
{
	GENERATED_BODY()

public:
	AFRPatrolPath();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;

	/** Replaces the route. Positions are relative to the actor. */
	void SetPathPoints(const TArray<FVector>& NewPoints, bool bNewClosedLoop);

	/** Total length of the route in centimetres. */
	float GetPathLength() const;

	/** World position at a distance measured along the route. */
	FVector GetLocationAtDistance(float Distance) const;

	/** Unit direction of travel at a distance measured along the route. */
	FVector GetDirectionAtDistance(float Distance) const;

	/** True when the end of the route joins back onto its start. */
	bool IsClosedLoop() const { return bClosedLoop; }

	/** Route in world space, used by patterns that need the extent of the lane. */
	FBox GetPathBounds() const;

protected:
	UPROPERTY(VisibleAnywhere, Category = "Firing Range|Patrol")
	TObjectPtr<USplineComponent> Spline;

	/** Route control points, in the local space of the actor. */
	UPROPERTY(EditAnywhere, Category = "Firing Range|Patrol")
	TArray<FVector> PathPoints;

	/** When true the spline is closed and the target circles it forever. */
	UPROPERTY(EditAnywhere, Category = "Firing Range|Patrol")
	bool bClosedLoop = false;

	/** Draws the route in the world for a few seconds at BeginPlay. */
	UPROPERTY(EditAnywhere, Category = "Firing Range|Patrol")
	bool bDrawDebugPath = false;

	/** Rewrites the spline from PathPoints. */
	void RebuildSpline();
};
