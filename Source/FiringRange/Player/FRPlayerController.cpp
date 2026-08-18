// Copyright zutemiss & dshadykh. Educational project.

#include "Player/FRPlayerController.h"

#include "Camera/PlayerCameraManager.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputMappingContext.h"

#include "FiringRange.h"

AFRPlayerController::AFRPlayerController()
{
	bShowMouseCursor = false;
	bEnableClickEvents = true;
	bEnableMouseOverEvents = true;
}

void AFRPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// Without a pitch clamp the player can look straight up and keep rotating,
	// which flips the view upside down and makes aiming impossible.
	if (PlayerCameraManager)
	{
		PlayerCameraManager->ViewPitchMin = ViewPitchMin;
		PlayerCameraManager->ViewPitchMax = ViewPitchMax;
	}

	SetGameplayInputMode();
}

void AFRPlayerController::BuildControllerInput()
{
	if (ControllerInputContext)
	{
		return;
	}

	ControllerInputContext = NewObject<UInputMappingContext>(this, TEXT("IMC_FiringRangeController"));

	ActionPause = NewObject<UInputAction>(this, TEXT("IA_Pause"));
	ActionPause->ValueType = EInputActionValueType::Boolean;

	ControllerInputContext->MapKey(ActionPause, EKeys::Escape);
	ControllerInputContext->MapKey(ActionPause, EKeys::Gamepad_Special_Right);
}

void AFRPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	BuildControllerInput();

	if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
		ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		Subsystem->AddMappingContext(ControllerInputContext, ControllerContextPriority);
	}

	if (UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(InputComponent))
	{
		EnhancedInput->BindAction(ActionPause, ETriggerEvent::Started, this, &AFRPlayerController::Input_Pause);
	}
	else
	{
		UE_LOG(LogFiringRange, Error,
			TEXT("Controller input component is not an EnhancedInputComponent. Pause will not work."));
	}
}

void AFRPlayerController::SetGameplayInputMode()
{
	FInputModeGameOnly InputMode;
	SetInputMode(InputMode);

	bShowMouseCursor = false;
}

void AFRPlayerController::SetMenuInputMode(TSharedPtr<SWidget> WidgetToFocus)
{
	// GameAndUI rather than UIOnly: the pause key has to keep reaching the
	// controller so that the same key that opened the menu also closes it.
	FInputModeGameAndUI InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	InputMode.SetHideCursorDuringCapture(false);

	if (WidgetToFocus.IsValid())
	{
		InputMode.SetWidgetToFocus(WidgetToFocus);
	}

	SetInputMode(InputMode);

	bShowMouseCursor = true;
}

void AFRPlayerController::Input_Pause()
{
	TogglePauseMenu();
}

void AFRPlayerController::TogglePauseMenu()
{
	if (bPauseMenuOpen)
	{
		ClosePauseMenu();
	}
	else
	{
		OpenPauseMenu();
	}
}

void AFRPlayerController::OpenPauseMenu()
{
	if (bPauseMenuOpen)
	{
		return;
	}

	bPauseMenuOpen = true;

	// SetPause freezes actor ticking, timers and physics, so targets stop moving
	// and reload timers stop counting down while the menu is open.
	SetPause(true);

	OnPauseMenuOpened();
}

void AFRPlayerController::ClosePauseMenu()
{
	if (!bPauseMenuOpen)
	{
		return;
	}

	OnPauseMenuClosed();

	bPauseMenuOpen = false;

	SetPause(false);
	SetGameplayInputMode();
}

void AFRPlayerController::OnPauseMenuOpened()
{
	// Base implementation only switches the input mode. The widget itself is
	// created by the override that knows about the pause menu.
	SetMenuInputMode(nullptr);
}

void AFRPlayerController::OnPauseMenuClosed()
{
}
