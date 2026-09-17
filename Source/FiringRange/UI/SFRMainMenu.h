// Copyright zutemiss & dshadykh. Educational project.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"

class SWidgetSwitcher;
class UFRGameInstance;

/**
 * Front end of the game, shown on a map of its own.
 *
 * The widget owns two pages inside a switcher: the entry list and the settings
 * panel. Keeping the settings inside the main menu widget rather than swapping
 * whole widgets in the controller means the transition is one index change, and
 * the menu never has to be torn down and rebuilt.
 */
class SFRMainMenu : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SFRMainMenu) {}
		/** Game instance the settings page edits and the record line reads. */
		SLATE_ARGUMENT(TWeakObjectPtr<UFRGameInstance>, GameInstance)

		/** Invoked by the start button. The controller opens the range map. */
		SLATE_EVENT(FSimpleDelegate, OnStartGame)

		/** Invoked by the quit button. The controller closes the game. */
		SLATE_EVENT(FSimpleDelegate, OnQuitGame)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	/**
	 * Menus accept keyboard focus.
	 *
	 * A compound widget refuses focus by default, and the controller hands focus
	 * to the menu when it switches the input mode. Without this override the
	 * engine logs an error and keyboard navigation of the menu does not work.
	 */
	virtual bool SupportsKeyboardFocus() const override { return true; }

private:
	/** Entry list: start, settings, quit. */
	TSharedRef<SWidget> BuildEntryPage();

	FReply HandleStartClicked();
	FReply HandleSettingsClicked();
	FReply HandleQuitClicked();

	/** Returns the menu to the entry page. Passed to the settings panel. */
	void ShowEntryPage();

	TSharedPtr<SWidgetSwitcher> PageSwitcher;
	TWeakObjectPtr<UFRGameInstance> GameInstance;

	FSimpleDelegate OnStartGame;
	FSimpleDelegate OnQuitGame;

	static constexpr int32 EntryPageIndex = 0;
	static constexpr int32 SettingsPageIndex = 1;
};
