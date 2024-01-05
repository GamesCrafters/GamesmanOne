#ifndef GAMESMANONE_CORE_TYPES_TIER_POSITION_ARRAY_H
#define GAMESMANONE_CORE_TYPES_TIER_POSITION_ARRAY_H

#include <stdbool.h>  // bool
#include <stdint.h>   // int64_t

#include "core/types/base.h"

/** @brief Dynamic TierPositionArray. */
typedef struct TierPositionArray {
    TierPosition *array;
    int64_t size;
    int64_t capacity;
} TierPositionArray;

void TierPositionArrayInit(TierPositionArray *array);
void TierPositionArrayDestroy(TierPositionArray *array);
bool TierPositionArrayAppend(TierPositionArray *array,
                             TierPosition tier_position);
TierPosition TierPositionArrayBack(const TierPositionArray *array);
bool TierPositionArrayResize(TierPositionArray *array, int64_t size);

#endif  // GAMESMANONE_CORE_TYPES_TIER_POSITION_ARRAY_H
