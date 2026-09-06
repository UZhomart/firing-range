// Copyright zutemiss & dshadykh. Educational project.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"

#include "Core/FRTypes.h"

class UFRGameInstance;

/**
 * Settings page shared by the main menu and by the pause menu.
 *
 * Writing it once and reusing it is what makes the two places agree: a
 * sensitivity changed mid session on the pause screen is the same value the main
 * menu shows afterwards, because both edit the same game instance.
 *
 * Every control is bound to the game instance rather than to a local copy, so
 * there is no apply step and nothing to keep in sync. Moving a slider changes
 * the setting, the game instance saves it, and anything listening reacts at once.
 */
class SFRSettingsPanel : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SFRSettingsPanel) {}
		/** Game instance the controls read from and write to. */
		SLATE_ARGUMENT(TWeakObjectPtr<UFRGameInstance>, GameInstance)

		/** Invoked when the back button is pressed. */
		SLATE_EVENT(FSimpleDelegate, OnBack)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	/** Builds the row of three difficulty buttons. */
	TSharedRef<SWidget> BuildDifficultyRow();

	/** Builds one difficulty button, highlighted while it is the chosen one. */
	TSharedRef<SWidget> BuildDifficultyButton(EFRDifficulty Difficulty, const FText& Label);

	FReply HandleDifficultyClicked(EFRDifficulty Difficulty);
	FReply HandleInvertClicked();
	FReply HandleResetClicked();
	FReply HandleBackClicked();

	/** Maps a raw setting onto the 0..1 range a slider works in, and back. */
	static float ToSliderValue(float RawValue, float MinValue, float MaxValue);
	static float FromSliderValue(float SliderValue, float MinValue, float MaxValue);

	TWeakObjectPtr<UFRGameInstance> GameInstance;
	FSimpleDelegate OnBack;

	/** Bounds of the mouse sensitivity setting, mirrored from UFRSaveGame. */
	static constexpr float MinSensitivity = 0.10f;
	static constexpr float MaxSensitivity = 4.0f;

	/** Bounds of the aim sensitivity multiplier. */
	static constexpr float MinAimScale = 0.10f;
	static constexpr float MaxAimScale = 1.0f;
};
