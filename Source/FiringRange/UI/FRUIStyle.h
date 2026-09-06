// Copyright zutemiss & dshadykh. Educational project.

#pragma once

#include "CoreMinimal.h"
#include "Framework/SlateDelegates.h"
#include "Brushes/SlateColorBrush.h"
#include "Styling/SlateBrush.h"
#include "Styling/SlateTypes.h"

class SWidget;

/**
 * Shared look of every menu in the project.
 *
 * None of it comes from an asset. Solid colours are FSlateColorBrush values,
 * which Slate renders without a texture, and the type is the Roboto face that is
 * compiled into the engine itself. That is what lets three full menus exist in a
 * project with no content folder at all.
 *
 * Brushes are returned as pointers to function local statics so they are built
 * once, on first use, after Slate is running.
 */
namespace FRUI
{
	/** Deep, slightly blue background of the menu maps. */
	extern const FLinearColor BackgroundColor;

	/** Panel the menu content sits on. */
	extern const FLinearColor PanelColor;

	/** Dimmer laid over the world behind the pause menu. */
	extern const FLinearColor ScrimColor;

	/** Primary text. */
	extern const FLinearColor TextColor;

	/** Secondary text: hints, values, footers. */
	extern const FLinearColor MutedTextColor;

	/** Highlight used by titles, hovered buttons and the accent rule. */
	extern const FLinearColor AccentColor;

	/** Destructive action, such as leaving the range. */
	extern const FLinearColor DangerColor;

	const FSlateBrush* GetBackgroundBrush();
	const FSlateBrush* GetPanelBrush();
	const FSlateBrush* GetScrimBrush();
	const FSlateBrush* GetAccentBrush();
	const FSlateBrush* GetTransparentBrush();

	FSlateFontInfo GetTitleFont();
	FSlateFontInfo GetHeadingFont();
	FSlateFontInfo GetBodyFont();
	FSlateFontInfo GetSmallFont();

	/** Button style shared by every menu entry. */
	const FButtonStyle& GetMenuButtonStyle();

	/** Slider style used by the sensitivity controls. */
	const FSliderStyle& GetSliderStyle();

	/** Builds one full width menu entry. */
	TSharedRef<SWidget> MakeMenuButton(const FText& Label, FOnClicked OnClicked, const FLinearColor& LabelColor = TextColor);

	/** Builds a labelled slider row with a live numeric readout. */
	TSharedRef<SWidget> MakeSliderRow(
		const FText& Label,
		TAttribute<float> Value,
		FOnFloatValueChanged OnValueChanged,
		TAttribute<FText> ValueText);

	/** Thin horizontal rule in the accent colour. */
	TSharedRef<SWidget> MakeAccentRule(float Width = 120.0f);
}
