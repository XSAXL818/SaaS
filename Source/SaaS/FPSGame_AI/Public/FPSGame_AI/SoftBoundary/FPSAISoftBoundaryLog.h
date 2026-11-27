#pragma once

#include "CoreMinimal.h"
#include "Logging/LogMacros.h"

/**
 * Shared log category for the soft-boundary actor, component and subsystem.
 * Defined once in FPSAISoftBoundaryVolume.cpp; use Display verbosity for anything
 * that has to show up in the console / on screen during a PIE check.
 */
DECLARE_LOG_CATEGORY_EXTERN(LogFPSAISoftBoundary, Log, All);
