// Copyright zutemiss & dshadykh. Educational project.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"

#include "Core/FRTypes.h"

#include "FRGameInstance.generated.h"

class UFRSaveGame;

/** Broadcast whenever a setting changes so listeners can apply it immediately. */
DECLARE_MULTICAST_DELEGATE(FFROnSettingsChanged);

/**
 * Owner of everything that must survive a level transition.
 *
 * The game instance is created once when the game starts and destroyed when it
 * closes. Unlike the game mode or the player controller it is not rebuilt when
 * the player travels from the menu map to the range map, which makes it the
 * correct home for user settings and for cross level records.
 */
UCLASS()
class FIRINGRANGE_API UFRGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	virtual void Init() override;
	virtual void Shutdown() override;

	/** Convenience accessor. Returns nullptr outside of a world, never asserts. */
	static UFRGameInstance* Get(const UObject* WorldContextObject);

	/** Fired after any setter below wrote a new value. */
	FFROnSettingsChanged OnSettingsChanged;

	// -- Look settings --------------------------------------------------------

	float GetMouseSensitivity() const;
	void SetMouseSensitivity(float NewSensitivity);

	float GetAimSensitivityScale() const;
	void SetAimSensitivityScale(float NewScale);

	bool GetInvertLookY() const;
	void SetInvertLookY(bool bNewInvert);

	// -- Session settings -----------------------------------------------------

	EFRDifficulty GetDifficulty() const;
	void SetDifficulty(EFRDifficulty NewDifficulty);

	// -- Records --------------------------------------------------------------

	int32 GetBestChallengeScore() const;
	float GetBestChallengeAccuracy() const;

	/** Stores a finished challenge run if it beats the previous record. */
	void SubmitChallengeResult(int32 Score, float Accuracy);

	// -- Persistence ----------------------------------------------------------

	/** Writes the settings object to its save slot. Safe to call often. */
	void SaveSettings();

	/** Restores defaults and immediately persists them. Used by the settings menu. */
	void ResetSettingsToDefaults();

private:
	/** Reads the save slot, or creates a fresh settings object when nothing is stored yet. */
	void LoadSettings();

	/** Live settings object. Always valid after Init. */
	UPROPERTY()
	TObjectPtr<UFRSaveGame> Settings = nullptr;

	/** Set while a batch of setters runs, to avoid writing the file several times. */
	bool bSaveRequested = false;
};
