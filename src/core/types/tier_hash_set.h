#ifndef GAMESMANONE_CORE_TYPES_TIER_HASH_SET_H
#define GAMESMANONE_CORE_TYPES_TIER_HASH_SET_H

#include "core/data_structures/int64_hash_map.h"
#include "core/types/base.h"

/** @brief Linear-probing Tier hash set using Int64HashMap. */
typedef Int64HashMap TierHashSet;

void TierHashSetInit(TierHashSet *set, double max_load_factor);
void TierHashSetDestroy(TierHashSet *set);
bool TierHashSetContains(TierHashSet *set, Tier tier);
bool TierHashSetAdd(TierHashSet *set, Tier tier);

#endif  // GAMESMANONE_CORE_TYPES_TIER_HASH_SET_H
