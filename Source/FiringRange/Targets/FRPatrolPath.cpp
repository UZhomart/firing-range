// Copyright zutemiss & dshadykh. Educational project.

#include "Targets/FRPatrolPath.h"

#include "Components/SplineComponent.h"
#include "DrawDebugHelpers.h"

AFRPatrolPath::AFRPatrolPath()
{
	PrimaryActorTick.bCanEverTick = false;

	Spline = CreateDefaultSubobject<USplineComponent>(TEXT("Spline"));
	SetRootComponent(Spline);

	// A sensible default so a path dropped into a level is immediately usable:
	// a straight run of eight metres across the lane.
	PathPoints = { FVector(0.0f, -400.0f, 0.0f), FVector(0.0f, 400.0f, 0.0f) };
}

void AFRPatrolPath::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	// Rebuilding here means the spline follows the points while they are being
	// edited in the level, not only once the game starts.
	RebuildSpline();
}

void AFRPatrolPath::BeginPlay()
{
	Super::BeginPlay();

	RebuildSpline();

	if (bDrawDebugPath && Spline)
	{
		const float Length = Spline->GetSplineLength();
		const int32 Segments = FMath::Max(2, FMath::RoundToInt(Length / 50.0f));

		for (int32 Index = 0; Index < Segments; ++Index)
		{
			const FVector From = GetLocationAtDistance(Length * Index / Segments);
			const FVector To = GetLocationAtDistance(Length * (Index + 1) / Segments);
			DrawDebugLine(GetWorld(), From, To, FColor::Green, false, 20.0f, 0, 2.0f);
		}
	}
}

void AFRPatrolPath::SetPathPoints(const TArray<FVector>& NewPoints, bool bNewClosedLoop)
{
	PathPoints = NewPoints;
	bClosedLoop = bNewClosedLoop;

	RebuildSpline();
}

void AFRPatrolPath::RebuildSpline()
{
	if (!Spline)
	{
		return;
	}

	Spline->ClearSplinePoints(false);

	for (const FVector& Point : PathPoints)
	{
		Spline->AddSplinePoint(Point, ESplineCoordinateSpace::Local, false);
	}

	Spline->SetClosedLoop(bClosedLoop, false);

	// One update at the end instead of one per point: the spline recomputes its
	// arc length table on every update, which is the expensive part.
	Spline->UpdateSpline();
}

float AFRPatrolPath::GetPathLength() const
{
	return Spline ? Spline->GetSplineLength() : 0.0f;
}

FVector AFRPatrolPath::GetLocationAtDistance(float Distance) const
{
	if (!Spline)
	{
		return GetActorLocation();
	}

	return Spline->GetLocationAtDistanceAlongSpline(Distance, ESplineCoordinateSpace::World);
}

FVector AFRPatrolPath::GetDirectionAtDistance(float Distance) const
{
	if (!Spline)
	{
		return GetActorForwardVector();
	}

	return Spline->GetDirectionAtDistanceAlongSpline(Distance, ESplineCoordinateSpace::World);
}

FBox AFRPatrolPath::GetPathBounds() const
{
	FBox Bounds(ForceInit);

	if (!Spline)
	{
		return Bounds;
	}

	const int32 PointCount = Spline->GetNumberOfSplinePoints();
	for (int32 Index = 0; Index < PointCount; ++Index)
	{
		Bounds += Spline->GetLocationAtSplinePoint(Index, ESplineCoordinateSpace::World);
	}

	return Bounds;
}
