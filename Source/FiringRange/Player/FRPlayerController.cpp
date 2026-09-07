// Copyright zutemiss & dshadykh. Educational project.

#include "Player/FRPlayerController.h"

#include "Camera/PlayerCameraManager.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputMappingContext.h"

#include "Core/FRGameInstance.h"
#include "Core/FRRangeGameMode.h"
#include "Core/FRRangeGameState.h"
#include "Engine/GameViewportClient.h"
#include "FiringRange.h"
#include "Kismet/GameplayStatics.h"
#include "UI/SFRPauseMenu.h"

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

	// The challenge key lives on the controller rather than on the character,
	// because starting a run is a session level decision, not an action of the
	// body holding the weapon.
	ActionChallenge = NewObject<UInputAction>(this, TEXT("IA_StartChallenge"));
	ActionChallenge->ValueType = EInputActionValueType::Boolean;

	ControllerInputContext->MapKey(ActionChallenge, EKeys::T);
	ControllerInputContext->MapKey(ActionChallenge, EKeys::Gamepad_FaceButton_Top);
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
		EnhancedInput->BindAction(ActionChallenge, ETriggerEvent::Started, this, &AFRPlayerController::Input_StartChallenge);
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

void AFRPlayerController::Input_StartChallenge()
{
	// Pressing the key during a run has no effect: restarting a challenge by
	// accident in its last seconds would throw away the whole attempt.
	if (UWorld* World = GetWorld())
	{
		if (AFRRangeGameMode* GameMode = World->GetAuthGameMode<AFRRangeGameMode>())
		{
			if (!GameMode->IsChallengeRunning())
			{
				GameMode->StartTimedChallenge();
			}
		}
	}
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
	UWorld* World = GetWorld();
	UGameViewportClient* Viewport = World ? World->GetGameViewport() : nullptr;

	if (!Viewport || PauseMenuWidget.IsValid())
	{
		SetMenuInputMode(nullptr);
		return;
	}

	PauseMenuWidget = SNew(SFRPauseMenu)
		.GameInstance(UFRGameInstance::Get(this))
		.RangeGameState(World->GetGameState<AFRRangeGameState>())
		.OnResume(FSimpleDelegate::CreateUObject(this, &AFRPlayerController::ClosePauseMenu))
		.OnRestart(FSimpleDelegate::CreateUObject(this, &AFRPlayerController::HandleRestartRange))
		.OnQuitToMainMenu(FSimpleDelegate::CreateUObject(this, &AFRPlayerController::HandleQuitToMainMenu));

	// A high z order keeps the menu above the canvas HUD, which keeps drawing
	// underneath while the world is frozen.
	Viewport->AddViewportWidgetContent(PauseMenuWidget.ToSharedRef(), 20);

	SetMenuInputMode(PauseMenuWidget);
}

void AFRPlayerController::OnPauseMenuClosed()
{
	if (!PauseMenuWidget.IsValid())
	{
		return;
	}

	UWorld* World = GetWorld();
	if (UGameViewportClient* Viewport = World ? World->GetGameViewport() : nullptr)
	{
		Viewport->RemoveViewportWidgetContent(PauseMenuWidget.ToSharedRef());
	}

	PauseMenuWidget.Reset();
}

void AFRPlayerController::HandleRestartRange()
{
	// The rules of a restart belong to the game mode. The controller only asks
	// for one and then gets out of the way.
	if (UWorld* World = GetWorld())
	{
		if (AFRRangeGameMode* GameMode = World->GetAuthGameMode<AFRRangeGameMode>())
		{
			GameMode->RestartRange();
		}
	}

	ClosePauseMenu();
}

void AFRPlayerController::HandleQuitToMainMenu()
{
	// Unpause before travelling: a world left paused on the way out would arrive
	// at the menu map with its time dilation still frozen.
	ClosePauseMenu();

	UE_LOG(LogFiringRange, Log, TEXT("Returning to %s."), *MainMenuLevelName.ToString());
	UGameplayStatics::OpenLevel(this, MainMenuLevelName);
}
