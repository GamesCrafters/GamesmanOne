#ifndef GAMESMANONE_CORE_TYPES_TIER_POSITION_HASH_SET_H
#define GAMESMANONE_CORE_TYPES_TIER_POSITION_HASH_SET_H

#include <stdbool.h>  // bool
#include <stdint.h>   // int64_t

#include "core/types/base.h"

/** @brief Entry in a TierPositionHashSet. */
typedef struct TierPositionHashSetEntry {
    TierPosition key;
    bool used;
} TierPositionHashSetEntry;

/** @brief Linear-probing TierPosition hash set. */
typedef struct TierPositionHashSet {
    TierPositionHashSetEntry *entries;
    int64_t capacity;
    int64_t size;
    double max_load_factor;
} TierPositionHashSet;

void TierPositionHashSetInit(TierPositionHashSet *set, double max_load_factor);
void TierPositionHashSetDestroy(TierPositionHashSet *set);
bool TierPositionHashSetContains(TierPositionHashSet *set, TierPosition key);
bool TierPositionHashSetAdd(TierPositionHashSet *set, TierPosition key);

#endif  // GAMESMANONE_CORE_TYPES_TIER_POSITION_HASH_SET_H
