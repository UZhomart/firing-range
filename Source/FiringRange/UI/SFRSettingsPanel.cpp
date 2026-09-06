// Copyright zutemiss & dshadykh. Educational project.

#include "UI/SFRSettingsPanel.h"

#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

#include "Core/FRGameInstance.h"
#include "UI/FRUIStyle.h"

#define LOCTEXT_NAMESPACE "FiringRangeSettings"

float SFRSettingsPanel::ToSliderValue(float RawValue, float MinValue, float MaxValue)
{
	return FMath::Clamp((RawValue - MinValue) / FMath::Max(MaxValue - MinValue, KINDA_SMALL_NUMBER), 0.0f, 1.0f);
}

float SFRSettingsPanel::FromSliderValue(float SliderValue, float MinValue, float MaxValue)
{
	return MinValue + FMath::Clamp(SliderValue, 0.0f, 1.0f) * (MaxValue - MinValue);
}

void SFRSettingsPanel::Construct(const FArguments& InArgs)
{
	GameInstance = InArgs._GameInstance;
	OnBack = InArgs._OnBack;

	TWeakObjectPtr<UFRGameInstance> WeakGameInstance = GameInstance;

	ChildSlot
	[
		SNew(SBorder)
		.BorderImage(FRUI::GetPanelBrush())
		.Padding(FMargin(48.0f, 40.0f))
		[
			SNew(SVerticalBox)

			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Center)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("SettingsTitle", "SETTINGS"))
				.Font(FRUI::GetHeadingFont())
				.ColorAndOpacity(FSlateColor(FRUI::AccentColor))
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Center)
			.Padding(0.0f, 12.0f, 0.0f, 28.0f)
			[
				FRUI::MakeAccentRule(90.0f)
			]

			// ----- Mouse sensitivity -----------------------------------------
			// The setting the brief asks for by name. It is written straight into
			// the game instance, which persists it and tells the character to
			// pick the new value up immediately.
			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Center)
			.Padding(0.0f, 0.0f, 0.0f, 24.0f)
			[
				FRUI::MakeSliderRow(
					LOCTEXT("MouseSensitivity", "Mouse sensitivity"),
					TAttribute<float>::Create(TAttribute<float>::FGetter::CreateLambda([WeakGameInstance]() -> float
					{
						const float Raw = WeakGameInstance.IsValid() ? WeakGameInstance->GetMouseSensitivity() : 1.0f;
						return ToSliderValue(Raw, MinSensitivity, MaxSensitivity);
					})),
					FOnFloatValueChanged::CreateLambda([WeakGameInstance](float NewValue)
					{
						if (WeakGameInstance.IsValid())
						{
							WeakGameInstance->SetMouseSensitivity(FromSliderValue(NewValue, MinSensitivity, MaxSensitivity));
						}
					}),
					TAttribute<FText>::Create(TAttribute<FText>::FGetter::CreateLambda([WeakGameInstance]() -> FText
					{
						const float Raw = WeakGameInstance.IsValid() ? WeakGameInstance->GetMouseSensitivity() : 1.0f;
						return FText::FromString(FString::Printf(TEXT("%.2f"), Raw));
					})))
			]

			// ----- Aim sensitivity -------------------------------------------
			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Center)
			.Padding(0.0f, 0.0f, 0.0f, 24.0f)
			[
				FRUI::MakeSliderRow(
					LOCTEXT("AimSensitivity", "Sensitivity while aiming"),
					TAttribute<float>::Create(TAttribute<float>::FGetter::CreateLambda([WeakGameInstance]() -> float
					{
						const float Raw = WeakGameInstance.IsValid() ? WeakGameInstance->GetAimSensitivityScale() : 0.55f;
						return ToSliderValue(Raw, MinAimScale, MaxAimScale);
					})),
					FOnFloatValueChanged::CreateLambda([WeakGameInstance](float NewValue)
					{
						if (WeakGameInstance.IsValid())
						{
							WeakGameInstance->SetAimSensitivityScale(FromSliderValue(NewValue, MinAimScale, MaxAimScale));
						}
					}),
					TAttribute<FText>::Create(TAttribute<FText>::FGetter::CreateLambda([WeakGameInstance]() -> FText
					{
						const float Raw = WeakGameInstance.IsValid() ? WeakGameInstance->GetAimSensitivityScale() : 0.55f;
						return FText::FromString(FString::Printf(TEXT("%d %%"), FMath::RoundToInt(Raw * 100.0f)));
					})))
			]

			// ----- Invert vertical look --------------------------------------
			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Center)
			.Padding(0.0f, 0.0f, 0.0f, 18.0f)
			[
				SNew(SBox)
				.WidthOverride(440.0f)
				[
					SNew(SHorizontalBox)

					+ SHorizontalBox::Slot()
					.FillWidth(1.0f)
					.VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("InvertLook", "Invert vertical look"))
						.Font(FRUI::GetBodyFont())
						.ColorAndOpacity(FSlateColor(FRUI::TextColor))
					]

					+ SHorizontalBox::Slot()
					.AutoWidth()
					[
						SNew(SBox)
						.WidthOverride(110.0f)
						.HeightOverride(38.0f)
						[
							SNew(SButton)
							.ButtonStyle(&FRUI::GetMenuButtonStyle())
							.HAlign(HAlign_Center)
							.VAlign(VAlign_Center)
							.OnClicked(this, &SFRSettingsPanel::HandleInvertClicked)
							[
								SNew(STextBlock)
								.Font(FRUI::GetBodyFont())
								.Text_Lambda([WeakGameInstance]() -> FText
								{
									const bool bInverted = WeakGameInstance.IsValid() && WeakGameInstance->GetInvertLookY();
									return bInverted ? LOCTEXT("On", "ON") : LOCTEXT("Off", "OFF");
								})
								.ColorAndOpacity_Lambda([WeakGameInstance]() -> FSlateColor
								{
									const bool bInverted = WeakGameInstance.IsValid() && WeakGameInstance->GetInvertLookY();
									return FSlateColor(bInverted ? FRUI::AccentColor : FRUI::MutedTextColor);
								})
							]
						]
					]
				]
			]

			// ----- Difficulty --------------------------------------------------
			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Center)
			.Padding(0.0f, 0.0f, 0.0f, 34.0f)
			[
				BuildDifficultyRow()
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Center)
			.Padding(0.0f, 0.0f, 0.0f, 12.0f)
			[
				FRUI::MakeMenuButton(
					LOCTEXT("ResetDefaults", "RESET TO DEFAULTS"),
					FOnClicked::CreateSP(this, &SFRSettingsPanel::HandleResetClicked),
					FRUI::MutedTextColor)
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Center)
			[
				FRUI::MakeMenuButton(
					LOCTEXT("Back", "BACK"),
					FOnClicked::CreateSP(this, &SFRSettingsPanel::HandleBackClicked))
			]
		]
	];
}

TSharedRef<SWidget> SFRSettingsPanel::BuildDifficultyRow()
{
	return SNew(SBox)
		.WidthOverride(440.0f)
		[
			SNew(SVerticalBox)

			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 0.0f, 0.0f, 8.0f)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("Difficulty", "Target difficulty"))
				.Font(FRUI::GetBodyFont())
				.ColorAndOpacity(FSlateColor(FRUI::TextColor))
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SHorizontalBox)

				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				.Padding(0.0f, 0.0f, 8.0f, 0.0f)
				[
					BuildDifficultyButton(EFRDifficulty::Easy, LOCTEXT("Easy", "EASY"))
				]

				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				.Padding(4.0f, 0.0f)
				[
					BuildDifficultyButton(EFRDifficulty::Normal, LOCTEXT("Normal", "NORMAL"))
				]

				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				.Padding(8.0f, 0.0f, 0.0f, 0.0f)
				[
					BuildDifficultyButton(EFRDifficulty::Hard, LOCTEXT("Hard", "HARD"))
				]
			]
		];
}

TSharedRef<SWidget> SFRSettingsPanel::BuildDifficultyButton(EFRDifficulty Difficulty, const FText& Label)
{
	TWeakObjectPtr<UFRGameInstance> WeakGameInstance = GameInstance;

	return SNew(SBox)
		.HeightOverride(42.0f)
		[
			SNew(SButton)
			.ButtonStyle(&FRUI::GetMenuButtonStyle())
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			.OnClicked(FOnClicked::CreateSP(this, &SFRSettingsPanel::HandleDifficultyClicked, Difficulty))
			[
				SNew(STextBlock)
				.Text(Label)
				.Font(FRUI::GetBodyFont())

				// The chosen level is the one painted in the accent colour. Binding
				// the colour instead of setting it means the row needs no rebuild
				// when the choice changes.
				.ColorAndOpacity_Lambda([WeakGameInstance, Difficulty]() -> FSlateColor
				{
					const bool bSelected = WeakGameInstance.IsValid() && WeakGameInstance->GetDifficulty() == Difficulty;
					return FSlateColor(bSelected ? FRUI::AccentColor : FRUI::MutedTextColor);
				})
			]
		];
}

FReply SFRSettingsPanel::HandleDifficultyClicked(EFRDifficulty Difficulty)
{
	if (GameInstance.IsValid())
	{
		GameInstance->SetDifficulty(Difficulty);
		GameInstance->SaveSettings();
	}

	return FReply::Handled();
}

FReply SFRSettingsPanel::HandleInvertClicked()
{
	if (GameInstance.IsValid())
	{
		GameInstance->SetInvertLookY(!GameInstance->GetInvertLookY());
		GameInstance->SaveSettings();
	}

	return FReply::Handled();
}

FReply SFRSettingsPanel::HandleResetClicked()
{
	if (GameInstance.IsValid())
	{
		GameInstance->ResetSettingsToDefaults();
	}

	return FReply::Handled();
}

FReply SFRSettingsPanel::HandleBackClicked()
{
	// Sliders write on every movement but only ask for a save. Leaving the page
	// is the natural moment to actually put the file on disk.
	if (GameInstance.IsValid())
	{
		GameInstance->SaveSettings();
	}

	OnBack.ExecuteIfBound();

	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE
