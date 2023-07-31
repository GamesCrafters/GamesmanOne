#ifndef GAMESMANEXPERIMENT_CORE_TIER_SOLVER_REVERSE_GRAPH_H_
#define GAMESMANEXPERIMENT_CORE_TIER_SOLVER_REVERSE_GRAPH_H_

#include <stdbool.h>
#include <stdint.h>

#include "core/gamesman_types.h"

#ifdef _OPENMP
#include <omp.h>
#endif

typedef struct ReverseGraph {
    PositionArray *parents_of;
#ifdef _OPENMP
    omp_lock_t *locks;
#endif
    int64_t size;
    TierHashMap offset_map;
} ReverseGraph;

bool ReverseGraphInit(ReverseGraph *graph, const TierArray *child_tiers,
                      Tier this_tier);
void ReverseGraphDestroy(ReverseGraph *graph);
int64_t ReverseGraphGetIndex(ReverseGraph *graph, TierPosition tier_position);
bool ReverseGraphAdd(ReverseGraph *graph, TierPosition child, Position parent);

#endif  // GAMESMANEXPERIMENT_CORE_TIER_SOLVER_REVERSE_GRAPH_H_
