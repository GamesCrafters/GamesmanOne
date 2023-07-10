#ifndef GAMESMANEXPERIMENT_CORE_TIER_SOLVER_H_
#define GAMESMANEXPERIMENT_CORE_TIER_SOLVER_H_

#include <stdint.h>

#include "gamesman_types.h"

typedef struct TierSolverStatistics {
    int64_t num_legal_pos;
    int64_t num_win;
    int64_t num_lose;
    int64_t longest_num_steps_to_p1_win;
    Position longest_pos_to_p1_win;
    int64_t longest_num_steps_to_p2_win;
    Position longest_pos_to_p2_win;
    // TODO: add more fields if necessary.
} TierSolverStatistics;

TierSolverStatistics TierSolverSolve(Tier tier);

#endif  // GAMESMANEXPERIMENT_CORE_TIER_SOLVER_H_
