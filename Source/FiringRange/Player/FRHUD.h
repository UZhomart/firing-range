// Copyright zutemiss & dshadykh. Educational project.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"

#include "Core/FRTypes.h"

#include "FRHUD.generated.h"

class AFRCharacter;
class AFRRangeGameState;
class AFRWeaponBase;
class UFont;

/** One floating "+50 BULLSEYE" label, fading out near the crosshair. */
USTRUCT()
struct FFRHitMarker
{
	GENERATED_BODY()

	EFRHitZone Zone = EFRHitZone::None;
	int32 Points = 0;
	float DistanceMetres = 0.0f;

	/** Seconds since the marker appeared. */
	float Age = 0.0f;

	/** Sideways offset so several markers in a row do not stack on one spot. */
	float HorizontalOffset = 0.0f;
};

/**
 * Head up display of the range, drawn entirely with the canvas.
 *
 * Everything here is drawn from code: no UMG widget, no texture and no font
 * asset is involved, which is what keeps the project free of binary content.
 * The HUD only reads. Scores come from the game state, ammunition comes from the
 * weapon in hand, and nothing on this class is allowed to change either.
 *
 * The crosshair is drawn at the exact centre of the screen because that is where
 * the bullet goes: the weapon traces forward from the camera and aims the muzzle
 * at whatever that trace found. Its gap is the current spread of the weapon,
 * projected into pixels, so an open crosshair is a truthful warning that the
 * shot may land anywhere inside it.
 */
UCLASS()
class FIRINGRANGE_API AFRHUD : public AHUD
{
	GENERATED_BODY()

public:
	AFRHUD();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void DrawHUD() override;

protected:
	// -- Sections --------------------------------------------------------------

	/** Dynamic crosshair, sized from the current weapon spread. */
	void DrawCrosshair();

	/** Score, accuracy and streak, in the top left corner. */
	void DrawScorePanel();

	/** Weapon name, magazine, reserve and reload progress, in the bottom right. */
	void DrawWeaponPanel();

	/** Floating labels for the hits that just landed. */
	void DrawHitMarkers();

	/** Countdown bar while a timed run is going, result panel once it ends. */
	void DrawChallengeBanner();

	/** Key reminders along the bottom edge. */
	void DrawControlHints();

	// -- Helpers ---------------------------------------------------------------

	/** Panel background with a thin outline, so text stays readable on any wall. */
	void DrawPanel(float X, float Y, float Width, float Height, const FLinearColor& Tint) const;

	/** Draws a label and returns the height it consumed. */
	float DrawLabel(const FString& Text, float X, float Y, const FLinearColor& Color, UFont* Font, float Scale = 1.0f);

	/** Horizontal progress bar used by the reload indicator. */
	void DrawProgressBar(float X, float Y, float Width, float Height, float Progress, const FLinearColor& Color) const;

	/** Subscribes to the game state once it exists. Safe to call every frame. */
	void EnsureGameStateBinding();

	/** Called by the game state whenever a hit scored. */
	void HandleHitRegistered(EFRHitZone Zone, float DistanceMetres, int32 Points);

	/** Character being played, or nullptr between respawns. */
	AFRCharacter* GetPlayerCharacter() const;

	/** Weapon in the hands of the player, or nullptr. */
	AFRWeaponBase* GetActiveWeapon() const;

	// -- Style -----------------------------------------------------------------

	UPROPERTY(EditDefaultsOnly, Category = "Firing Range|HUD")
	FLinearColor CrosshairColor = FLinearColor(0.95f, 0.97f, 1.0f, 0.9f);

	UPROPERTY(EditDefaultsOnly, Category = "Firing Range|HUD")
	FLinearColor PanelColor = FLinearColor(0.02f, 0.03f, 0.05f, 0.55f);

	UPROPERTY(EditDefaultsOnly, Category = "Firing Range|HUD")
	FLinearColor TextColor = FLinearColor(0.92f, 0.94f, 0.97f, 1.0f);

	UPROPERTY(EditDefaultsOnly, Category = "Firing Range|HUD")
	FLinearColor AccentColor = FLinearColor(1.0f, 0.73f, 0.18f, 1.0f);

	UPROPERTY(EditDefaultsOnly, Category = "Firing Range|HUD")
	FLinearColor WarningColor = FLinearColor(0.95f, 0.30f, 0.22f, 1.0f);

	/** Shortest half length of a crosshair arm, in pixels. */
	UPROPERTY(EditDefaultsOnly, Category = "Firing Range|HUD")
	float CrosshairArmLength = 9.0f;

	/** Smallest gap between the centre and an arm, in pixels. */
	UPROPERTY(EditDefaultsOnly, Category = "Firing Range|HUD")
	float CrosshairMinimumGap = 4.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Firing Range|HUD")
	float CrosshairThickness = 2.0f;

	/** Seconds a hit marker stays on screen. */
	UPROPERTY(EditDefaultsOnly, Category = "Firing Range|HUD")
	float HitMarkerLifetime = 1.1f;

private:
	/** Markers currently fading out. */
	TArray<FFRHitMarker> HitMarkers;

	/** Seconds left of the short flash drawn around the crosshair after a hit. */
	float CrosshairFlashTimer = 0.0f;

	/** Colour of that flash, taken from the zone that was struck. */
	FLinearColor CrosshairFlashColor = FLinearColor::White;

	/** Cached game state, resolved lazily because it may not exist on the first frame. */
	TWeakObjectPtr<AFRRangeGameState> CachedGameState;

	bool bBoundToGameState = false;
};
