#ifndef GAMESMANEXPERIMENT_CORE_SOLVERS_TIER_SOLVER_TIER_WORKER_H_
#define GAMESMANEXPERIMENT_CORE_SOLVERS_TIER_SOLVER_TIER_WORKER_H_

#include <stdbool.h>

#include "core/gamesman_types.h"
#include "core/solvers/tier_solver/tier_solver.h"

void TierWorkerInit(const TierSolverApi *api);
int TierWorkerSolve(Tier tier, bool force);

#endif  // GAMESMANEXPERIMENT_CORE_SOLVERS_TIER_SOLVER_TIER_WORKER_H_
