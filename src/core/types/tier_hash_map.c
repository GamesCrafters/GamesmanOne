#include "core/types/tier_hash_map.h"

#include <stdint.h>  // int64_t

#include "core/data_structures/int64_hash_map.h"
#include "core/types/base.h"

void TierHashMapInit(TierHashMap *map, double max_load_factor) {
    Int64HashMapInit(map, max_load_factor);
}

void TierHashMapDestroy(TierHashMap *map) { Int64HashMapDestroy(map); }

TierHashMapIterator TierHashMapGet(TierHashMap *map, Tier key) {
    return Int64HashMapGet(map, key);
}

bool TierHashMapSet(TierHashMap *map, Tier tier, int64_t value) {
    return Int64HashMapSet(map, tier, value);
}

bool TierHashMapContains(TierHashMap *map, Tier tier) {
    return Int64HashMapContains(map, tier);
}

TierHashMapIterator TierHashMapBegin(TierHashMap *map) {
    return Int64HashMapBegin(map);
}

Tier TierHashMapIteratorKey(const TierHashMapIterator *it) {
    return Int64HashMapIteratorKey(it);
}

int64_t TierHashMapIteratorValue(const TierHashMapIterator *it) {
    return Int64HashMapIteratorValue(it);
}

bool TierHashMapIteratorIsValid(const TierHashMapIterator *it) {
    return Int64HashMapIteratorIsValid(it);
}

bool TierHashMapIteratorNext(TierHashMapIterator *iterator, Tier *tier,
                             int64_t *value) {
    return Int64HashMapIteratorNext(iterator, tier, value);
}
