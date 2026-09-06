// Copyright zutemiss & dshadykh. Educational project.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"

#include "FRMenuPlayerController.generated.h"

class SFRMainMenu;

/**
 * Controller of the main menu map.
 *
 * It builds the menu widget, puts it in the viewport and answers the two entries
 * that leave the menu: starting the range and closing the game. The settings
 * page is handled inside the widget, because nothing about it concerns the world.
 */
UCLASS()
class FIRINGRANGE_API AFRMenuPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AFRMenuPlayerController();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

protected:
	/** Map opened by the start button. */
	UPROPERTY(EditDefaultsOnly, Category = "Firing Range|Menu")
	FName RangeLevelName = TEXT("/Game/Maps/FiringRange");

	/** Creates the widget and hands input over to it. */
	void ShowMainMenu();

	/** Takes the widget out of the viewport. */
	void HideMainMenu();

	void HandleStartGame();
	void HandleQuitGame();

private:
	TSharedPtr<SFRMainMenu> MainMenuWidget;
};
