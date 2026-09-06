// Copyright zutemiss & dshadykh. Educational project.

#include "Core/FRMenuGameMode.h"

#include "GameFramework/HUD.h"
#include "GameFramework/SpectatorPawn.h"

#include "Core/FRMenuPlayerController.h"

AFRMenuGameMode::AFRMenuGameMode()
{
	PlayerControllerClass = AFRMenuPlayerController::StaticClass();

	// A spectator pawn gives the controller something to see through without
	// bringing any gameplay with it. The menu never reads input from the world,
	// so the pawn simply stands still.
	DefaultPawnClass = ASpectatorPawn::StaticClass();

	// The plain engine HUD draws nothing at all, which is exactly right here: the
	// menu is a Slate widget in the viewport, not a canvas drawing.
	HUDClass = AHUD::StaticClass();
}
