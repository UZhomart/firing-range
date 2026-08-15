// Copyright zutemiss & dshadykh. Educational project.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"

#include "Core/FRTypes.h"

#include "FRSaveGame.generated.h"

/**
 * Persistent player preferences and records.
 *
 * The object is written through UGameplayStatics::SaveGameToSlot, which stores
 * the file under the project's `Saved/SaveGames` folder on every platform, so no
 * path handling code is needed to stay portable.
 */
UCLASS()
class FIRINGRANGE_API UFRSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	/** Slot name shared by every read and write of the settings. */
	static const FString SlotName;

	/** Local user index. Single player project, so it is always zero. */
	static const int32 UserIndex;

	/** Degrees of view rotation per unit of mouse movement. 1.0 is the engine default. */
	UPROPERTY()
	float MouseSensitivity = 1.0f;

	/** Extra multiplier applied while aiming down sights, where finer aim is expected. */
	UPROPERTY()
	float AimSensitivityScale = 0.55f;

	/** Inverts the vertical look axis. */
	UPROPERTY()
	bool bInvertLookY = false;

	/** Difficulty used by the target AI on the next range session. */
	UPROPERTY()
	EFRDifficulty Difficulty = EFRDifficulty::Normal;

	/** Best score achieved in the timed challenge, shown on the main menu. */
	UPROPERTY()
	int32 BestChallengeScore = 0;

	/** Best accuracy achieved in a finished timed challenge, stored as a 0..1 ratio. */
	UPROPERTY()
	float BestChallengeAccuracy = 0.0f;

	/** Clamps every field into a valid range after loading a file from disk. */
	void Sanitise();
};
