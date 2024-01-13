#ifndef GAMESMANONE_CORE_SOLVERS_TIER_SOLVER_REVERSE_TIER_GRAPH_H_
#define GAMESMANONE_CORE_SOLVERS_TIER_SOLVER_REVERSE_TIER_GRAPH_H_

#include <stdint.h>  // int64_t

#include "core/types/gamesman_types.h"

typedef struct ReverseTierGraph {
    TierArray *parents_of;
    TierHashMap index_of;
    int64_t size;
    int64_t capacity;
} ReverseTierGraph;

void ReverseTierGraphInit(ReverseTierGraph *graph);

void ReverseTierGraphDestroy(ReverseTierGraph *graph);

int ReverseTierGraphAdd(ReverseTierGraph *graph, Tier child, Tier parent);

TierArray ReverseTierGraphPopParentsOf(ReverseTierGraph *graph, Tier child);

#endif  // GAMESMANONE_CORE_SOLVERS_TIER_SOLVER_REVERSE_TIER_GRAPH_H_
