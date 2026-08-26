// Copyright zutemiss & dshadykh. Educational project.

#include "Targets/FRTargetBase.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/DamageEvents.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "TimerManager.h"

#include "Core/FRVisualUtils.h"
#include "FiringRange.h"
#include "Weapons/FRImpactEffect.h"

AFRTargetBase::AFRTargetBase()
{
	PrimaryActorTick.bCanEverTick = true;

	// A target is never driven by the player and, in the stationary case, not by
	// an AI either. The subclass that moves turns possession back on.
	AutoPossessPlayer = EAutoReceiveInput::Disabled;
	AutoPossessAI = EAutoPossessAI::Disabled;
	AIControllerClass = nullptr;

	TargetRoot = CreateDefaultSubobject<USceneComponent>(TEXT("TargetRoot"));
	SetRootComponent(TargetRoot);

	PostMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PostMesh"));
	PostMesh->SetupAttachment(TargetRoot);

	PivotRoot = CreateDefaultSubobject<USceneComponent>(TEXT("PivotRoot"));
	PivotRoot->SetupAttachment(TargetRoot);

	BoardMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BoardMesh"));
	BoardMesh->SetupAttachment(PivotRoot);

	InnerRingMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("InnerRingMesh"));
	InnerRingMesh->SetupAttachment(PivotRoot);

	BullseyeMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BullseyeMesh"));
	BullseyeMesh->SetupAttachment(PivotRoot);

	HeadMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HeadMesh"));
	HeadMesh->SetupAttachment(PivotRoot);

	SetCanBeDamaged(true);
}

void AFRTargetBase::BeginPlay()
{
	Super::BeginPlay();

	BuildTargetMesh();

	State = EFRTargetState::Standing;
	TipAlpha = 0.0f;
	PivotRoot->SetRelativeRotation(FRotator::ZeroRotator);
}

void AFRTargetBase::BuildTargetMesh()
{
	// Post: a thin column that carries the board. It blocks bullets but scores
	// nothing, which is why a low shot is simply a miss.
	FRVisual::BuildMesh(PostMesh, EFRBasicShape::Cylinder, FLinearColor(0.10f, 0.11f, 0.12f), true);
	PostMesh->SetRelativeLocation(FVector(0.0f, 0.0f, PostHeight * 0.5f));
	PostMesh->SetRelativeScale3D(FRVisual::SizeToScale(FVector(8.0f, 8.0f, PostHeight)));

	PivotRoot->SetRelativeLocation(FVector(0.0f, 0.0f, PostHeight));

	// Board: a cylinder turned on its side, so its flat face looks down the lane.
	// A pitch of ninety degrees maps the local up axis of the cylinder onto the
	// forward axis of the actor, which is the direction the target faces.
	const FRotator FaceForward(90.0f, 0.0f, 0.0f);

	FRVisual::BuildMesh(BoardMesh, EFRBasicShape::Cylinder, BoardColor, true);
	BoardMesh->SetRelativeLocation(FVector(0.0f, 0.0f, BoardRadius));
	BoardMesh->SetRelativeRotation(FaceForward);
	BoardMesh->SetRelativeScale3D(FRVisual::SizeToScale(FVector(BoardRadius * 2.0f, BoardRadius * 2.0f, 6.0f)));

	// Painted rings, floating a few millimetres in front of the board face.
	FRVisual::BuildMesh(InnerRingMesh, EFRBasicShape::Cylinder, RingColor, false);
	InnerRingMesh->SetRelativeLocation(FVector(3.4f, 0.0f, BoardRadius));
	InnerRingMesh->SetRelativeRotation(FaceForward);
	InnerRingMesh->SetRelativeScale3D(FRVisual::SizeToScale(FVector(InnerRingRadius * 2.0f, InnerRingRadius * 2.0f, 0.6f)));

	FRVisual::BuildMesh(BullseyeMesh, EFRBasicShape::Cylinder, FLinearColor(0.95f, 0.75f, 0.10f), false);
	BullseyeMesh->SetRelativeLocation(FVector(3.8f, 0.0f, BoardRadius));
	BullseyeMesh->SetRelativeRotation(FaceForward);
	BullseyeMesh->SetRelativeScale3D(FRVisual::SizeToScale(FVector(BullseyeRadius * 2.0f, BullseyeRadius * 2.0f, 0.6f)));

	// Head: a sphere sitting on top of the board.
	FRVisual::BuildMesh(HeadMesh, EFRBasicShape::Sphere, HeadColor, true);
	HeadMesh->SetRelativeLocation(FVector(0.0f, 0.0f, BoardRadius * 2.0f + HeadRadius * 0.8f));
	HeadMesh->SetRelativeScale3D(FRVisual::SizeToScale(FVector(HeadRadius * 2.0f)));

	BoardMaterial = FRVisual::SetMeshColor(BoardMesh, BoardColor);
	HeadMaterial = FRVisual::SetMeshColor(HeadMesh, HeadColor);
}

EFRHitZone AFRTargetBase::ResolveHitZone(const FHitResult& Hit) const
{
	// The head is its own collider, so it is identified by the component struck.
	if (Hit.GetComponent() == HeadMesh)
	{
		return EFRHitZone::Head;
	}

	if (Hit.GetComponent() != BoardMesh)
	{
		// The post, or anything else attached to the target. Not a scoring hit.
		return EFRHitZone::None;
	}

	// Distance from the centre of the board, measured in the plane of the board.
	// Working in world space keeps the measurement in centimetres whatever the
	// component scale is, which a local space comparison would not.
	const FVector BoardCentre = BoardMesh->GetComponentLocation();
	const FVector FaceNormal = BoardMesh->GetUpVector();

	const FVector Offset = Hit.ImpactPoint - BoardCentre;
	const FVector InPlane = Offset - FaceNormal * FVector::DotProduct(Offset, FaceNormal);
	const float RadialDistance = InPlane.Size();

	if (RadialDistance <= BullseyeRadius)
	{
		return EFRHitZone::Bullseye;
	}

	if (RadialDistance <= InnerRingRadius)
	{
		return EFRHitZone::Inner;
	}

	return EFRHitZone::Body;
}

float AFRTargetBase::TakeDamage(
	float DamageAmount,
	FDamageEvent const& DamageEvent,
	AController* EventInstigator,
	AActor* DamageCauser)
{
	// A target that is already falling or flat cannot be scored twice, which is
	// what stops a shotgun blast from counting as eight separate knockdowns.
	if (!IsStanding())
	{
		return 0.0f;
	}

	FHitResult HitInfo;
	FVector ImpulseDirection;
	DamageEvent.GetBestHitInfo(this, DamageCauser, HitInfo, ImpulseDirection);

	const EFRHitZone Zone = ResolveHitZone(HitInfo);
	if (Zone == EFRHitZone::None)
	{
		return 0.0f;
	}

	const FVector ImpactPoint = HitInfo.ImpactPoint.IsNearlyZero() ? GetActorLocation() : HitInfo.ImpactPoint;

	// Distance is reported in metres because that is the unit a shooting range
	// is measured in, and the HUD prints it next to the hit marker.
	float DistanceMetres = 0.0f;
	if (const APawn* Shooter = EventInstigator ? EventInstigator->GetPawn() : nullptr)
	{
		DistanceMetres = FVector::Dist(Shooter->GetActorLocation(), ImpactPoint) / 100.0f;
	}

	AFRImpactEffect::PlayImpactFeedback(this, ImpactPoint, -ImpulseDirection, FRTypes::GetHitZoneColor(Zone), HitSound, 1.4f);

	OnTargetHit.Broadcast(this, Zone, DistanceMetres);

	HandleKnockedDown(Zone, ImpactPoint);

	return DamageAmount;
}

void AFRTargetBase::HandleKnockedDown(EFRHitZone Zone, const FVector& ImpactPoint)
{
	State = EFRTargetState::FallingDown;

	// Turning collision off during the fall prevents a second bullet already in
	// flight from scoring on a target that is on its way down.
	SetScoringCollisionEnabled(false);

	if (UWorld* World = GetWorld())
	{
		// Higher difficulty means the target comes back sooner.
		const float Delay = FMath::Max(0.2f, RespawnDelay / FMath::Max(DifficultyScale, 0.1f));
		World->GetTimerManager().SetTimer(RespawnTimerHandle, this, &AFRTargetBase::HandleRespawn, Delay, false);
	}
}

void AFRTargetBase::HandleRespawn()
{
	State = EFRTargetState::RisingUp;
}

void AFRTargetBase::ResetTarget()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(RespawnTimerHandle);
	}

	State = EFRTargetState::Standing;
	TipAlpha = 0.0f;
	PivotRoot->SetRelativeRotation(FRotator::ZeroRotator);
	SetScoringCollisionEnabled(true);
}

void AFRTargetBase::SetDifficulty(EFRDifficulty NewDifficulty)
{
	DifficultyScale = FRTypes::GetDifficultyScale(NewDifficulty);
}

void AFRTargetBase::SetScoringCollisionEnabled(bool bEnabled)
{
	const ECollisionEnabled::Type Mode = bEnabled ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision;

	BoardMesh->SetCollisionEnabled(Mode);
	HeadMesh->SetCollisionEnabled(Mode);
}

void AFRTargetBase::UpdateTipAnimation(float DeltaSeconds)
{
	const float Step = DeltaSeconds / FMath::Max(TipDuration, KINDA_SMALL_NUMBER);

	switch (State)
	{
	case EFRTargetState::FallingDown:
		TipAlpha = FMath::Min(TipAlpha + Step, 1.0f);
		if (TipAlpha >= 1.0f)
		{
			State = EFRTargetState::Down;
		}
		break;

	case EFRTargetState::RisingUp:
		TipAlpha = FMath::Max(TipAlpha - Step, 0.0f);
		if (TipAlpha <= 0.0f)
		{
			State = EFRTargetState::Standing;
			SetScoringCollisionEnabled(true);
		}
		break;

	default:
		return;
	}

	// The board hinges backwards around the top of the post. A negative pitch
	// tips the face away from the shooter.
	PivotRoot->SetRelativeRotation(FRotator(-TipAngle * TipAlpha, 0.0f, 0.0f));
}

void AFRTargetBase::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	UpdateTipAnimation(DeltaSeconds);
}
