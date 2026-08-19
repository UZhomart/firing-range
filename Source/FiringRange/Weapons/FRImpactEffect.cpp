// Copyright zutemiss & dshadykh. Educational project.

#include "Weapons/FRImpactEffect.h"

#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"

#include "Core/FRVisualUtils.h"

AFRImpactEffect::AFRImpactEffect()
{
	PrimaryActorTick.bCanEverTick = true;

	// The effect is purely cosmetic: it must never block a trace or a projectile.
	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	FlashMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FlashMesh"));
	FlashMesh->SetupAttachment(Root);
	FlashMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	FlashMesh->SetCastShadow(false);

	ScorchMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ScorchMesh"));
	ScorchMesh->SetupAttachment(Root);
	ScorchMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ScorchMesh->SetCastShadow(false);

	// Nothing of this actor should ever be replicated or saved.
	SetCanBeDamaged(false);
	InitialLifeSpan = 2.0f;
}

void AFRImpactEffect::Configure(const FLinearColor& InColor, float InSizeScale)
{
	EffectColor = InColor;
	SizeScale = FMath::Max(0.1f, InSizeScale);
}

void AFRImpactEffect::BeginPlay()
{
	Super::BeginPlay();

	FlashMaterial = FRVisual::BuildMesh(FlashMesh, EFRBasicShape::Sphere, EffectColor, false);
	ScorchMaterial = FRVisual::BuildMesh(ScorchMesh, EFRBasicShape::Cylinder, FLinearColor(0.05f, 0.05f, 0.06f), false);

	// The flash starts as a spark and expands. The scorch disc is a cylinder
	// squashed flat along its own up axis, which the spawn rotation has already
	// aligned with the surface normal.
	FlashMesh->SetRelativeScale3D(FVector(0.02f));
	ScorchMesh->SetRelativeScale3D(FVector(ScorchRadius * 2.0f / 100.0f, ScorchRadius * 2.0f / 100.0f, 0.012f) * SizeScale);

	// Lifting the disc off the surface by a millimetre avoids z fighting with
	// the wall it is drawn on.
	ScorchMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 0.1f));
}

void AFRImpactEffect::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	Elapsed += DeltaSeconds;

	const float Alpha = FMath::Clamp(Elapsed / FMath::Max(Lifetime, KINDA_SMALL_NUMBER), 0.0f, 1.0f);

	// Ease out expansion: fast at the start, almost still at the end.
	const float Eased = 1.0f - FMath::Square(1.0f - Alpha);
	const float Radius = FMath::Lerp(1.0f, FlashEndRadius, Eased) * SizeScale;
	FlashMesh->SetRelativeScale3D(FVector(Radius * 2.0f / 100.0f));

	// BasicShapeMaterial is opaque, so the flash fades by losing brightness
	// rather than by losing alpha.
	if (FlashMaterial)
	{
		const FLinearColor Faded = EffectColor * FMath::Pow(1.0f - Alpha, 2.0f);
		FlashMaterial->SetVectorParameterValue(TEXT("Color"), Faded);
	}

	if (Alpha >= 1.0f)
	{
		Destroy();
	}
}

void AFRImpactEffect::PlayImpactFeedback(
	const UObject* WorldContext,
	const FVector& Location,
	const FVector& Normal,
	const FLinearColor& Color,
	USoundBase* ImpactSound,
	float SizeScale)
{
	if (!WorldContext)
	{
		return;
	}

	UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContext, EGetWorldErrorMode::ReturnNull) : nullptr;
	if (!World)
	{
		return;
	}

	// MakeFromZ builds a rotation whose up axis points along the surface normal,
	// which is what lays the scorch disc flat on any wall, floor or target.
	const FRotator SurfaceRotation = FRotationMatrix::MakeFromZ(Normal).Rotator();

	const FTransform SpawnTransform(SurfaceRotation, Location);

	// Deferred spawning is required here: a plain SpawnActor runs BeginPlay before
	// returning, so the colour would be applied one frame too late.
	if (AFRImpactEffect* Effect = World->SpawnActorDeferred<AFRImpactEffect>(
		AFRImpactEffect::StaticClass(),
		SpawnTransform,
		nullptr,
		nullptr,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn))
	{
		Effect->Configure(Color, SizeScale);
		Effect->FinishSpawning(SpawnTransform);
	}

	// The sound slot stays empty in the source project because no audio asset is
	// committed. Assigning one on the weapon is enough to make impacts audible.
	if (ImpactSound)
	{
		UGameplayStatics::PlaySoundAtLocation(World, ImpactSound, Location);
	}
}
