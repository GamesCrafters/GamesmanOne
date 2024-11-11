#ifndef GAMESMANONE_CORE_TYPES_TIER_HASH_MAP_SC_H_
#define GAMESMANONE_CORE_TYPES_TIER_HASH_MAP_SC_H_

#include <stdbool.h>  // bool
#include <stdint.h>   // int64_t

#include "core/data_structures/int64_hash_map_sc.h"
#include "core/types/base.h"

typedef Int64HashMapSC TierHashMapSC;

void TierHashMapSCInit(TierHashMapSC *map, double max_load_factor);
void TierHashMapSCDestroy(TierHashMapSC *map);

bool TierHashMapSCContains(const TierHashMapSC *map, Tier tier);
bool TierHashMapSCGet(const TierHashMapSC *map, Tier tier, int64_t *value);
bool TierHashMapSCSet(TierHashMapSC *map, Tier tier, int64_t value);
void TierHashMapSCRemove(TierHashMapSC *map, Tier tier);


#endif  // GAMESMANONE_CORE_TYPES_TIER_HASH_MAP_H_
