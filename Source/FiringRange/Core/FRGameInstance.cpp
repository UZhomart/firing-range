// Copyright zutemiss & dshadykh. Educational project.

#include "Core/FRGameInstance.h"

#include "Kismet/GameplayStatics.h"

#include "Core/FRSaveGame.h"
#include "FiringRange.h"

void UFRGameInstance::Init()
{
	Super::Init();

	LoadSettings();

	UE_LOG(LogFiringRange, Log, TEXT("Game instance initialised. Mouse sensitivity %.2f, difficulty %d."),
		GetMouseSensitivity(), static_cast<int32>(GetDifficulty()));
}

void UFRGameInstance::Shutdown()
{
	// Last chance to persist anything that was changed but not written yet.
	if (bSaveRequested)
	{
		SaveSettings();
	}

	Super::Shutdown();
}

UFRGameInstance* UFRGameInstance::Get(const UObject* WorldContextObject)
{
	if (!WorldContextObject)
	{
		return nullptr;
	}

	const UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
	return World ? Cast<UFRGameInstance>(World->GetGameInstance()) : nullptr;
}

void UFRGameInstance::LoadSettings()
{
	if (UGameplayStatics::DoesSaveGameExist(UFRSaveGame::SlotName, UFRSaveGame::UserIndex))
	{
		Settings = Cast<UFRSaveGame>(UGameplayStatics::LoadGameFromSlot(UFRSaveGame::SlotName, UFRSaveGame::UserIndex));
	}

	// Either nothing was stored yet, or the stored file could not be read back
	// because the class changed. Both cases are handled by starting fresh.
	if (!Settings)
	{
		Settings = Cast<UFRSaveGame>(UGameplayStatics::CreateSaveGameObject(UFRSaveGame::StaticClass()));
		UE_LOG(LogFiringRange, Log, TEXT("No settings file found, created defaults."));
	}

	if (Settings)
	{
		Settings->Sanitise();
	}
}

void UFRGameInstance::SaveSettings()
{
	if (!Settings)
	{
		return;
	}

	Settings->Sanitise();
	UGameplayStatics::SaveGameToSlot(Settings, UFRSaveGame::SlotName, UFRSaveGame::UserIndex);
	bSaveRequested = false;
}

void UFRGameInstance::ResetSettingsToDefaults()
{
	Settings = Cast<UFRSaveGame>(UGameplayStatics::CreateSaveGameObject(UFRSaveGame::StaticClass()));
	SaveSettings();
	OnSettingsChanged.Broadcast();
}

float UFRGameInstance::GetMouseSensitivity() const
{
	return Settings ? Settings->MouseSensitivity : 1.0f;
}

void UFRGameInstance::SetMouseSensitivity(float NewSensitivity)
{
	if (!Settings)
	{
		return;
	}

	const float Clamped = FMath::Clamp(NewSensitivity, 0.10f, 4.0f);
	if (FMath::IsNearlyEqual(Clamped, Settings->MouseSensitivity))
	{
		return;
	}

	Settings->MouseSensitivity = Clamped;
	bSaveRequested = true;
	OnSettingsChanged.Broadcast();
}

float UFRGameInstance::GetAimSensitivityScale() const
{
	return Settings ? Settings->AimSensitivityScale : 0.55f;
}

void UFRGameInstance::SetAimSensitivityScale(float NewScale)
{
	if (!Settings)
	{
		return;
	}

	const float Clamped = FMath::Clamp(NewScale, 0.10f, 1.0f);
	if (FMath::IsNearlyEqual(Clamped, Settings->AimSensitivityScale))
	{
		return;
	}

	Settings->AimSensitivityScale = Clamped;
	bSaveRequested = true;
	OnSettingsChanged.Broadcast();
}

bool UFRGameInstance::GetInvertLookY() const
{
	return Settings ? Settings->bInvertLookY : false;
}

void UFRGameInstance::SetInvertLookY(bool bNewInvert)
{
	if (!Settings || Settings->bInvertLookY == bNewInvert)
	{
		return;
	}

	Settings->bInvertLookY = bNewInvert;
	bSaveRequested = true;
	OnSettingsChanged.Broadcast();
}

EFRDifficulty UFRGameInstance::GetDifficulty() const
{
	return Settings ? Settings->Difficulty : EFRDifficulty::Normal;
}

void UFRGameInstance::SetDifficulty(EFRDifficulty NewDifficulty)
{
	if (!Settings || Settings->Difficulty == NewDifficulty)
	{
		return;
	}

	Settings->Difficulty = NewDifficulty;
	bSaveRequested = true;
	OnSettingsChanged.Broadcast();
}

int32 UFRGameInstance::GetBestChallengeScore() const
{
	return Settings ? Settings->BestChallengeScore : 0;
}

float UFRGameInstance::GetBestChallengeAccuracy() const
{
	return Settings ? Settings->BestChallengeAccuracy : 0.0f;
}

void UFRGameInstance::SubmitChallengeResult(int32 Score, float Accuracy)
{
	if (!Settings || Score <= Settings->BestChallengeScore)
	{
		return;
	}

	Settings->BestChallengeScore = Score;
	Settings->BestChallengeAccuracy = Accuracy;
	SaveSettings();

	UE_LOG(LogFiringRange, Log, TEXT("New challenge record: %d points at %.1f %% accuracy."), Score, Accuracy * 100.0f);
}
