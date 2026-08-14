// Copyright zutemiss & dshadykh. Educational project.

#pragma once

#include "CoreMinimal.h"

/**
 * Shared log category for the whole project.
 *
 * Every subsystem logs through this category, which makes it possible to filter
 * the output log with a single command: `Log LogFiringRange Verbose`.
 */
DECLARE_LOG_CATEGORY_EXTERN(LogFiringRange, Log, All);
