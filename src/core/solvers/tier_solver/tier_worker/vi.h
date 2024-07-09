#ifndef GAMESMANONE_CORE_SOLVERS_TIER_SOLVER_TIER_WORKER_VI_H_
#define GAMESMANONE_CORE_SOLVERS_TIER_SOLVER_TIER_WORKER_VI_H_

#include <stdbool.h>

#include "core/solvers/tier_solver/tier_solver.h"
#include "core/types/gamesman_types.h"

int TierWorkerSolveVIInternal(const TierSolverApi *api, int64_t db_chunk_size,
                              Tier tier, bool force, bool compare,
                              bool *solved);

#endif  // GAMESMANONE_CORE_SOLVERS_TIER_SOLVER_TIER_WORKER_VI_H_
