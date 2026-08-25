// Copyright zutemiss & dshadykh. Educational project.

#pragma once

#include "CoreMinimal.h"
#include "Weapons/FRWeaponBase.h"

#include "FRPistol.generated.h"

/**
 * Standard sidearm of the range and the weapon the player starts with.
 *
 * Semi automatic, quick magazine change, mild recoil. It is the reference the
 * other two weapons are tuned against.
 */
UCLASS()
class FIRINGRANGE_API AFRPistol : public AFRWeaponBase
{
	GENERATED_BODY()

public:
	AFRPistol();

protected:
	virtual void BuildWeaponMesh() override;
};
