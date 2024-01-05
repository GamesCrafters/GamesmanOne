#ifndef GAMESMANONE_CORE_TYPES_TIER_ARRAY_H
#define GAMESMANONE_CORE_TYPES_TIER_ARRAY_H

#include "core/data_structures/int64_array.h"
#include "core/types/base.h"

/** @brief Dynamic Tier array. */
typedef Int64Array TierArray;

void TierArrayInit(TierArray *array);
void TierArrayDestroy(TierArray *array);
bool TierArrayAppend(TierArray *array, Tier tier);

#endif  // GAMESMANONE_CORE_TYPES_TIER_ARRAY_H
