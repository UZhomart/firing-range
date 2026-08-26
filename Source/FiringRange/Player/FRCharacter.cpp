// Copyright zutemiss & dshadykh. Educational project.

#include "Player/FRCharacter.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"

#include "Core/FRGameInstance.h"
#include "FiringRange.h"
#include "Player/FRPlayerController.h"
#include "Weapons/FRPistol.h"
#include "Weapons/FRShotgun.h"
#include "Weapons/FRSniperRifle.h"
#include "Weapons/FRWeaponBase.h"

AFRCharacter::AFRCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	UCapsuleComponent* Capsule = GetCapsuleComponent();
	Capsule->InitCapsuleSize(38.0f, 90.0f);

	// A first person shooter turns the whole body with the mouse yaw, but the
	// pitch belongs to the camera alone, otherwise the capsule would tip over.
	bUseControllerRotationYaw = true;
	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;

	BaseEyeHeight = 70.0f;
	CrouchedEyeHeight = 44.0f;

	FirstPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCamera->SetupAttachment(Capsule);
	FirstPersonCamera->SetRelativeLocation(FVector(0.0f, 0.0f, BaseEyeHeight));
	FirstPersonCamera->bUsePawnControlRotation = true;
	FirstPersonCamera->SetFieldOfView(90.0f);

	// Parenting the holder to the camera means the weapon inherits the view
	// rotation for free; only the procedural offsets have to be animated.
	WeaponHolder = CreateDefaultSubobject<USceneComponent>(TEXT("WeaponHolder"));
	WeaponHolder->SetupAttachment(FirstPersonCamera);

	UCharacterMovementComponent* Movement = GetCharacterMovement();
	Movement->bOrientRotationToMovement = false;
	Movement->MaxWalkSpeed = WalkSpeed;
	Movement->MaxWalkSpeedCrouched = 220.0f;
	Movement->JumpZVelocity = 460.0f;
	Movement->AirControl = 0.35f;
	Movement->BrakingDecelerationWalking = 2048.0f;
	Movement->GetNavAgentPropertiesRef().bCanCrouch = true;

	// One entry per EFRAmmoType, in declaration order: 9 mm, 12 Gauge, 7.62 mm.
	const int32 AmmoTypes = FRTypes::AmmoTypeCount();
	StartingReserveAmmo.Init(0, AmmoTypes);
	MaxReserveAmmoPerType.Init(0, AmmoTypes);

	StartingReserveAmmo[static_cast<int32>(EFRAmmoType::Pistol)] = 72;
	StartingReserveAmmo[static_cast<int32>(EFRAmmoType::Shell)] = 32;
	StartingReserveAmmo[static_cast<int32>(EFRAmmoType::Rifle)] = 25;

	MaxReserveAmmoPerType[static_cast<int32>(EFRAmmoType::Pistol)] = 180;
	MaxReserveAmmoPerType[static_cast<int32>(EFRAmmoType::Shell)] = 80;
	MaxReserveAmmoPerType[static_cast<int32>(EFRAmmoType::Rifle)] = 60;

	// Slot order is the order of the number keys on the keyboard.
	WeaponClasses.Add(AFRPistol::StaticClass());
	WeaponClasses.Add(AFRShotgun::StaticClass());
	WeaponClasses.Add(AFRSniperRifle::StaticClass());
}

int32 AFRCharacter::GetAmmoIndex(EFRAmmoType AmmoType) const
{
	const int32 Index = static_cast<int32>(AmmoType);
	return ReserveAmmo.IsValidIndex(Index) ? Index : INDEX_NONE;
}

int32 AFRCharacter::GetReserveAmmo(EFRAmmoType AmmoType) const
{
	const int32 Index = GetAmmoIndex(AmmoType);
	return Index != INDEX_NONE ? ReserveAmmo[Index] : 0;
}

int32 AFRCharacter::GetMaxReserveAmmo(EFRAmmoType AmmoType) const
{
	const int32 Index = static_cast<int32>(AmmoType);
	return MaxReserveAmmoPerType.IsValidIndex(Index) ? MaxReserveAmmoPerType[Index] : 0;
}

int32 AFRCharacter::AddReserveAmmo(EFRAmmoType AmmoType, int32 Amount)
{
	const int32 Index = GetAmmoIndex(AmmoType);
	if (Index == INDEX_NONE || Amount <= 0)
	{
		return 0;
	}

	const int32 Limit = GetMaxReserveAmmo(AmmoType);
	const int32 Before = ReserveAmmo[Index];
	ReserveAmmo[Index] = FMath::Min(Before + Amount, Limit);

	// The caller uses the return value to decide whether a pickup was consumed:
	// walking over a box while already full must leave the box in the world.
	return ReserveAmmo[Index] - Before;
}

int32 AFRCharacter::ConsumeReserveAmmo(EFRAmmoType AmmoType, int32 Amount)
{
	const int32 Index = GetAmmoIndex(AmmoType);
	if (Index == INDEX_NONE || Amount <= 0)
	{
		return 0;
	}

	const int32 Taken = FMath::Min(Amount, ReserveAmmo[Index]);
	ReserveAmmo[Index] -= Taken;
	return Taken;
}

void AFRCharacter::ResetReserveAmmo()
{
	ReserveAmmo = StartingReserveAmmo;
	ReserveAmmo.SetNum(FRTypes::AmmoTypeCount());
}

void AFRCharacter::BeginPlay()
{
	Super::BeginPlay();

	ResetReserveAmmo();
	RefreshLookSettings();
	SpawnLoadout();

	// Settings can change while the range level is running, because the pause
	// menu exposes the same sliders as the main menu.
	if (UFRGameInstance* GameInstance = UFRGameInstance::Get(this))
	{
		GameInstance->OnSettingsChanged.AddUObject(this, &AFRCharacter::RefreshLookSettings);
	}

	UpdateMovementSpeed();
}

void AFRCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UFRGameInstance* GameInstance = UFRGameInstance::Get(this))
	{
		GameInstance->OnSettingsChanged.RemoveAll(this);
	}

	Super::EndPlay(EndPlayReason);
}

void AFRCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	UpdateAimFieldOfView(DeltaSeconds);
}

void AFRCharacter::UpdateAimFieldOfView(float DeltaSeconds)
{
	if (!FirstPersonCamera)
	{
		return;
	}

	const AFRWeaponBase* Weapon = GetActiveWeapon();
	const float TargetFov = (bIsAiming && Weapon) ? Weapon->GetAimFieldOfView() : HipFieldOfView;

	// Interpolating rather than snapping is what makes aiming read as raising the
	// weapon instead of as a camera cut.
	const float NewFov = FMath::FInterpTo(FirstPersonCamera->FieldOfView, TargetFov, DeltaSeconds, FieldOfViewInterpSpeed);
	FirstPersonCamera->SetFieldOfView(NewFov);
}

void AFRCharacter::RefreshLookSettings()
{
	if (const UFRGameInstance* GameInstance = UFRGameInstance::Get(this))
	{
		CachedMouseSensitivity = GameInstance->GetMouseSensitivity();
		CachedAimSensitivityScale = GameInstance->GetAimSensitivityScale();
		bCachedInvertLookY = GameInstance->GetInvertLookY();
	}
}

UInputAction* AFRCharacter::CreateInputAction(FName ActionName, EInputActionValueType ValueType)
{
	UInputAction* Action = NewObject<UInputAction>(this, ActionName);
	Action->ValueType = ValueType;
	return Action;
}

void AFRCharacter::BuildInputActions()
{
	// Built once per character instance. The guard keeps the method safe to call
	// from both PawnClientRestart and SetupPlayerInputComponent.
	if (InputContext)
	{
		return;
	}

	InputContext = NewObject<UInputMappingContext>(this, TEXT("IMC_FiringRange"));

	ActionMove = CreateInputAction(TEXT("IA_Move"), EInputActionValueType::Axis2D);
	ActionLook = CreateInputAction(TEXT("IA_Look"), EInputActionValueType::Axis2D);
	ActionJump = CreateInputAction(TEXT("IA_Jump"), EInputActionValueType::Boolean);
	ActionSprint = CreateInputAction(TEXT("IA_Sprint"), EInputActionValueType::Boolean);
	ActionCrouch = CreateInputAction(TEXT("IA_Crouch"), EInputActionValueType::Boolean);
	ActionFire = CreateInputAction(TEXT("IA_Fire"), EInputActionValueType::Boolean);
	ActionAim = CreateInputAction(TEXT("IA_Aim"), EInputActionValueType::Boolean);
	ActionReload = CreateInputAction(TEXT("IA_Reload"), EInputActionValueType::Boolean);
	ActionNextWeapon = CreateInputAction(TEXT("IA_NextWeapon"), EInputActionValueType::Boolean);
	ActionPreviousWeapon = CreateInputAction(TEXT("IA_PreviousWeapon"), EInputActionValueType::Boolean);
	ActionWeaponSlotOne = CreateInputAction(TEXT("IA_WeaponSlotOne"), EInputActionValueType::Boolean);
	ActionWeaponSlotTwo = CreateInputAction(TEXT("IA_WeaponSlotTwo"), EInputActionValueType::Boolean);
	ActionWeaponSlotThree = CreateInputAction(TEXT("IA_WeaponSlotThree"), EInputActionValueType::Boolean);

	// ----- Movement ---------------------------------------------------------
	// A single Axis2D action carries both axes. A key press produces 1.0 on X,
	// so the swizzle moves that value onto Y for the forward and back keys, and
	// the negate modifier flips the sign for back and left.

	InputContext->MapKey(ActionMove, EKeys::W).Modifiers.Add(NewObject<UInputModifierSwizzleAxis>(InputContext));

	{
		FEnhancedActionKeyMapping& Mapping = InputContext->MapKey(ActionMove, EKeys::S);
		Mapping.Modifiers.Add(NewObject<UInputModifierNegate>(InputContext));
		Mapping.Modifiers.Add(NewObject<UInputModifierSwizzleAxis>(InputContext));
	}

	InputContext->MapKey(ActionMove, EKeys::A).Modifiers.Add(NewObject<UInputModifierNegate>(InputContext));
	InputContext->MapKey(ActionMove, EKeys::D);

	// The left stick already delivers a 2D vector in the expected layout.
	InputContext->MapKey(ActionMove, EKeys::Gamepad_Left2D);

	// ----- Look -------------------------------------------------------------
	// Mouse2D reports a positive Y when the mouse moves away from the player,
	// while AddControllerPitchInput expects the opposite sign, hence the negate
	// on the vertical axis only.

	{
		FEnhancedActionKeyMapping& Mapping = InputContext->MapKey(ActionLook, EKeys::Mouse2D);
		UInputModifierNegate* NegateY = NewObject<UInputModifierNegate>(InputContext);
		NegateY->bX = false;
		NegateY->bY = true;
		NegateY->bZ = false;
		Mapping.Modifiers.Add(NegateY);
	}

	{
		FEnhancedActionKeyMapping& Mapping = InputContext->MapKey(ActionLook, EKeys::Gamepad_Right2D);
		UInputModifierNegate* NegateY = NewObject<UInputModifierNegate>(InputContext);
		NegateY->bX = false;
		NegateY->bY = true;
		NegateY->bZ = false;
		Mapping.Modifiers.Add(NegateY);
	}

	// ----- Stance -----------------------------------------------------------

	InputContext->MapKey(ActionJump, EKeys::SpaceBar);
	InputContext->MapKey(ActionJump, EKeys::Gamepad_FaceButton_Bottom);

	InputContext->MapKey(ActionSprint, EKeys::LeftShift);
	InputContext->MapKey(ActionSprint, EKeys::Gamepad_LeftThumbstick);

	InputContext->MapKey(ActionCrouch, EKeys::LeftControl);
	InputContext->MapKey(ActionCrouch, EKeys::C);
	InputContext->MapKey(ActionCrouch, EKeys::Gamepad_RightThumbstick);

	// ----- Combat -----------------------------------------------------------

	InputContext->MapKey(ActionFire, EKeys::LeftMouseButton);
	InputContext->MapKey(ActionFire, EKeys::Gamepad_RightTrigger);

	InputContext->MapKey(ActionAim, EKeys::RightMouseButton);
	InputContext->MapKey(ActionAim, EKeys::Gamepad_LeftTrigger);

	InputContext->MapKey(ActionReload, EKeys::R);
	InputContext->MapKey(ActionReload, EKeys::Gamepad_FaceButton_Left);

	InputContext->MapKey(ActionNextWeapon, EKeys::MouseScrollUp);
	InputContext->MapKey(ActionNextWeapon, EKeys::Gamepad_DPad_Right);

	InputContext->MapKey(ActionPreviousWeapon, EKeys::MouseScrollDown);
	InputContext->MapKey(ActionPreviousWeapon, EKeys::Gamepad_DPad_Left);

	InputContext->MapKey(ActionWeaponSlotOne, EKeys::One);
	InputContext->MapKey(ActionWeaponSlotTwo, EKeys::Two);
	InputContext->MapKey(ActionWeaponSlotThree, EKeys::Three);
}

void AFRCharacter::PawnClientRestart()
{
	Super::PawnClientRestart();

	BuildInputActions();

	if (const APlayerController* PlayerController = Cast<APlayerController>(GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
			ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			// Only this context is removed, never every mapping: the controller
			// registers its own pause context before the pawn is possessed, and
			// ClearAllMappings here would silently destroy it. Removing before
			// adding still keeps respawning idempotent.
			Subsystem->RemoveMappingContext(InputContext);
			Subsystem->AddMappingContext(InputContext, AFRPlayerController::PawnContextPriority);
		}
	}
}

void AFRCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	BuildInputActions();

	UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (!EnhancedInput)
	{
		// This happens when DefaultInputComponentClass was not overridden in
		// Config/DefaultInput.ini. Failing loudly saves hours of silent debugging.
		UE_LOG(LogFiringRange, Error,
			TEXT("Enhanced Input component missing. Check DefaultInputComponentClass in Config/DefaultInput.ini."));
		return;
	}

	EnhancedInput->BindAction(ActionMove, ETriggerEvent::Triggered, this, &AFRCharacter::Input_Move);
	EnhancedInput->BindAction(ActionLook, ETriggerEvent::Triggered, this, &AFRCharacter::Input_Look);
	EnhancedInput->BindAction(ActionJump, ETriggerEvent::Started, this, &AFRCharacter::Input_JumpStarted);
	EnhancedInput->BindAction(ActionJump, ETriggerEvent::Completed, this, &AFRCharacter::Input_JumpCompleted);
	EnhancedInput->BindAction(ActionSprint, ETriggerEvent::Started, this, &AFRCharacter::Input_SprintStarted);
	EnhancedInput->BindAction(ActionSprint, ETriggerEvent::Completed, this, &AFRCharacter::Input_SprintCompleted);
	EnhancedInput->BindAction(ActionCrouch, ETriggerEvent::Started, this, &AFRCharacter::Input_CrouchToggled);

	// Fire and aim need both edges of the key: Started to press, Completed to
	// release, which is what lets an automatic weapon keep firing.
	EnhancedInput->BindAction(ActionFire, ETriggerEvent::Started, this, &AFRCharacter::Input_FireStarted);
	EnhancedInput->BindAction(ActionFire, ETriggerEvent::Completed, this, &AFRCharacter::Input_FireCompleted);
	EnhancedInput->BindAction(ActionAim, ETriggerEvent::Started, this, &AFRCharacter::Input_AimStarted);
	EnhancedInput->BindAction(ActionAim, ETriggerEvent::Completed, this, &AFRCharacter::Input_AimCompleted);

	EnhancedInput->BindAction(ActionReload, ETriggerEvent::Started, this, &AFRCharacter::Input_Reload);
	EnhancedInput->BindAction(ActionNextWeapon, ETriggerEvent::Started, this, &AFRCharacter::Input_NextWeapon);
	EnhancedInput->BindAction(ActionPreviousWeapon, ETriggerEvent::Started, this, &AFRCharacter::Input_PreviousWeapon);
	EnhancedInput->BindAction(ActionWeaponSlotOne, ETriggerEvent::Started, this, &AFRCharacter::Input_WeaponSlotOne);
	EnhancedInput->BindAction(ActionWeaponSlotTwo, ETriggerEvent::Started, this, &AFRCharacter::Input_WeaponSlotTwo);
	EnhancedInput->BindAction(ActionWeaponSlotThree, ETriggerEvent::Started, this, &AFRCharacter::Input_WeaponSlotThree);
}

void AFRCharacter::Input_Move(const FInputActionValue& Value)
{
	const FVector2D Axis = Value.Get<FVector2D>();
	if (!Controller || Axis.IsNearlyZero())
	{
		return;
	}

	// Movement follows the yaw of the view only. Pitch is ignored so looking at
	// the floor does not push the character into the ground.
	const FRotator YawOnly(0.0f, Controller->GetControlRotation().Yaw, 0.0f);
	const FRotationMatrix RotationMatrix(YawOnly);

	AddMovementInput(RotationMatrix.GetUnitAxis(EAxis::X), Axis.Y);
	AddMovementInput(RotationMatrix.GetUnitAxis(EAxis::Y), Axis.X);
}

void AFRCharacter::Input_Look(const FInputActionValue& Value)
{
	const FVector2D Axis = Value.Get<FVector2D>();
	if (Axis.IsNearlyZero())
	{
		return;
	}

	// Aiming narrows the field of view, so the same mouse movement would sweep
	// across far more of the world. Scaling the sensitivity down keeps the feel
	// of the aim consistent with the hip.
	const float Sensitivity = CachedMouseSensitivity * (bIsAiming ? CachedAimSensitivityScale : 1.0f);
	const float VerticalSign = bCachedInvertLookY ? -1.0f : 1.0f;

	AddControllerYawInput(Axis.X * Sensitivity);
	AddControllerPitchInput(Axis.Y * Sensitivity * VerticalSign);
}

void AFRCharacter::Input_JumpStarted()
{
	Jump();
}

void AFRCharacter::Input_JumpCompleted()
{
	StopJumping();
}

void AFRCharacter::Input_SprintStarted()
{
	bWantsToSprint = true;
	UpdateMovementSpeed();
}

void AFRCharacter::Input_SprintCompleted()
{
	bWantsToSprint = false;
	UpdateMovementSpeed();
}

void AFRCharacter::Input_CrouchToggled()
{
	if (bIsCrouched)
	{
		UnCrouch();
	}
	else
	{
		Crouch();
	}

	UpdateMovementSpeed();
}

void AFRCharacter::UpdateMovementSpeed()
{
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	if (!Movement)
	{
		return;
	}

	// Aiming beats sprinting: a player holding both keys is aiming, and walks at
	// the slower, steadier pace that goes with it.
	if (bIsAiming)
	{
		Movement->MaxWalkSpeed = AimWalkSpeed;
	}
	else if (bWantsToSprint && !bIsCrouched)
	{
		Movement->MaxWalkSpeed = SprintSpeed;
	}
	else
	{
		Movement->MaxWalkSpeed = WalkSpeed;
	}
}

// -- Weapon loadout ----------------------------------------------------------

void AFRCharacter::SpawnLoadout()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	if (WeaponClasses.Num() == 0)
	{
		UE_LOG(LogFiringRange, Warning, TEXT("Character has an empty weapon loadout."));
		return;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.Instigator = this;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	for (const TSubclassOf<AFRWeaponBase>& WeaponClass : WeaponClasses)
	{
		if (!WeaponClass)
		{
			continue;
		}

		AFRWeaponBase* Weapon = World->SpawnActor<AFRWeaponBase>(WeaponClass, GetActorTransform(), SpawnParams);
		if (!Weapon)
		{
			continue;
		}

		// Every weapon starts holstered. Equipping one is what makes it visible.
		Weapon->OnUnequipped();
		Weapons.Add(Weapon);
	}

	EquipWeaponAtIndex(0);
}

AFRWeaponBase* AFRCharacter::GetActiveWeapon() const
{
	return Weapons.IsValidIndex(ActiveWeaponIndex) ? Weapons[ActiveWeaponIndex] : nullptr;
}

void AFRCharacter::EquipWeaponAtIndex(int32 Index)
{
	if (!Weapons.IsValidIndex(Index) || Index == ActiveWeaponIndex)
	{
		return;
	}

	if (AFRWeaponBase* Previous = GetActiveWeapon())
	{
		Previous->OnUnequipped();
	}

	ActiveWeaponIndex = Index;

	AFRWeaponBase* Current = Weapons[Index];
	Current->OnEquipped(this);

	OnActiveWeaponChanged.Broadcast(Current);
}

void AFRCharacter::EquipNextWeapon()
{
	if (Weapons.Num() < 2)
	{
		return;
	}

	EquipWeaponAtIndex((ActiveWeaponIndex + 1) % Weapons.Num());
}

void AFRCharacter::EquipPreviousWeapon()
{
	if (Weapons.Num() < 2)
	{
		return;
	}

	EquipWeaponAtIndex((ActiveWeaponIndex - 1 + Weapons.Num()) % Weapons.Num());
}

void AFRCharacter::ResetLoadout()
{
	ResetReserveAmmo();

	for (AFRWeaponBase* Weapon : Weapons)
	{
		if (Weapon)
		{
			Weapon->ResetToFullMagazine();
		}
	}

	EquipWeaponAtIndex(0);
}

// -- Combat input ------------------------------------------------------------

void AFRCharacter::Input_FireStarted()
{
	if (AFRWeaponBase* Weapon = GetActiveWeapon())
	{
		Weapon->StartFire();
	}
}

void AFRCharacter::Input_FireCompleted()
{
	if (AFRWeaponBase* Weapon = GetActiveWeapon())
	{
		Weapon->StopFire();
	}
}

void AFRCharacter::Input_AimStarted()
{
	bIsAiming = true;
	UpdateMovementSpeed();
}

void AFRCharacter::Input_AimCompleted()
{
	bIsAiming = false;
	UpdateMovementSpeed();
}

void AFRCharacter::Input_Reload()
{
	if (AFRWeaponBase* Weapon = GetActiveWeapon())
	{
		Weapon->StartReload();
	}
}

void AFRCharacter::Input_NextWeapon()
{
	EquipNextWeapon();
}

void AFRCharacter::Input_PreviousWeapon()
{
	EquipPreviousWeapon();
}

void AFRCharacter::Input_WeaponSlotOne()
{
	EquipWeaponAtIndex(0);
}

void AFRCharacter::Input_WeaponSlotTwo()
{
	EquipWeaponAtIndex(1);
}

void AFRCharacter::Input_WeaponSlotThree()
{
	EquipWeaponAtIndex(2);
}
