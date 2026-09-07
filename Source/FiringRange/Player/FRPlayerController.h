// Copyright zutemiss & dshadykh. Educational project.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"

#include "FRPlayerController.generated.h"

class SFRPauseMenu;
class SWidget;
class UInputAction;
class UInputMappingContext;

/**
 * Player controller used on the firing range map.
 *
 * It owns everything that must outlive the pawn: the input mode currently in
 * effect, the pause state, and the camera pitch limits. The pause key is bound
 * here rather than on the character for exactly that reason - a controller is
 * never destroyed while the level is running.
 */
UCLASS()
class FIRINGRANGE_API AFRPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AFRPlayerController();

	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

	/** Hides the cursor and routes every key to the gameplay bindings. */
	void SetGameplayInputMode();

	/**
	 * Shows the cursor and lets Slate receive the mouse.
	 *
	 * Game input stays alive on purpose so the pause key can still close the
	 * menu that requested this mode.
	 */
	void SetMenuInputMode(TSharedPtr<SWidget> WidgetToFocus);

	/** Opens the pause menu when it is closed, closes it when it is open. */
	void TogglePauseMenu();

	/** Pauses the world and shows the pause menu. */
	void OpenPauseMenu();

	/** Closes the pause menu and resumes the world. */
	void ClosePauseMenu();

	bool IsPauseMenuOpen() const { return bPauseMenuOpen; }

	/** Priority of the controller context. Higher than the pawn context so pause always wins. */
	static constexpr int32 ControllerContextPriority = 10;

	/** Priority used by the pawn for its movement and weapon bindings. */
	static constexpr int32 PawnContextPriority = 0;

protected:
	/**
	 * Called right after the world has been paused.
	 *
	 * The pause menu widget is created by an override of this hook, which keeps
	 * the pause state machine independent from the user interface.
	 */
	virtual void OnPauseMenuOpened();

	/** Called right before the world is unpaused. Tears the widget down. */
	virtual void OnPauseMenuClosed();

	/** Map opened by the main menu entry of the pause screen. */
	UPROPERTY(EditDefaultsOnly, Category = "Firing Range|Menu")
	FName MainMenuLevelName = TEXT("/Game/Maps/MainMenu");

	/** Resets the session and closes the pause menu. */
	void HandleRestartRange();

	/** Leaves the range and returns to the front end. */
	void HandleQuitToMainMenu();

	/** Mapping context owned by the controller, added above the pawn context. */
	UPROPERTY(Transient)
	TObjectPtr<UInputMappingContext> ControllerInputContext;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> ActionPause;

	/** Starts a timed run without leaving the range. */
	UPROPERTY(Transient)
	TObjectPtr<UInputAction> ActionChallenge;

	/** Lowest pitch the camera may reach, in degrees. */
	UPROPERTY(EditDefaultsOnly, Category = "Firing Range|Camera")
	float ViewPitchMin = -80.0f;

	/** Highest pitch the camera may reach, in degrees. */
	UPROPERTY(EditDefaultsOnly, Category = "Firing Range|Camera")
	float ViewPitchMax = 80.0f;

private:
	void Input_Pause();
	void Input_StartChallenge();

	/** Builds the controller owned input action and mapping context. */
	void BuildControllerInput();

	bool bPauseMenuOpen = false;

	/** Live pause widget, valid only while the menu is open. */
	TSharedPtr<SFRPauseMenu> PauseMenuWidget;
};
