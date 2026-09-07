// Copyright zutemiss & dshadykh. Educational project.

#include "UI/SFRPauseMenu.h"

#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SWidgetSwitcher.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

#include "Core/FRRangeGameState.h"
#include "UI/FRUIStyle.h"
#include "UI/SFRSettingsPanel.h"

#define LOCTEXT_NAMESPACE "FiringRangePauseMenu"

void SFRPauseMenu::Construct(const FArguments& InArgs)
{
	GameInstance = InArgs._GameInstance;
	RangeGameState = InArgs._RangeGameState;
	OnResume = InArgs._OnResume;
	OnRestart = InArgs._OnRestart;
	OnQuitToMainMenu = InArgs._OnQuitToMainMenu;

	ChildSlot
	[
		SNew(SOverlay)

		// Dimmer over the frozen world instead of an opaque background.
		+ SOverlay::Slot()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
		[
			SNew(SBorder)
			.BorderImage(FRUI::GetScrimBrush())
			.Padding(0.0f)
		]

		+ SOverlay::Slot()
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SAssignNew(PageSwitcher, SWidgetSwitcher)

			+ SWidgetSwitcher::Slot()
			[
				BuildEntryPage()
			]

			+ SWidgetSwitcher::Slot()
			[
				SNew(SFRSettingsPanel)
				.GameInstance(GameInstance)
				.OnBack(FSimpleDelegate::CreateSP(this, &SFRPauseMenu::ShowEntryPage))
			]
		]
	];
}

TSharedRef<SWidget> SFRPauseMenu::BuildEntryPage()
{
	TWeakObjectPtr<AFRRangeGameState> WeakState = RangeGameState;

	return SNew(SBorder)
		.BorderImage(FRUI::GetPanelBrush())
		.Padding(FMargin(60.0f, 44.0f))
		[
			SNew(SVerticalBox)

			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Center)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("Paused", "PAUSED"))
				.Font(FRUI::GetTitleFont())
				.ColorAndOpacity(FSlateColor(FRUI::TextColor))
			]

			// Where the run stands right now. Reading it from the game state keeps
			// the pause screen and the HUD showing exactly the same figures.
			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Center)
			.Padding(0.0f, 10.0f, 0.0f, 0.0f)
			[
				SNew(STextBlock)
				.Font(FRUI::GetBodyFont())
				.ColorAndOpacity(FSlateColor(FRUI::MutedTextColor))
				.Text_Lambda([WeakState]() -> FText
				{
					if (!WeakState.IsValid())
					{
						return FText::GetEmpty();
					}

					const FFRRangeStats& Stats = WeakState->GetStats();
					return FText::FromString(FString::Printf(
						TEXT("%d points     %s accuracy     %d / %d hits"),
						Stats.Score, *Stats.GetAccuracyText(), Stats.ShotsHit, Stats.ShotsFired));
				})
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Center)
			.Padding(0.0f, 20.0f, 0.0f, 34.0f)
			[
				FRUI::MakeAccentRule(110.0f)
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Center)
			.Padding(0.0f, 0.0f, 0.0f, 13.0f)
			[
				FRUI::MakeMenuButton(
					LOCTEXT("Resume", "RESUME"),
					FOnClicked::CreateSP(this, &SFRPauseMenu::HandleResumeClicked),
					FRUI::AccentColor)
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Center)
			.Padding(0.0f, 0.0f, 0.0f, 13.0f)
			[
				FRUI::MakeMenuButton(
					LOCTEXT("Settings", "SETTINGS"),
					FOnClicked::CreateSP(this, &SFRPauseMenu::HandleSettingsClicked))
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Center)
			.Padding(0.0f, 0.0f, 0.0f, 13.0f)
			[
				FRUI::MakeMenuButton(
					LOCTEXT("Restart", "RESTART RANGE"),
					FOnClicked::CreateSP(this, &SFRPauseMenu::HandleRestartClicked))
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Center)
			[
				FRUI::MakeMenuButton(
					LOCTEXT("QuitToMenu", "MAIN MENU"),
					FOnClicked::CreateSP(this, &SFRPauseMenu::HandleQuitClicked),
					FRUI::DangerColor)
			]
		];
}

void SFRPauseMenu::ShowEntryPage()
{
	if (PageSwitcher.IsValid())
	{
		PageSwitcher->SetActiveWidgetIndex(EntryPageIndex);
	}
}

FReply SFRPauseMenu::HandleResumeClicked()
{
	OnResume.ExecuteIfBound();
	return FReply::Handled();
}

FReply SFRPauseMenu::HandleSettingsClicked()
{
	if (PageSwitcher.IsValid())
	{
		PageSwitcher->SetActiveWidgetIndex(SettingsPageIndex);
	}

	return FReply::Handled();
}

FReply SFRPauseMenu::HandleRestartClicked()
{
	OnRestart.ExecuteIfBound();
	return FReply::Handled();
}

FReply SFRPauseMenu::HandleQuitClicked()
{
	OnQuitToMainMenu.ExecuteIfBound();
	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE
