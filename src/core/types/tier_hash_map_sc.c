#include "core/types/tier_hash_map_sc.h"

#include <stdbool.h>  // bool
#include <stdint.h>   // int64_t

#include "core/data_structures/int64_hash_map_sc.h"
#include "core/types/base.h"

void TierHashMapSCInit(TierHashMapSC *map, double max_load_factor) {
    Int64HashMapSCInit(map, max_load_factor);
}

void TierHashMapSCDestroy(TierHashMapSC *map) {
    Int64HashMapSCDestroy(map);
}

bool TierHashMapSCContains(const TierHashMapSC *map, Tier tier) {
    return Int64HashMapSCContains(map, tier);
}

bool TierHashMapSCGet(const TierHashMapSC *map, Tier tier, int64_t *value) {
    return Int64HashMapSCGet(map, tier, value);
}

bool TierHashMapSCSet(TierHashMapSC *map, Tier tier, int64_t value) {
    return Int64HashMapSCSet(map, tier, value);
}

void TierHashMapSCRemove(TierHashMapSC *map, Tier tier) {
    Int64HashMapSCRemove(map, tier);
}

