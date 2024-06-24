#include "core/types/tier_hash_map_ext.h"

#include <stdbool.h>  // bool
#include <stdint.h>   // int64_t

#include "core/data_structures/int64_hash_map_ext.h"
#include "core/types/base.h"

void TierHashMapExtInit(TierHashMapExt *map, double max_load_factor) {
    Int64HashMapExtInit(map, max_load_factor);
}

void TierHashMapExtDestroy(TierHashMapExt *map) {
    Int64HashMapExtDestroy(map);
}

bool TierHashMapExtContains(const TierHashMapExt *map, Tier tier) {
    return Int64HashMapExtContains(map, tier);
}

bool TierHashMapExtGet(const TierHashMapExt *map, Tier tier, int64_t *value) {
    return Int64HashMapExtGet(map, tier, value);
}

bool TierHashMapExtSet(TierHashMapExt *map, Tier tier, int64_t value) {
    return Int64HashMapExtSet(map, tier, value);
}

void TierHashMapExtRemove(TierHashMapExt *map, Tier tier) {
    Int64HashMapExtRemove(map, tier);
}

