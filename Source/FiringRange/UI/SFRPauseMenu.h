// Copyright zutemiss & dshadykh. Educational project.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"

class AFRRangeGameState;
class SWidgetSwitcher;
class UFRGameInstance;

/**
 * Pause screen of the range.
 *
 * It carries the three things the brief asks a pause menu for - resume, restart
 * and return to the main menu - plus the same settings panel the front end uses,
 * so sensitivity can be adjusted without leaving the session.
 *
 * The panel is laid over a dimmed copy of the world rather than over a solid
 * ground, which keeps the player oriented: the range is still there behind the
 * menu, frozen exactly where it was.
 */
class SFRPauseMenu : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SFRPauseMenu) {}
		SLATE_ARGUMENT(TWeakObjectPtr<UFRGameInstance>, GameInstance)

		/** Scoreboard shown in the summary line. */
		SLATE_ARGUMENT(TWeakObjectPtr<AFRRangeGameState>, RangeGameState)

		SLATE_EVENT(FSimpleDelegate, OnResume)
		SLATE_EVENT(FSimpleDelegate, OnRestart)
		SLATE_EVENT(FSimpleDelegate, OnQuitToMainMenu)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	TSharedRef<SWidget> BuildEntryPage();

	FReply HandleResumeClicked();
	FReply HandleSettingsClicked();
	FReply HandleRestartClicked();
	FReply HandleQuitClicked();

	void ShowEntryPage();

	TSharedPtr<SWidgetSwitcher> PageSwitcher;

	TWeakObjectPtr<UFRGameInstance> GameInstance;
	TWeakObjectPtr<AFRRangeGameState> RangeGameState;

	FSimpleDelegate OnResume;
	FSimpleDelegate OnRestart;
	FSimpleDelegate OnQuitToMainMenu;

	static constexpr int32 EntryPageIndex = 0;
	static constexpr int32 SettingsPageIndex = 1;
};
