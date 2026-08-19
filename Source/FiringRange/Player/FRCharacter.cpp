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
}

void AFRCharacter::BeginPlay()
{
	Super::BeginPlay();

	RefreshLookSettings();

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

	const float Sensitivity = CachedMouseSensitivity;
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

	Movement->MaxWalkSpeed = (bWantsToSprint && !bIsCrouched) ? SprintSpeed : WalkSpeed;
}
