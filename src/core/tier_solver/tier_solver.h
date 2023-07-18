#ifndef GAMESMANEXPERIMENT_CORE_TIER_SOLVER_H_
#define GAMESMANEXPERIMENT_CORE_TIER_SOLVER_H_

#include <stdbool.h>  // bool
#include <stdint.h>   // int64_t

#include "core/gamesman_types.h"  // Position

int TierSolverSolve(Tier tier, bool force);

#endif  // GAMESMANEXPERIMENT_CORE_TIER_SOLVER_H_
