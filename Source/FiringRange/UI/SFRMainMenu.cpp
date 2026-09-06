// Copyright zutemiss & dshadykh. Educational project.

#include "UI/SFRMainMenu.h"

#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SWidgetSwitcher.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

#include "Core/FRGameInstance.h"
#include "UI/FRUIStyle.h"
#include "UI/SFRSettingsPanel.h"

#define LOCTEXT_NAMESPACE "FiringRangeMainMenu"

void SFRMainMenu::Construct(const FArguments& InArgs)
{
	GameInstance = InArgs._GameInstance;
	OnStartGame = InArgs._OnStartGame;
	OnQuitGame = InArgs._OnQuitGame;

	ChildSlot
	[
		SNew(SOverlay)

		// A solid ground so the menu does not depend on anything being in the map
		// behind it. The menu level can stay completely empty.
		+ SOverlay::Slot()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
		[
			SNew(SBorder)
			.BorderImage(FRUI::GetBackgroundBrush())
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
				.OnBack(FSimpleDelegate::CreateSP(this, &SFRMainMenu::ShowEntryPage))
			]
		]
	];
}

TSharedRef<SWidget> SFRMainMenu::BuildEntryPage()
{
	TWeakObjectPtr<UFRGameInstance> WeakGameInstance = GameInstance;

	return SNew(SBorder)
		.BorderImage(FRUI::GetPanelBrush())
		.Padding(FMargin(64.0f, 52.0f))
		[
			SNew(SVerticalBox)

			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Center)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("Title", "FIRING RANGE"))
				.Font(FRUI::GetTitleFont())
				.ColorAndOpacity(FSlateColor(FRUI::TextColor))
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Center)
			.Padding(0.0f, 6.0f, 0.0f, 0.0f)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("Subtitle", "MARKSMANSHIP TRAINING"))
				.Font(FRUI::GetSmallFont())
				.ColorAndOpacity(FSlateColor(FRUI::MutedTextColor))
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Center)
			.Padding(0.0f, 18.0f, 0.0f, 40.0f)
			[
				FRUI::MakeAccentRule(140.0f)
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Center)
			.Padding(0.0f, 0.0f, 0.0f, 14.0f)
			[
				FRUI::MakeMenuButton(
					LOCTEXT("StartGame", "START GAME"),
					FOnClicked::CreateSP(this, &SFRMainMenu::HandleStartClicked),
					FRUI::AccentColor)
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Center)
			.Padding(0.0f, 0.0f, 0.0f, 14.0f)
			[
				FRUI::MakeMenuButton(
					LOCTEXT("Settings", "SETTINGS"),
					FOnClicked::CreateSP(this, &SFRMainMenu::HandleSettingsClicked))
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Center)
			[
				FRUI::MakeMenuButton(
					LOCTEXT("Quit", "QUIT"),
					FOnClicked::CreateSP(this, &SFRMainMenu::HandleQuitClicked),
					FRUI::DangerColor)
			]

			// Personal best, read live so it is already updated when the player
			// comes back from a challenge run.
			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Center)
			.Padding(0.0f, 38.0f, 0.0f, 0.0f)
			[
				SNew(STextBlock)
				.Font(FRUI::GetSmallFont())
				.ColorAndOpacity(FSlateColor(FRUI::MutedTextColor))
				.Text_Lambda([WeakGameInstance]() -> FText
				{
					if (!WeakGameInstance.IsValid() || WeakGameInstance->GetBestChallengeScore() <= 0)
					{
						return LOCTEXT("NoRecord", "No challenge run recorded yet");
					}

					return FText::FromString(FString::Printf(
						TEXT("Best challenge: %d points at %.1f %% accuracy"),
						WeakGameInstance->GetBestChallengeScore(),
						WeakGameInstance->GetBestChallengeAccuracy() * 100.0f));
				})
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Center)
			.Padding(0.0f, 8.0f, 0.0f, 0.0f)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("Authors", "zutemiss  and  dshadykh"))
				.Font(FRUI::GetSmallFont())
				.ColorAndOpacity(FSlateColor(FLinearColor(0.35f, 0.38f, 0.43f, 1.0f)))
			]
		];
}

void SFRMainMenu::ShowEntryPage()
{
	if (PageSwitcher.IsValid())
	{
		PageSwitcher->SetActiveWidgetIndex(EntryPageIndex);
	}
}

FReply SFRMainMenu::HandleStartClicked()
{
	OnStartGame.ExecuteIfBound();
	return FReply::Handled();
}

FReply SFRMainMenu::HandleSettingsClicked()
{
	if (PageSwitcher.IsValid())
	{
		PageSwitcher->SetActiveWidgetIndex(SettingsPageIndex);
	}

	return FReply::Handled();
}

FReply SFRMainMenu::HandleQuitClicked()
{
	OnQuitGame.ExecuteIfBound();
	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE
