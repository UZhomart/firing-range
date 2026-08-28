// Copyright zutemiss & dshadykh. Educational project.

#pragma once

#include "CoreMinimal.h"
#include "Targets/FRTargetBase.h"

#include "FRStationaryTarget.generated.h"

/**
 * Fixed target for the static lanes.
 *
 * It adds no behaviour of its own: the value of the class is that the range can
 * ask for a stationary target by type, and that the static lane can be tuned
 * separately from the moving one. The only thing it changes is the look, so the
 * two kinds of target are told apart across the range at a glance.
 */
UCLASS()
class FIRINGRANGE_API AFRStationaryTarget : public AFRTargetBase
{
	GENERATED_BODY()

public:
	AFRStationaryTarget();
};
