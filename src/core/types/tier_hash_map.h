#ifndef GAMESMANONE_CORE_TYPES_TIER_HASH_MAP_H
#define GAMESMANONE_CORE_TYPES_TIER_HASH_MAP_H

#include <stdint.h>  // int64_t

#include "core/data_structures/int64_hash_map.h"
#include "core/types/base.h"

/** @brief Linear-probing Tier to int64_t hash map using Int64HashMap. */
typedef Int64HashMap TierHashMap;

/** @brief Iterator for TierHashMap. */
typedef Int64HashMapIterator TierHashMapIterator;

void TierHashMapInit(TierHashMap *map, double max_load_factor);
void TierHashMapDestroy(TierHashMap *map);
TierHashMapIterator TierHashMapGet(TierHashMap *map, Tier key);
bool TierHashMapSet(TierHashMap *map, Tier tier, int64_t value);
bool TierHashMapContains(TierHashMap *map, Tier tier);

TierHashMapIterator TierHashMapBegin(TierHashMap *map);
Tier TierHashMapIteratorKey(const TierHashMapIterator *it);
int64_t TierHashMapIteratorValue(const TierHashMapIterator *it);
bool TierHashMapIteratorIsValid(const TierHashMapIterator *it);
bool TierHashMapIteratorNext(TierHashMapIterator *iterator, Tier *tier,
                             int64_t *value);

#endif  // GAMESMANONE_CORE_TYPES_TIER_HASH_MAP_H
