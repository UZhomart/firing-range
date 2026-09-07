// Copyright zutemiss & dshadykh. Educational project.

#include "Player/FRHUD.h"

#include "Camera/CameraComponent.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"

#include "Core/FRRangeGameState.h"
#include "Player/FRCharacter.h"
#include "Weapons/FRSniperRifle.h"
#include "Weapons/FRWeaponBase.h"

AFRHUD::AFRHUD()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
}

void AFRHUD::BeginPlay()
{
	Super::BeginPlay();

	EnsureGameStateBinding();
}

void AFRHUD::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (AFRRangeGameState* RangeState = CachedGameState.Get())
	{
		RangeState->OnHitRegistered.RemoveAll(this);
	}

	Super::EndPlay(EndPlayReason);
}

void AFRHUD::EnsureGameStateBinding()
{
	if (bBoundToGameState)
	{
		return;
	}

	UWorld* World = GetWorld();
	AFRRangeGameState* RangeState = World ? World->GetGameState<AFRRangeGameState>() : nullptr;
	if (!RangeState)
	{
		// The game state is created by the game mode and may not exist yet on the
		// very first frames. Retrying costs nothing and removes an ordering rule.
		return;
	}

	CachedGameState = RangeState;
	RangeState->OnHitRegistered.AddUObject(this, &AFRHUD::HandleHitRegistered);
	bBoundToGameState = true;
}

AFRCharacter* AFRHUD::GetPlayerCharacter() const
{
	return GetOwningPawn() ? Cast<AFRCharacter>(GetOwningPawn()) : nullptr;
}

AFRWeaponBase* AFRHUD::GetActiveWeapon() const
{
	const AFRCharacter* Character = GetPlayerCharacter();
	return Character ? Character->GetActiveWeapon() : nullptr;
}

void AFRHUD::HandleHitRegistered(EFRHitZone Zone, float DistanceMetres, int32 Points)
{
	FFRHitMarker Marker;
	Marker.Zone = Zone;
	Marker.Points = Points;
	Marker.DistanceMetres = DistanceMetres;
	Marker.HorizontalOffset = FMath::FRandRange(-34.0f, 34.0f);

	HitMarkers.Add(Marker);

	CrosshairFlashTimer = 0.18f;
	CrosshairFlashColor = FRTypes::GetHitZoneColor(Zone);
}

void AFRHUD::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	EnsureGameStateBinding();

	CrosshairFlashTimer = FMath::Max(0.0f, CrosshairFlashTimer - DeltaSeconds);

	// Iterating backwards makes removal safe while the loop is running.
	for (int32 Index = HitMarkers.Num() - 1; Index >= 0; --Index)
	{
		HitMarkers[Index].Age += DeltaSeconds;

		if (HitMarkers[Index].Age >= HitMarkerLifetime)
		{
			HitMarkers.RemoveAt(Index);
		}
	}
}

void AFRHUD::DrawHUD()
{
	Super::DrawHUD();

	if (!Canvas)
	{
		return;
	}

	DrawScorePanel();
	DrawWeaponPanel();
	DrawCrosshair();
	DrawHitMarkers();
	DrawChallengeBanner();
	DrawControlHints();
}

// -- Crosshair ---------------------------------------------------------------

void AFRHUD::DrawCrosshair()
{
	const AFRWeaponBase* Weapon = GetActiveWeapon();
	const AFRCharacter* Character = GetPlayerCharacter();

	// A scoped rifle has its own reticle in the middle of the optic, and drawing
	// a second one over it would be a lie about where the bullet goes.
	if (const AFRSniperRifle* Rifle = Cast<AFRSniperRifle>(Weapon))
	{
		if (Rifle->IsScoped())
		{
			const float ScopeCentreX = Canvas->SizeX * 0.5f;
			const float ScopeCentreY = Canvas->SizeY * 0.5f;

			DrawLine(ScopeCentreX - 160.0f, ScopeCentreY, ScopeCentreX - 14.0f, ScopeCentreY, CrosshairColor, 1.0f);
			DrawLine(ScopeCentreX + 14.0f, ScopeCentreY, ScopeCentreX + 160.0f, ScopeCentreY, CrosshairColor, 1.0f);
			DrawLine(ScopeCentreX, ScopeCentreY - 160.0f, ScopeCentreX, ScopeCentreY - 14.0f, CrosshairColor, 1.0f);
			DrawLine(ScopeCentreX, ScopeCentreY + 14.0f, ScopeCentreX, ScopeCentreY + 160.0f, CrosshairColor, 1.0f);
			DrawRect(CrosshairColor, ScopeCentreX - 1.0f, ScopeCentreY - 1.0f, 2.0f, 2.0f);
			return;
		}
	}

	const float CentreX = Canvas->SizeX * 0.5f;
	const float CentreY = Canvas->SizeY * 0.5f;

	float Gap = CrosshairMinimumGap;

	if (Weapon && Character)
	{
		if (const UCameraComponent* Camera = Character->GetFirstPersonCamera())
		{
			// Project the spread cone onto the screen. Both the spread and the
			// field of view are half angles measured from the view direction, so
			// the ratio of their tangents is the ratio of their screen extents.
			const float SpreadDegrees = Weapon->GetCurrentSpreadDegrees();
			const float HalfFovDegrees = FMath::Max(Camera->FieldOfView * 0.5f, 1.0f);

			const float SpreadTangent = FMath::Tan(FMath::DegreesToRadians(SpreadDegrees));
			const float FovTangent = FMath::Tan(FMath::DegreesToRadians(HalfFovDegrees));

			const float SpreadPixels = (Canvas->SizeX * 0.5f) * SpreadTangent / FovTangent;
			Gap = FMath::Max(CrosshairMinimumGap, SpreadPixels);
		}
	}

	const float Arm = CrosshairArmLength;
	const FLinearColor Colour = (CrosshairFlashTimer > 0.0f) ? CrosshairFlashColor : CrosshairColor;

	DrawLine(CentreX - Gap - Arm, CentreY, CentreX - Gap, CentreY, Colour, CrosshairThickness);
	DrawLine(CentreX + Gap, CentreY, CentreX + Gap + Arm, CentreY, Colour, CrosshairThickness);
	DrawLine(CentreX, CentreY - Gap - Arm, CentreX, CentreY - Gap, Colour, CrosshairThickness);
	DrawLine(CentreX, CentreY + Gap, CentreX, CentreY + Gap + Arm, Colour, CrosshairThickness);

	// The centre dot marks the point of impact itself, which is what the brief
	// asks the crosshair to indicate.
	DrawRect(Colour, CentreX - 1.0f, CentreY - 1.0f, 2.0f, 2.0f);

	// Hit confirmation: four short diagonals, the shape every shooter reads as
	// "that one connected".
	if (CrosshairFlashTimer > 0.0f)
	{
		const float Inner = Gap + 3.0f;
		const float Outer = Gap + 11.0f;

		DrawLine(CentreX - Outer, CentreY - Outer, CentreX - Inner, CentreY - Inner, CrosshairFlashColor, 2.0f);
		DrawLine(CentreX + Inner, CentreY - Inner, CentreX + Outer, CentreY - Outer, CrosshairFlashColor, 2.0f);
		DrawLine(CentreX - Outer, CentreY + Outer, CentreX - Inner, CentreY + Inner, CrosshairFlashColor, 2.0f);
		DrawLine(CentreX + Inner, CentreY + Inner, CentreX + Outer, CentreY + Outer, CrosshairFlashColor, 2.0f);
	}
}

// -- Panels ------------------------------------------------------------------

void AFRHUD::DrawPanel(float X, float Y, float Width, float Height, const FLinearColor& Tint) const
{
	const_cast<AFRHUD*>(this)->DrawRect(Tint, X, Y, Width, Height);

	// A one pixel highlight along the top edge lifts the panel off the world
	// behind it without needing a texture.
	const_cast<AFRHUD*>(this)->DrawRect(FLinearColor(1.0f, 1.0f, 1.0f, 0.10f), X, Y, Width, 1.0f);
}

float AFRHUD::DrawLabel(const FString& Text, float X, float Y, const FLinearColor& Color, UFont* Font, float Scale)
{
	UFont* UsedFont = Font ? Font : GEngine->GetMediumFont();

	DrawText(Text, Color, X, Y, UsedFont, Scale, false);

	float Width = 0.0f;
	float Height = 0.0f;
	GetTextSize(Text, Width, Height, UsedFont, Scale);

	return Height;
}

void AFRHUD::DrawProgressBar(float X, float Y, float Width, float Height, float Progress, const FLinearColor& Color) const
{
	AFRHUD* Self = const_cast<AFRHUD*>(this);

	Self->DrawRect(FLinearColor(0.0f, 0.0f, 0.0f, 0.55f), X, Y, Width, Height);
	Self->DrawRect(Color, X, Y, Width * FMath::Clamp(Progress, 0.0f, 1.0f), Height);
}

void AFRHUD::DrawScorePanel()
{
	const AFRRangeGameState* RangeState = CachedGameState.Get();
	if (!RangeState)
	{
		return;
	}

	const FFRRangeStats& Stats = RangeState->GetStats();

	const float PanelX = 26.0f;
	const float PanelY = 26.0f;
	const float PanelWidth = 268.0f;
	const float PanelHeight = 132.0f;

	DrawPanel(PanelX, PanelY, PanelWidth, PanelHeight, PanelColor);

	float CursorY = PanelY + 10.0f;
	const float TextX = PanelX + 14.0f;

	CursorY += DrawLabel(FString::Printf(TEXT("SCORE  %d"), Stats.Score), TextX, CursorY, AccentColor, GEngine->GetLargeFont()) + 6.0f;

	// Accuracy is the headline figure of a firing range, so it gets the second
	// line and the exact wording the brief uses: hits divided by shots fired.
	CursorY += DrawLabel(FString::Printf(TEXT("ACCURACY  %s"), *Stats.GetAccuracyText()),
		TextX, CursorY, TextColor, GEngine->GetMediumFont()) + 4.0f;

	CursorY += DrawLabel(FString::Printf(TEXT("HITS  %d / %d"), Stats.ShotsHit, Stats.ShotsFired),
		TextX, CursorY, TextColor, GEngine->GetSmallFont()) + 3.0f;

	CursorY += DrawLabel(FString::Printf(TEXT("STREAK  %d    BEST  %d"), Stats.CurrentStreak, Stats.BestStreak),
		TextX, CursorY, TextColor, GEngine->GetSmallFont()) + 3.0f;

	DrawLabel(FString::Printf(TEXT("HEADSHOTS  %d    TARGETS  %d"), Stats.Headshots, Stats.TargetsDown),
		TextX, CursorY, TextColor, GEngine->GetSmallFont());
}

void AFRHUD::DrawWeaponPanel()
{
	const AFRWeaponBase* Weapon = GetActiveWeapon();
	const AFRCharacter* Character = GetPlayerCharacter();

	if (!Weapon || !Character)
	{
		return;
	}

	const float PanelWidth = 280.0f;
	const float PanelHeight = 106.0f;
	const float PanelX = Canvas->SizeX - PanelWidth - 26.0f;
	const float PanelY = Canvas->SizeY - PanelHeight - 26.0f;

	DrawPanel(PanelX, PanelY, PanelWidth, PanelHeight, PanelColor);

	const float TextX = PanelX + 14.0f;
	float CursorY = PanelY + 9.0f;

	CursorY += DrawLabel(Weapon->GetDisplayName(), TextX, CursorY, TextColor, GEngine->GetMediumFont()) + 4.0f;

	// An empty magazine turns the counter red. That single colour change is the
	// fastest way to tell the player why the trigger stopped responding.
	const int32 InMagazine = Weapon->GetAmmoInMagazine();
	const int32 Reserve = Character->GetReserveAmmo(Weapon->GetAmmoType());
	const FLinearColor AmmoColor = (InMagazine <= 0) ? WarningColor : AccentColor;

	CursorY += DrawLabel(FString::Printf(TEXT("%d / %d"), InMagazine, Reserve),
		TextX, CursorY, AmmoColor, GEngine->GetLargeFont()) + 6.0f;

	DrawLabel(FRTypes::GetAmmoTypeName(Weapon->GetAmmoType()), TextX, CursorY, TextColor, GEngine->GetSmallFont());

	// Reload state, exactly as the brief asks for: the fact that a reload is
	// happening and how far along it is.
	if (Weapon->IsReloading())
	{
		DrawLabel(TEXT("RELOADING"), TextX + 150.0f, CursorY - 2.0f, WarningColor, GEngine->GetSmallFont());
		DrawProgressBar(TextX, PanelY + PanelHeight - 12.0f, PanelWidth - 28.0f, 5.0f, Weapon->GetReloadProgress(), WarningColor);
	}
	else if (InMagazine <= 0 && Reserve <= 0)
	{
		DrawLabel(TEXT("NO AMMO"), TextX + 150.0f, CursorY - 2.0f, WarningColor, GEngine->GetSmallFont());
	}
}

void AFRHUD::DrawHitMarkers()
{
	const float CentreX = Canvas->SizeX * 0.5f;
	const float CentreY = Canvas->SizeY * 0.5f;

	for (const FFRHitMarker& Marker : HitMarkers)
	{
		const float Alpha = FMath::Clamp(Marker.Age / FMath::Max(HitMarkerLifetime, KINDA_SMALL_NUMBER), 0.0f, 1.0f);

		// Rise and fade. The label drifts up and out so a burst of hits reads as
		// several events instead of one flickering number.
		const float Rise = 46.0f + Alpha * 40.0f;
		FLinearColor Colour = FRTypes::GetHitZoneColor(Marker.Zone);
		Colour.A = 1.0f - Alpha;

		const FString Text = FString::Printf(TEXT("%s  +%d   %.0f m"),
			*FRTypes::GetHitZoneName(Marker.Zone), Marker.Points, Marker.DistanceMetres);

		float Width = 0.0f;
		float Height = 0.0f;
		GetTextSize(Text, Width, Height, GEngine->GetMediumFont(), 1.0f);

		DrawText(Text, Colour, CentreX - Width * 0.5f + Marker.HorizontalOffset, CentreY - Rise,
			GEngine->GetMediumFont(), 1.0f, false);
	}
}

void AFRHUD::DrawChallengeBanner()
{
	const AFRRangeGameState* RangeState = CachedGameState.Get();
	if (!RangeState)
	{
		return;
	}

	const EFRSessionState Session = RangeState->GetSessionState();
	if (Session == EFRSessionState::FreePractice)
	{
		return;
	}

	const float BannerWidth = 420.0f;
	const float BannerHeight = 64.0f;
	const float BannerX = (Canvas->SizeX - BannerWidth) * 0.5f;
	const float BannerY = 24.0f;

	DrawPanel(BannerX, BannerY, BannerWidth, BannerHeight, PanelColor);

	if (Session == EFRSessionState::Challenge)
	{
		const float Remaining = RangeState->GetChallengeTimeRemaining();
		const float Duration = FMath::Max(RangeState->GetChallengeDuration(), KINDA_SMALL_NUMBER);

		// The bar turns red in the last five seconds, which is the moment the
		// player stops aiming carefully and starts rushing.
		const FLinearColor BarColor = (Remaining <= 5.0f) ? WarningColor : AccentColor;

		const FString TimeText = FString::Printf(TEXT("CHALLENGE   %02d:%02d"),
			FMath::FloorToInt(Remaining / 60.0f), FMath::FloorToInt(FMath::Fmod(Remaining, 60.0f)));

		float Width = 0.0f;
		float Height = 0.0f;
		GetTextSize(TimeText, Width, Height, GEngine->GetLargeFont(), 1.0f);

		DrawText(TimeText, BarColor, BannerX + (BannerWidth - Width) * 0.5f, BannerY + 8.0f,
			GEngine->GetLargeFont(), 1.0f, false);

		DrawProgressBar(BannerX + 16.0f, BannerY + BannerHeight - 14.0f, BannerWidth - 32.0f, 6.0f,
			Remaining / Duration, BarColor);
	}
	else
	{
		const FFRRangeStats& Stats = RangeState->GetStats();

		const FString ResultText = FString::Printf(TEXT("TIME UP    %d POINTS    %s"),
			RangeState->GetLastChallengeScore(), *Stats.GetAccuracyText());

		float Width = 0.0f;
		float Height = 0.0f;
		GetTextSize(ResultText, Width, Height, GEngine->GetLargeFont(), 1.0f);

		DrawText(ResultText, AccentColor, BannerX + (BannerWidth - Width) * 0.5f, BannerY + 10.0f,
			GEngine->GetLargeFont(), 1.0f, false);

		const FString HintText = TEXT("Press T to run it again");
		GetTextSize(HintText, Width, Height, GEngine->GetSmallFont(), 1.0f);

		DrawText(HintText, TextColor, BannerX + (BannerWidth - Width) * 0.5f, BannerY + BannerHeight - 22.0f,
			GEngine->GetSmallFont(), 1.0f, false);
	}
}

void AFRHUD::DrawControlHints()
{
	const FString Hints = TEXT("LMB fire    RMB aim    R reload    1 2 3 weapons    T challenge    Esc pause");

	float Width = 0.0f;
	float Height = 0.0f;
	GetTextSize(Hints, Width, Height, GEngine->GetSmallFont(), 1.0f);

	DrawText(Hints, FLinearColor(1.0f, 1.0f, 1.0f, 0.35f),
		(Canvas->SizeX - Width) * 0.5f, Canvas->SizeY - Height - 12.0f,
		GEngine->GetSmallFont(), 1.0f, false);
}
