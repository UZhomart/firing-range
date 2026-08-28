// Copyright zutemiss & dshadykh. Educational project.

#include "Targets/FRMovingTarget.h"

#include "GameFramework/FloatingPawnMovement.h"

#include "Targets/FRPatrolPath.h"
#include "Targets/FRTargetAIController.h"

AFRMovingTarget::AFRMovingTarget()
{
	// The pawn is possessed by its own AI controller as soon as it exists, both
	// when it is placed in a level and when the range builder spawns it.
	AIControllerClass = AFRTargetAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	MovementComponent = CreateDefaultSubobject<UFloatingPawnMovement>(TEXT("MovementComponent"));
	MovementComponent->UpdatedComponent = RootComponent;

	// High acceleration keeps the pawn glued to the point it is chasing along the
	// spline. Without it the target would trail behind its own route on every
	// direction change and drift off the lane.
	MovementComponent->Acceleration = 4000.0f;
	MovementComponent->Deceleration = 4000.0f;
	MovementComponent->TurningBoost = 8.0f;

	// Moving targets are a different colour from the static ones, so the two
	// sections of the range read as two sections.
	BoardColor = FLinearColor(0.20f, 0.25f, 0.32f);
	RingColor = FLinearColor(0.95f, 0.55f, 0.08f);
	HeadColor = FLinearColor(0.26f, 0.31f, 0.38f);

	BoardRadius = 30.0f;
	BullseyeRadius = 8.0f;
	InnerRingRadius = 17.0f;
	HeadRadius = 12.0f;
	PostHeight = 78.0f;

	// A moving target is harder to hit, so it is off the range for longer once
	// it goes down, which keeps the lane from becoming a wall of boards.
	RespawnDelay = 3.2f;
}

void AFRMovingTarget::BeginPlay()
{
	Super::BeginPlay();

	MovementComponent->MaxSpeed = GetMoveSpeed();
}

float AFRMovingTarget::GetMoveSpeed() const
{
	// Difficulty is a single multiplier that the game mode pushes into every
	// target, so one setting changes the whole range consistently.
	return BaseMoveSpeed * DifficultyScale;
}

float AFRMovingTarget::GetPauseDuration() const
{
	// Harder means shorter pauses: the target turns around almost at once.
	return BasePauseDuration / FMath::Max(DifficultyScale, 0.1f);
}

void AFRMovingTarget::MoveTowards(const FVector& TargetLocation)
{
	if (!MovementComponent)
	{
		return;
	}

	MovementComponent->MaxSpeed = GetMoveSpeed();

	const FVector Delta = TargetLocation - GetActorLocation();
	const FVector Direction = Delta.GetSafeNormal();

	if (!Direction.IsNearlyZero())
	{
		AddMovementInput(Direction, 1.0f);
	}
}

void AFRMovingTarget::StopMoving()
{
	if (MovementComponent)
	{
		MovementComponent->Velocity = FVector::ZeroVector;
		MovementComponent->StopMovementImmediately();
	}
}

void AFRMovingTarget::ResetTarget()
{
	Super::ResetTarget();

	StopMoving();
}
