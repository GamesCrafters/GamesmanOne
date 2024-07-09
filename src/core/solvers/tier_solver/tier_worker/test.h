#ifndef GAMESMANONE_CORE_SOLVERS_TIER_SOLVER_TIER_WORKER_TEST_H_
#define GAMESMANONE_CORE_SOLVERS_TIER_SOLVER_TIER_WORKER_TEST_H_

#include "core/solvers/tier_solver/tier_solver.h"
#include "core/types/gamesman_types.h"

int TierWorkerTestInternal(const TierSolverApi *api, Tier tier,
                           const TierArray *parent_tiers, long seed);

#endif  // GAMESMANONE_CORE_SOLVERS_TIER_SOLVER_TIER_WORKER_TEST_H_
