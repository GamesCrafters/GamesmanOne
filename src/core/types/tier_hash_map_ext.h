#ifndef GAMESMANONE_CORE_TYPES_TIER_HASH_MAP_EXT_H
#define GAMESMANONE_CORE_TYPES_TIER_HASH_MAP_EXT_H

#include <stdbool.h>  // bool
#include <stdint.h>   // int64_t

#include "core/data_structures/int64_hash_map_ext.h"
#include "core/types/base.h"

typedef Int64HashMapExt TierHashMapExt;

void TierHashMapExtInit(TierHashMapExt *map, double max_load_factor);
void TierHashMapExtDestroy(TierHashMapExt *map);

bool TierHashMapExtContains(const TierHashMapExt *map, Tier tier);
bool TierHashMapExtGet(const TierHashMapExt *map, Tier tier, int64_t *value);
bool TierHashMapExtSet(TierHashMapExt *map, Tier tier, int64_t value);
void TierHashMapExtRemove(TierHashMapExt *map, Tier tier);


#endif  // GAMESMANONE_CORE_TYPES_TIER_HASH_MAP_H
