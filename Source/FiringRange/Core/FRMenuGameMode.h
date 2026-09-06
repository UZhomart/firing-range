// Copyright zutemiss & dshadykh. Educational project.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"

#include "FRMenuGameMode.generated.h"

/**
 * Game mode of the main menu map.
 *
 * The brief asks for the menu to be a level of its own rather than a widget laid
 * over the range, and this class is what makes that level behave like a menu: no
 * weapons, no scoring, no HUD, only a controller that shows the front end.
 *
 * It is referenced by name from Config/DefaultEngine.ini as the global default
 * game mode, so the game boots into the menu without any Blueprint involved.
 */
UCLASS()
class FIRINGRANGE_API AFRMenuGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AFRMenuGameMode();
};
