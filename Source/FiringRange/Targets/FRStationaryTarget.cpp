// Copyright zutemiss & dshadykh. Educational project.

#include "Targets/FRStationaryTarget.h"

AFRStationaryTarget::AFRStationaryTarget()
{
	// A paper target on a fixed stand: pale board, red rings, waist high mount.
	BoardColor = FLinearColor(0.88f, 0.87f, 0.83f);
	RingColor = FLinearColor(0.78f, 0.11f, 0.09f);
	HeadColor = FLinearColor(0.82f, 0.81f, 0.78f);

	BoardRadius = 34.0f;
	BullseyeRadius = 9.0f;
	InnerRingRadius = 20.0f;
	HeadRadius = 13.0f;
	PostHeight = 92.0f;

	// Static targets come back quickly, because their whole job is to let the
	// player keep a rhythm while working on accuracy.
	RespawnDelay = 2.2f;
	TipDuration = 0.20f;
}
