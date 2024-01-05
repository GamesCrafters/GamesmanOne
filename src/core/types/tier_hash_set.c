#include "core/types/tier_hash_set.h"

#include "core/data_structures/int64_hash_map.h"
#include "core/types/base.h"

void TierHashSetInit(TierHashSet *set, double max_load_factor) {
    Int64HashMapInit(set, max_load_factor);
}

void TierHashSetDestroy(TierHashSet *set) { Int64HashMapDestroy(set); }

bool TierHashSetContains(TierHashSet *set, Tier tier) {
    return Int64HashMapContains(set, tier);
}

bool TierHashSetAdd(TierHashSet *set, Tier tier) {
    return Int64HashMapSet(set, tier, 0);
}
