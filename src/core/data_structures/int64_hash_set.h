#ifndef GAMESMANONE_CORE_DATA_STRUCTURES_INT64_HASH_SET_H_
#define GAMESMANONE_CORE_DATA_STRUCTURES_INT64_HASH_SET_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "core/data_structures/hash.h"
#include "core/gamesman_memory.h"

#define INT64_HASH_SET_EMPTY_KEY INT64_MIN

typedef struct Int64HashSet {
    int64_t *keys;
    uint64_t mask;
    int64_t size;
    int64_t max_size;
    double max_load_factor;
} Int64HashSet;

static inline void Int64HashSetInit(Int64HashSet *set, double max_load_factor) {
    // Clamp max_load_factor to [0.25, 0.75]
    if (max_load_factor > 0.75) {
        max_load_factor = 0.75;
    } else if (max_load_factor < 0.25) {
        max_load_factor = 0.25;
    }

    set->keys = NULL;
    set->mask = 0x0ULL;
    set->size = 0L;
    set->max_size = 0L;
    set->max_load_factor = max_load_factor;
}

bool Int64HashSetInternalExpand(Int64HashSet *set);

bool Int64HashSetInternalExpandExplicit(Int64HashSet *set, uint64_t new_mask);

static inline bool Int64HashSetReserve(Int64HashSet *set, int64_t size) {
    if (size <= set->max_size) {
        return true;
    }

    uint64_t required_capacity =
        (uint64_t)((double)size / set->max_load_factor);
    uint64_t target_capacity = set->mask + 1;
    while (target_capacity <= required_capacity) {
        target_capacity <<= 1;
    }

    return Int64HashSetInternalExpandExplicit(set, target_capacity - 1);
}

static inline void Int64HashSetDestroy(Int64HashSet *set) {
    GamesmanFree(set->keys);  // NULL-safe
    set->keys = NULL;
    set->mask = 0x0ULL;
    set->size = 0L;
    set->max_size = 0L;
    set->max_load_factor = 0.0;
}

static inline bool Int64HashSetAdd(Int64HashSet *__restrict set, int64_t key) {
    if (set->size >= set->max_size) {
        if (!Int64HashSetInternalExpand(set)) {
            return false;
        }
    }

    // Hoist pointers and values to locals so the compiler
    // doesn't worry about memory aliasing during the loop.
    int64_t *__restrict keys = set->keys;
    uint64_t mask = set->mask;

    // Add key to the set
    uint64_t index = Splitmix64(key) & mask;
    while (keys[index] != INT64_HASH_SET_EMPTY_KEY) {
        if (keys[index] == key) {
            return false;
        }
        index = (index + 1) & mask;
    }
    keys[index] = key;
    ++set->size;

    return true;
}

#endif  // GAMESMANONE_CORE_DATA_STRUCTURES_INT64_HASH_SET_H_
