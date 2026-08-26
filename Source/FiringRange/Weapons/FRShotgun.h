// Copyright zutemiss & dshadykh. Educational project.

#pragma once

#include "CoreMinimal.h"
#include "Weapons/FRWeaponBase.h"

#include "FRShotgun.generated.h"

/**
 * Pump action shotgun.
 *
 * Two things make it genuinely different from the pistol rather than a pistol
 * with other numbers. It releases a whole cone of pellets on one trigger pull,
 * so a single shot counts as several shots for the accuracy figure, and it tops
 * its tube up one shell at a time, which the player can interrupt by firing.
 */
UCLASS()
class FIRINGRANGE_API AFRShotgun : public AFRWeaponBase
{
	GENERATED_BODY()

public:
	AFRShotgun();

protected:
	virtual void BuildWeaponMesh() override;

	/** Firing during a shell by shell reload aborts it and shoots what is loaded. */
	virtual void HandleFireDenied() override;
};
