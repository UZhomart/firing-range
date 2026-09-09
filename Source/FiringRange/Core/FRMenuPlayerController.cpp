// Copyright zutemiss & dshadykh. Educational project.

#include "Core/FRMenuPlayerController.h"

#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"

#include "Core/FRGameInstance.h"
#include "FiringRange.h"
#include "UI/SFRMainMenu.h"

AFRMenuPlayerController::AFRMenuPlayerController()
{
	bShowMouseCursor = true;
	bEnableClickEvents = true;
	bEnableMouseOverEvents = true;
}

void AFRMenuPlayerController::BeginPlay()
{
	Super::BeginPlay();

	ShowMainMenu();
}

void AFRMenuPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	HideMainMenu();

	Super::EndPlay(EndPlayReason);
}

void AFRMenuPlayerController::ShowMainMenu()
{
	UWorld* World = GetWorld();
	UGameViewportClient* Viewport = World ? World->GetGameViewport() : nullptr;

	if (!Viewport || MainMenuWidget.IsValid())
	{
		return;
	}

	MainMenuWidget = SNew(SFRMainMenu)
		.GameInstance(UFRGameInstance::Get(this))
		.OnStartGame(FSimpleDelegate::CreateUObject(this, &AFRMenuPlayerController::HandleStartGame))
		.OnQuitGame(FSimpleDelegate::CreateUObject(this, &AFRMenuPlayerController::HandleQuitGame));

	Viewport->AddViewportWidgetContent(MainMenuWidget.ToSharedRef(), 10);

	// UIOnly on the menu map: there is no gameplay here for a key to reach, and
	// the cursor has to stay free for the buttons.
	FInputModeUIOnly InputMode;
	InputMode.SetWidgetToFocus(MainMenuWidget);
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(InputMode);

	bShowMouseCursor = true;
}

void AFRMenuPlayerController::HideMainMenu()
{
	if (!MainMenuWidget.IsValid())
	{
		return;
	}

	UWorld* World = GetWorld();
	if (UGameViewportClient* Viewport = World ? World->GetGameViewport() : nullptr)
	{
		Viewport->RemoveViewportWidgetContent(MainMenuWidget.ToSharedRef());
	}

	MainMenuWidget.Reset();
}

void AFRMenuPlayerController::HandleStartGame()
{
	// The widget is removed before the travel so nothing tries to draw a menu
	// over the level that is loading.
	HideMainMenu();

	// The game mode is passed in the travel URL rather than being read from the
	// World Settings of the map. That makes the range map work even when it was
	// created as a plain empty level and never configured by hand.
	const FString TravelOptions = TEXT("game=/Script/FiringRange.FRRangeGameMode");

	UE_LOG(LogFiringRange, Log, TEXT("Opening range level %s."), *RangeLevelName.ToString());
	UGameplayStatics::OpenLevel(this, RangeLevelName, true, TravelOptions);
}

void AFRMenuPlayerController::HandleQuitGame()
{
	// QuitGame is the portable way out. It does the right thing on Windows, Linux
	// and macOS, and it is also correct in the editor, where it stops play in
	// editor instead of closing the editor itself.
	UKismetSystemLibrary::QuitGame(this, this, EQuitPreference::Quit, false);
}
