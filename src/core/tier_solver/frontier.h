#ifndef GAMESMANEXPERIMENT_CORE_FRONTIER_H_
#define GAMESMANEXPERIMENT_CORE_FRONTIER_H_
#include <stdbool.h>
#include <stdint.h>

#include "core/gamesman_types.h"  // PositionArray

#ifdef _OPENMP
#include <omp.h>
#endif

typedef struct {
    PositionArray *buckets;
    int64_t **dividers;
#ifdef _OPENMP
    omp_lock_t *locks;
#endif
    int size;
    int dividers_size;
} Frontier;

bool FrontierInit(Frontier *frontier, int frontier_size, int dividers_size);
void FrontierDestroy(Frontier *frontier);

bool FrontierAdd(Frontier *frontier, Position position, int remoteness,
                 int child_tier_index);
void FrontierAccumulateDividers(Frontier *frontier);
void FrontierFreeRemoteness(Frontier *frontier, int remoteness);

#endif  // GAMESMANEXPERIMENT_CORE_FRONTIER_H_
