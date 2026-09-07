// Copyright zutemiss & dshadykh. Educational project.

#include "Level/FRRangeBuilder.h"

#include "Components/DirectionalLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/ExponentialHeightFog.h"
#include "Engine/SkyLight.h"
#include "Engine/World.h"
#include "GameFramework/PlayerStart.h"

#include "FiringRange.h"
#include "Pickups/FRAmmoPickup.h"
#include "Targets/FRMovingTarget.h"
#include "Targets/FRPatrolPath.h"
#include "Targets/FRStationaryTarget.h"

AFRRangeBuilder::AFRRangeBuilder()
{
	PrimaryActorTick.bCanEverTick = false;

	BuilderRoot = CreateDefaultSubobject<USceneComponent>(TEXT("BuilderRoot"));
	SetRootComponent(BuilderRoot);

	// Three boards at increasing range, so the player has somewhere to warm up
	// and somewhere to be humbled.
	StationaryDistances = { 1600.0f, 2800.0f, 4000.0f };

	// Patrol lanes sit between and beyond the static boards.
	MovingDistances = { 2000.0f, 3200.0f, 4400.0f };
}

void AFRRangeBuilder::BeginPlay()
{
	Super::BeginPlay();

	// Normally the game mode has already built the range during InitGame. This
	// call covers the case of a builder dropped into a level by hand.
	BuildRange();
}

void AFRRangeBuilder::BuildRange()
{
	if (bRangeBuilt)
	{
		return;
	}

	bRangeBuilt = true;

	BuildGround();
	BuildBerms();
	BuildFiringLine();
	BuildLaneDividers();
	BuildDistanceMarkers();
	BuildStationarySection();
	BuildMovingSection();
	BuildAmmoCrates();
	BuildLighting();

	UE_LOG(LogFiringRange, Log, TEXT("Range built with %d targets."), SpawnedTargets.Num());
}

FTransform AFRRangeBuilder::GetPlayerSpawnTransform() const
{
	// Standing behind the firing line, looking down range. The height clears the
	// capsule half height so the character does not start inside the ground.
	return FTransform(FRotator::ZeroRotator, GetActorLocation() + FVector(-PlayerSetback, 0.0f, 100.0f));
}

UStaticMeshComponent* AFRRangeBuilder::AddBlock(
	const FVector& Location,
	const FVector& Size,
	const FLinearColor& Color,
	const FRotator& Rotation,
	bool bCollides,
	EFRBasicShape Shape)
{
	UStaticMeshComponent* Mesh = NewObject<UStaticMeshComponent>(this);
	if (!Mesh)
	{
		return nullptr;
	}

	// Mobility has to be decided before registration. Anything created while the
	// game is running is movable by definition.
	Mesh->SetMobility(EComponentMobility::Movable);
	Mesh->SetupAttachment(BuilderRoot);
	Mesh->RegisterComponent();

	Mesh->SetRelativeLocation(Location);
	Mesh->SetRelativeRotation(Rotation);

	FRVisual::BuildMesh(Mesh, Shape, Color, bCollides);
	Mesh->SetRelativeScale3D(FRVisual::SizeToScale(Size));

	return Mesh;
}

// -- Ground and shell --------------------------------------------------------

void AFRRangeBuilder::BuildGround()
{
	const float GroundLength = RangeLength + 2000.0f;
	const float GroundCentreX = (RangeLength - 1000.0f) * 0.5f;

	// The slab sits below zero so its top face is exactly the walking plane.
	AddBlock(
		FVector(GroundCentreX, 0.0f, -40.0f),
		FVector(GroundLength, RangeHalfWidth * 2.0f, 80.0f),
		GroundColor);

	// Concrete apron under the firing line, a visual anchor for where to stand.
	AddBlock(
		FVector(-200.0f, 0.0f, 1.0f),
		FVector(900.0f, RangeHalfWidth * 1.8f, 6.0f),
		ConcreteColor,
		FRotator::ZeroRotator,
		false);

	UWorld* World = GetWorld();
	if (!World || GeneratedPlayerStart)
	{
		return;
	}

	// The player start is part of the range, not part of the map. Creating it
	// here is what lets the FiringRange map stay completely empty.
	const FTransform SpawnTransform = GetPlayerSpawnTransform();

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	GeneratedPlayerStart = World->SpawnActor<APlayerStart>(
		APlayerStart::StaticClass(), SpawnTransform.GetLocation(), SpawnTransform.Rotator(), SpawnParams);
}

void AFRRangeBuilder::BuildBerms()
{
	const float BermHeight = 520.0f;
	const float BermThickness = 320.0f;
	const float BermLength = RangeLength + 1400.0f;
	const float BermCentreX = (RangeLength - 700.0f) * 0.5f;

	// Side berms. A real range is walled in by earth, and it also stops a stray
	// bullet from flying off into nothing and being counted as a lost projectile.
	for (const float Side : { -1.0f, 1.0f })
	{
		AddBlock(
			FVector(BermCentreX, Side * (RangeHalfWidth + BermThickness * 0.5f), BermHeight * 0.5f),
			FVector(BermLength, BermThickness, BermHeight),
			BermColor);
	}

	// Backstop, taller than the sides because everything is shot towards it.
	AddBlock(
		FVector(RangeLength, 0.0f, 380.0f),
		FVector(BermThickness, RangeHalfWidth * 2.0f + BermThickness * 2.0f, 760.0f),
		BermColor);

	// Low berm behind the player, closing the range off.
	AddBlock(
		FVector(-PlayerSetback - 500.0f, 0.0f, 260.0f),
		FVector(BermThickness, RangeHalfWidth * 2.0f, 520.0f),
		BermColor);
}

void AFRRangeBuilder::BuildFiringLine()
{
	// Shooting bench. Low enough to see over while standing, high enough to read
	// as the line the shooter stays behind.
	AddBlock(
		FVector(90.0f, 0.0f, 50.0f),
		FVector(70.0f, RangeHalfWidth * 1.7f, 100.0f),
		ConcreteColor);

	// Canopy posts and roof over the firing line.
	const float PostHeight = 340.0f;
	const float CanopyHalfWidth = RangeHalfWidth * 0.85f;

	for (const float PostX : { 140.0f, -560.0f })
	{
		for (const float Side : { -1.0f, 1.0f })
		{
			AddBlock(
				FVector(PostX, Side * CanopyHalfWidth, PostHeight * 0.5f),
				FVector(26.0f, 26.0f, PostHeight),
				TimberColor);
		}
	}

	AddBlock(
		FVector(-210.0f, 0.0f, PostHeight + 12.0f),
		FVector(860.0f, CanopyHalfWidth * 2.0f + 60.0f, 24.0f),
		TimberColor);

	// Section signs hanging from the canopy, coloured to match the two halves of
	// the range so the player knows which way to look before walking up.
	AddBlock(
		FVector(150.0f, -RangeHalfWidth * 0.45f, 300.0f),
		FVector(12.0f, 520.0f, 90.0f),
		StaticSectionColor,
		FRotator::ZeroRotator,
		false);

	AddBlock(
		FVector(150.0f, RangeHalfWidth * 0.45f, 300.0f),
		FVector(12.0f, 520.0f, 90.0f),
		MovingSectionColor,
		FRotator::ZeroRotator,
		false);
}

void AFRRangeBuilder::BuildLaneDividers()
{
	const float DividerLength = RangeLength - 1200.0f;
	const float DividerCentreX = 600.0f + DividerLength * 0.5f;

	// The centre divider splits the range into its two sections, and the two
	// outer ones separate the stationary lanes.
	const float DividerPositions[] = { 0.0f, -RangeHalfWidth * 0.45f, -RangeHalfWidth * 0.9f };

	for (const float PositionY : DividerPositions)
	{
		AddBlock(
			FVector(DividerCentreX, PositionY, 45.0f),
			FVector(DividerLength, 22.0f, 90.0f),
			ConcreteColor);
	}
}

void AFRRangeBuilder::BuildDistanceMarkers()
{
	// A post every ten metres along both berms. Height steps up with distance,
	// which turns the row into a ruler the player can read at a glance.
	for (int32 Metres = 10; Metres * 100 < RangeLength; Metres += 10)
	{
		const float MarkerX = static_cast<float>(Metres * 100);
		const float MarkerHeight = 90.0f + Metres * 2.0f;

		for (const float Side : { -1.0f, 1.0f })
		{
			AddBlock(
				FVector(MarkerX, Side * (RangeHalfWidth - 40.0f), MarkerHeight * 0.5f),
				FVector(18.0f, 18.0f, MarkerHeight),
				ConcreteColor,
				FRotator::ZeroRotator,
				false);

			// Bright cap, so the marker is visible against the berm behind it.
			AddBlock(
				FVector(MarkerX, Side * (RangeHalfWidth - 40.0f), MarkerHeight + 10.0f),
				FVector(26.0f, 26.0f, 20.0f),
				FLinearColor(0.95f, 0.80f, 0.12f),
				FRotator::ZeroRotator,
				false);
		}
	}
}

// -- Targets -----------------------------------------------------------------

void AFRRangeBuilder::BuildStationarySection()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	// Two lanes on the left half of the range, one board per distance in each.
	const float LaneOffsets[] = { -RangeHalfWidth * 0.22f, -RangeHalfWidth * 0.67f };

	for (const float Distance : StationaryDistances)
	{
		for (const float LaneY : LaneOffsets)
		{
			const FVector Location = GetActorLocation() + FVector(Distance, LaneY, 0.0f);

			// Yaw of 180 turns the board around to face back down the range at
			// the shooter, because a target is built facing its own forward axis.
			AFRStationaryTarget* Target = World->SpawnActor<AFRStationaryTarget>(
				AFRStationaryTarget::StaticClass(), Location, FRotator(0.0f, 180.0f, 0.0f), SpawnParams);

			if (Target)
			{
				SpawnedTargets.Add(Target);
			}
		}
	}
}

void AFRRangeBuilder::BuildMovingSection()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	// One pattern per lane, so a single walk down the range shows all three.
	const EFRTargetMotion Patterns[] =
	{
		EFRTargetMotion::PathPingPong,
		EFRTargetMotion::PathLoop,
		EFRTargetMotion::RandomStrafe
	};

	const float LaneCentreY = RangeHalfWidth * 0.5f;
	const float LaneHalfSpan = RangeHalfWidth * 0.38f;

	for (int32 Index = 0; Index < MovingDistances.Num(); ++Index)
	{
		const float Distance = MovingDistances[Index];
		const FVector PathOrigin = GetActorLocation() + FVector(Distance, LaneCentreY, 0.0f);

		AFRPatrolPath* Path = World->SpawnActor<AFRPatrolPath>(
			AFRPatrolPath::StaticClass(), PathOrigin, FRotator::ZeroRotator, SpawnParams);

		if (!Path)
		{
			continue;
		}

		// A route that crosses the lane from side to side, bowed slightly down
		// range in the middle so the target also changes distance as it travels.
		const TArray<FVector> Points =
		{
			FVector(0.0f, -LaneHalfSpan, 0.0f),
			FVector(160.0f, 0.0f, 0.0f),
			FVector(0.0f, LaneHalfSpan, 0.0f)
		};

		Path->SetPathPoints(Points, false);

		// Deferred spawning: the AI controller possesses the pawn while it is
		// being created and reads the route straight away, so the route has to be
		// set before the pawn finishes spawning.
		const FTransform TargetTransform(FRotator(0.0f, 180.0f, 0.0f), Path->GetLocationAtDistance(0.0f));

		AFRMovingTarget* Target = World->SpawnActorDeferred<AFRMovingTarget>(
			AFRMovingTarget::StaticClass(),
			TargetTransform,
			nullptr,
			nullptr,
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn);

		if (!Target)
		{
			continue;
		}

		Target->SetPatrolPath(Path);
		Target->SetMotionType(Patterns[Index % UE_ARRAY_COUNT(Patterns)]);
		Target->FinishSpawning(TargetTransform);

		SpawnedTargets.Add(Target);
	}
}

void AFRRangeBuilder::BuildAmmoCrates()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	// One crate per ammunition family, lined up behind the firing line where the
	// player walks past them on the way to the bench.
	struct FCrateLayout
	{
		EFRAmmoType AmmoType;
		int32 Amount;
		float OffsetY;
	};

	const FCrateLayout Crates[] =
	{
		{ EFRAmmoType::Pistol, 45, -520.0f },
		{ EFRAmmoType::Shell,  18,    0.0f },
		{ EFRAmmoType::Rifle,  15,  520.0f }
	};

	for (const FCrateLayout& Layout : Crates)
	{
		const FVector Location = GetActorLocation() + FVector(-PlayerSetback + 120.0f, Layout.OffsetY, 0.0f);

		AFRAmmoPickup* Pickup = World->SpawnActor<AFRAmmoPickup>(
			AFRAmmoPickup::StaticClass(), Location, FRotator::ZeroRotator, SpawnParams);

		if (Pickup)
		{
			Pickup->Configure(Layout.AmmoType, Layout.Amount);
		}
	}
}

// -- Lighting ----------------------------------------------------------------

void AFRRangeBuilder::BuildLighting()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	// Sun. Low and from behind the shooter's left, so the boards down range are
	// lit from the front and their rings stay readable at every distance.
	if (ADirectionalLight* Sun = World->SpawnActor<ADirectionalLight>(
		ADirectionalLight::StaticClass(), FVector(0.0f, 0.0f, 1200.0f), FRotator(-38.0f, 125.0f, 0.0f), SpawnParams))
	{
		// ALight exposes mobility directly. It has to be movable because the actor
		// is created after the level has already been loaded.
		Sun->SetMobility(EComponentMobility::Movable);

		if (UDirectionalLightComponent* SunComponent = Cast<UDirectionalLightComponent>(Sun->GetLightComponent()))
		{
			SunComponent->SetIntensity(6.0f);
			SunComponent->SetLightColor(FLinearColor(1.0f, 0.95f, 0.86f));

			// Marks this light as the one the atmosphere scatters, which is what
			// produces the sky colour and the horizon.
			SunComponent->bAtmosphereSunLight = true;
			SunComponent->MarkRenderStateDirty();
		}
	}

	// Atmosphere. Without it the sky is a flat void and the sky light below has
	// nothing to capture.
	World->SpawnActor<ASkyAtmosphere>(ASkyAtmosphere::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);

	// Sky light in real time capture mode: it samples the atmosphere every frame
	// and turns it into ambient light, so nothing in shadow is ever pitch black.
	if (ASkyLight* SkyLight = World->SpawnActor<ASkyLight>(
		ASkyLight::StaticClass(), FVector(0.0f, 0.0f, 900.0f), FRotator::ZeroRotator, SpawnParams))
	{
		// ASkyLight is an AInfo and has no mobility of its own, so the mobility is
		// set on its component instead.
		if (USkyLightComponent* SkyComponent = SkyLight->GetLightComponent())
		{
			SkyComponent->SetMobility(EComponentMobility::Movable);
			SkyComponent->bRealTimeCapture = true;
			SkyComponent->SourceType = ESkyLightSourceType::SLS_CapturedScene;
			SkyComponent->SetIntensity(1.15f);
			SkyComponent->MarkRenderStateDirty();
		}
	}

	// A thin fog gives the long lanes a sense of depth, which is what makes the
	// far boards read as far rather than small.
	if (AExponentialHeightFog* Fog = World->SpawnActor<AExponentialHeightFog>(
		AExponentialHeightFog::StaticClass(), FVector(0.0f, 0.0f, -200.0f), FRotator::ZeroRotator, SpawnParams))
	{
		if (UExponentialHeightFogComponent* FogComponent = Fog->GetComponent())
		{
			FogComponent->SetMobility(EComponentMobility::Movable);
			FogComponent->SetFogDensity(0.012f);
			FogComponent->SetFogHeightFalloff(0.25f);
			FogComponent->SetFogInscatteringColor(FLinearColor(0.42f, 0.48f, 0.56f));
			FogComponent->MarkRenderStateDirty();
		}
	}
}
