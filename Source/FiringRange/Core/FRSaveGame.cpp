// Copyright zutemiss & dshadykh. Educational project.

#include "Core/FRSaveGame.h"

const FString UFRSaveGame::SlotName = TEXT("FiringRangeSettings");
const int32 UFRSaveGame::UserIndex = 0;

void UFRSaveGame::Sanitise()
{
	// A save file can come from an older build, or from a user who edited it by
	// hand. Clamping here means the rest of the code may trust these values.
	MouseSensitivity = FMath::Clamp(MouseSensitivity, 0.10f, 4.0f);
	AimSensitivityScale = FMath::Clamp(AimSensitivityScale, 0.10f, 1.0f);
	BestChallengeScore = FMath::Max(0, BestChallengeScore);
	BestChallengeAccuracy = FMath::Clamp(BestChallengeAccuracy, 0.0f, 1.0f);
}
