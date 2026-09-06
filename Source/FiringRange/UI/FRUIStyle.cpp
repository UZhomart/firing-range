// Copyright zutemiss & dshadykh. Educational project.

#include "UI/FRUIStyle.h"

#include "Brushes/SlateColorBrush.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SSlider.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

namespace FRUI
{
	const FLinearColor BackgroundColor(0.016f, 0.020f, 0.030f, 1.0f);
	const FLinearColor PanelColor(0.035f, 0.042f, 0.058f, 0.96f);
	const FLinearColor ScrimColor(0.0f, 0.0f, 0.0f, 0.72f);
	const FLinearColor TextColor(0.92f, 0.94f, 0.97f, 1.0f);
	const FLinearColor MutedTextColor(0.58f, 0.62f, 0.68f, 1.0f);
	const FLinearColor AccentColor(1.0f, 0.72f, 0.16f, 1.0f);
	const FLinearColor DangerColor(0.92f, 0.36f, 0.28f, 1.0f);
}

const FSlateBrush* FRUI::GetBackgroundBrush()
{
	// A function local static is built on first use, which is safely after Slate
	// has started, and lives for the rest of the process.
	static const FSlateColorBrush Brush(BackgroundColor);
	return &Brush;
}

const FSlateBrush* FRUI::GetPanelBrush()
{
	static const FSlateColorBrush Brush(PanelColor);
	return &Brush;
}

const FSlateBrush* FRUI::GetScrimBrush()
{
	static const FSlateColorBrush Brush(ScrimColor);
	return &Brush;
}

const FSlateBrush* FRUI::GetAccentBrush()
{
	static const FSlateColorBrush Brush(AccentColor);
	return &Brush;
}

const FSlateBrush* FRUI::GetTransparentBrush()
{
	static const FSlateColorBrush Brush(FLinearColor(0.0f, 0.0f, 0.0f, 0.0f));
	return &Brush;
}

FSlateFontInfo FRUI::GetTitleFont()
{
	// Roboto ships inside the engine, so no font asset has to be imported.
	return FCoreStyle::GetDefaultFontStyle("Bold", 46);
}

FSlateFontInfo FRUI::GetHeadingFont()
{
	return FCoreStyle::GetDefaultFontStyle("Bold", 24);
}

FSlateFontInfo FRUI::GetBodyFont()
{
	return FCoreStyle::GetDefaultFontStyle("Regular", 18);
}

FSlateFontInfo FRUI::GetSmallFont()
{
	return FCoreStyle::GetDefaultFontStyle("Regular", 14);
}

const FButtonStyle& FRUI::GetMenuButtonStyle()
{
	static FButtonStyle Style = []()
	{
		static const FSlateColorBrush Normal(FLinearColor(0.09f, 0.11f, 0.15f, 0.92f));
		static const FSlateColorBrush Hovered(FLinearColor(0.16f, 0.20f, 0.27f, 0.98f));
		static const FSlateColorBrush Pressed(FLinearColor(0.26f, 0.19f, 0.06f, 1.0f));
		static const FSlateColorBrush Disabled(FLinearColor(0.06f, 0.07f, 0.09f, 0.6f));

		FButtonStyle Built;
		Built.SetNormal(Normal);
		Built.SetHovered(Hovered);
		Built.SetPressed(Pressed);
		Built.SetDisabled(Disabled);
		Built.SetNormalPadding(FMargin(20.0f, 12.0f));
		Built.SetPressedPadding(FMargin(20.0f, 13.0f, 20.0f, 11.0f));
		return Built;
	}();

	return Style;
}

const FSliderStyle& FRUI::GetSliderStyle()
{
	static FSliderStyle Style = []()
	{
		static const FSlateColorBrush Bar(FLinearColor(0.12f, 0.14f, 0.19f, 1.0f));
		static const FSlateColorBrush Thumb(AccentColor);

		FSliderStyle Built = FSliderStyle::GetDefault();
		Built.SetNormalBarImage(Bar);
		Built.SetHoveredBarImage(Bar);
		Built.SetDisabledBarImage(Bar);
		Built.SetNormalThumbImage(Thumb);
		Built.SetHoveredThumbImage(Thumb);
		Built.SetDisabledThumbImage(Thumb);
		Built.SetBarThickness(6.0f);
		return Built;
	}();

	return Style;
}

TSharedRef<SWidget> FRUI::MakeMenuButton(const FText& Label, FOnClicked OnClicked, const FLinearColor& LabelColor)
{
	return SNew(SBox)
		.WidthOverride(360.0f)
		.HeightOverride(56.0f)
		[
			SNew(SButton)
			.ButtonStyle(&GetMenuButtonStyle())
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			.OnClicked(OnClicked)
			[
				SNew(STextBlock)
				.Text(Label)
				.Font(GetHeadingFont())
				.ColorAndOpacity(FSlateColor(LabelColor))
			]
		];
}

TSharedRef<SWidget> FRUI::MakeSliderRow(
	const FText& Label,
	TAttribute<float> Value,
	FOnFloatValueChanged OnValueChanged,
	TAttribute<FText> ValueText)
{
	return SNew(SBox)
		.WidthOverride(440.0f)
		[
			SNew(SVerticalBox)

			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 0.0f, 0.0f, 6.0f)
			[
				SNew(SHorizontalBox)

				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(Label)
					.Font(GetBodyFont())
					.ColorAndOpacity(FSlateColor(TextColor))
				]

				// The numeric readout is bound rather than set, so it follows the
				// slider while it is being dragged without any update code.
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(ValueText)
					.Font(GetBodyFont())
					.ColorAndOpacity(FSlateColor(AccentColor))
				]
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SSlider)
				.Style(&GetSliderStyle())
				.Value(Value)
				.OnValueChanged(OnValueChanged)
			]
		];
}

TSharedRef<SWidget> FRUI::MakeAccentRule(float Width)
{
	return SNew(SBox)
		.WidthOverride(Width)
		.HeightOverride(3.0f)
		[
			SNew(SBorder)
			.BorderImage(GetAccentBrush())
			.Padding(0.0f)
		];
}
