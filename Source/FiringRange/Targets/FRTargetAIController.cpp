// Copyright zutemiss & dshadykh. Educational project.

#include "Targets/FRTargetAIController.h"

#include "Targets/FRMovingTarget.h"
#include "Targets/FRPatrolPath.h"

AFRTargetAIController::AFRTargetAIController()
{
	PrimaryActorTick.bCanEverTick = true;

	// Targets do not need a player state, and the controller must not be dragged
	// around by the pawn it possesses.
	bWantsPlayerState = false;
	bAttachToPawn = false;
}

void AFRTargetAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	ControlledTarget = Cast<AFRMovingTarget>(InPawn);

	DistanceAlongPath = 0.0f;
	TravelDirection = 1;
	PauseTimeRemaining = 0.0f;
	OrbitAngle = 0.0f;

	if (const AFRMovingTarget* Target = ControlledTarget.Get())
	{
		// Starting every target at the same point would make a whole lane move in
		// lockstep. A random offset along the route breaks that up immediately.
		if (const AFRPatrolPath* Path = Target->GetPatrolPath())
		{
			DistanceAlongPath = FMath::FRandRange(0.0f, FMath::Max(Path->GetPathLength(), 1.0f));
		}

		OrbitAngle = FMath::FRandRange(0.0f, 2.0f * PI);
	}

	PickRandomDestination();
}

void AFRTargetAIController::OnUnPossess()
{
	ControlledTarget.Reset();

	Super::OnUnPossess();
}

void AFRTargetAIController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	AFRMovingTarget* Target = ControlledTarget.Get();
	if (!Target)
	{
		return;
	}

	// A target that has been shot lies still until it stands back up. Letting it
	// keep sliding along the lane while flat looks broken.
	if (!Target->IsStanding())
	{
		Target->StopMoving();
		return;
	}

	switch (Target->GetMotionType())
	{
	case EFRTargetMotion::PathLoop:
		UpdatePathFollowing(DeltaSeconds, false);
		break;

	case EFRTargetMotion::PathPingPong:
		UpdatePathFollowing(DeltaSeconds, true);
		break;

	case EFRTargetMotion::RandomStrafe:
		UpdateRandomStrafe(DeltaSeconds);
		break;

	case EFRTargetMotion::Orbit:
		UpdateOrbit(DeltaSeconds);
		break;

	case EFRTargetMotion::Static:
	default:
		Target->StopMoving();
		break;
	}
}

void AFRTargetAIController::UpdatePathFollowing(float DeltaSeconds, bool bPingPong)
{
	AFRMovingTarget* Target = ControlledTarget.Get();
	const AFRPatrolPath* Path = Target ? Target->GetPatrolPath() : nullptr;

	if (!Path)
	{
		return;
	}

	const float PathLength = Path->GetPathLength();
	if (PathLength <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	// Waiting at the end of a run gives the player a moment to take the shot, and
	// makes the reversal readable instead of instantaneous.
	if (PauseTimeRemaining > 0.0f)
	{
		PauseTimeRemaining -= DeltaSeconds;
		Target->StopMoving();
		return;
	}

	// The goal point moves along the spline at the travel speed, and the pawn
	// chases it. Advancing a distance rather than interpolating between waypoints
	// is what keeps the speed constant through the curves.
	DistanceAlongPath += Target->GetMoveSpeed() * DeltaSeconds * TravelDirection;

	if (Path->IsClosedLoop() || !bPingPong)
	{
		// Wrap around: the route is endless in one direction.
		DistanceAlongPath = FMath::Fmod(DistanceAlongPath + PathLength, PathLength);
	}
	else if (DistanceAlongPath >= PathLength)
	{
		DistanceAlongPath = PathLength;
		TravelDirection = -1;
		PauseTimeRemaining = Target->GetPauseDuration();
	}
	else if (DistanceAlongPath <= 0.0f)
	{
		DistanceAlongPath = 0.0f;
		TravelDirection = 1;
		PauseTimeRemaining = Target->GetPauseDuration();
	}

	Target->MoveTowards(Path->GetLocationAtDistance(DistanceAlongPath));
}

void AFRTargetAIController::PickRandomDestination()
{
	const AFRMovingTarget* Target = ControlledTarget.Get();
	if (!Target)
	{
		return;
	}

	const AFRPatrolPath* Path = Target->GetPatrolPath();
	if (!Path)
	{
		RandomDestination = Target->GetActorLocation();
		return;
	}

	// The route doubles as the extent of the lane: a random point along it is a
	// legal place for the target to be, whatever shape the lane has.
	const float PathLength = Path->GetPathLength();
	RandomDestination = Path->GetLocationAtDistance(FMath::FRandRange(0.0f, PathLength));
}

void AFRTargetAIController::UpdateRandomStrafe(float DeltaSeconds)
{
	AFRMovingTarget* Target = ControlledTarget.Get();
	if (!Target)
	{
		return;
	}

	if (PauseTimeRemaining > 0.0f)
	{
		PauseTimeRemaining -= DeltaSeconds;
		Target->StopMoving();
		return;
	}

	// Arriving is measured in the horizontal plane only, because the target
	// stands on the floor and never changes height.
	const FVector ToDestination = RandomDestination - Target->GetActorLocation();
	if (ToDestination.Size2D() <= RandomDestinationTolerance)
	{
		// A short, random hesitation before darting off again is what makes the
		// pattern genuinely hard to lead.
		PauseTimeRemaining = FMath::FRandRange(0.1f, 0.6f) * Target->GetPauseDuration();
		PickRandomDestination();
		return;
	}

	Target->MoveTowards(RandomDestination);
}

void AFRTargetAIController::UpdateOrbit(float DeltaSeconds)
{
	AFRMovingTarget* Target = ControlledTarget.Get();
	if (!Target)
	{
		return;
	}

	const AFRPatrolPath* Path = Target->GetPatrolPath();

	// The centre of the circle is the middle of the route, or the place the
	// target was spawned when it has no route at all.
	const FVector Centre = Path ? Path->GetPathBounds().GetCenter() : Target->GetActorLocation();
	const float Radius = Target->GetOrbitRadius();

	// Angular speed follows from linear speed and radius, so an orbiting target
	// crosses the lane at the same pace as one walking a straight route.
	const float AngularSpeed = Target->GetMoveSpeed() / FMath::Max(Radius, 1.0f);
	OrbitAngle += AngularSpeed * DeltaSeconds;

	const FVector Offset(FMath::Cos(OrbitAngle) * Radius * 0.35f, FMath::Sin(OrbitAngle) * Radius, 0.0f);

	Target->MoveTowards(Centre + Offset);
}
