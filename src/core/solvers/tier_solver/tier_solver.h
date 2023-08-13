#ifndef GAMESMANEXPERIMENT_CORE_SOLVERS_TIER_SOLVER_TIER_SOLVER_H_
#define GAMESMANEXPERIMENT_CORE_SOLVERS_TIER_SOLVER_TIER_SOLVER_H_

#include <stdbool.h>
#include <stdint.h>

#include "core/gamesman_types.h"

extern Solver kTierSolver;

struct TierSolverApi {
    Position initial_position;
    Tier initial_tier;

    int64_t (*GetTierSize)(Tier tier);
    MoveArray (*GenerateMoves)(TierPosition tier_position);
    Value (*Primitive)(TierPosition tier_position);
    TierPosition (*DoMove)(TierPosition tier_position, Move move);
    bool (*IsLegalPosition)(TierPosition tier_position);
    Position (*GetCanonicalPosition)(TierPosition tier_position);
    int (*GetNumberOfCanonicalChildPositions)(TierPosition tier_position);
    TierPositionArray (*GetCanonicalChildPositions)(TierPosition tier_position);
    PositionArray (*GetCanonicalParentPositions)(TierPosition child,
                                                 Tier parent_tier);
    Position (*GetPositionInNonCanonicalTier)(TierPosition canonical,
                                              Tier noncanonical_tier);
    TierArray (*GetChildTiers)(Tier tier);
    TierArray (*GetParentTiers)(Tier tier);
    Tier (*GetCanonicalTier)(Tier tier);
} TierSolverApi;

#endif  // GAMESMANEXPERIMENT_CORE_SOLVERS_TIER_SOLVER_TIER_SOLVER_H_
