/**
 * @file int64_hash_set.h
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Linear-probing int64_t hash set.
 * @version 2.0.0
 * @date 2025-05-11
 *
 * @copyright This file is part of GAMESMAN, The Finite, Two-person
 * Perfect-Information Game Generator released under the GPL:
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef GAMESMANONE_CORE_DATA_STRUCTURES_INT64_HASH_SET_H_
#define GAMESMANONE_CORE_DATA_STRUCTURES_INT64_HASH_SET_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "core/data_structures/hash.h"
#include "core/gamesman_memory.h"

/**
 * @brief Entry object of an \c Int64HashSet. This struct is not meant to be
 * used by the user of this library. Always use accessor and mutator functions
 * instead.
 */
typedef struct Int64HashSetEntry {
    int64_t key; /**< Key to the entry. */
    bool used;   /**< True iff this bucket contains an actual record. */
} Int64HashSetEntry;

/**
 * @brief Linear-probing int64_t hash set.
 */
typedef struct Int64HashSet {
    Int64HashSetEntry *entries; /**< Dynamic array of buckets. */
    int64_t
        capacity_mask; /**< Number of buckets - 1, for fast bucket indexing. */
    int64_t size;      /**< Number of entries in the hash set. */
    double max_load_factor; /**< Hash set will automatically expand if
                            (double)size/capacity is greater than this value. */
} Int64HashSet;

/**
 * @brief Initializes the given \p set to an empty set with maximum load
 * factor \p max_load_factor.
 *
 * @param set Set to initialize.
 * @param max_load_factor Set maximum load factor of \p set to this value. The
 * hash set will automatically expand its capacity if (double)size/capacity is
 * greater than \p max_load_factor. A small value trades memory for speed
 * whereas a large value trades speed for memory. This value is restricted to be
 * in the range [0.25, 0.75] to provide optimal performance. The actual max load
 * factor is capped at 0.25 and 0.75 respectively if the user passes a value
 * that is smaller than 0.25 or greater than 0.75.
 */
static inline void Int64HashSetInit(Int64HashSet *set, double max_load_factor) {
    set->entries = NULL;
    set->capacity_mask = -1;
    set->size = 0;
    if (max_load_factor > 0.75) max_load_factor = 0.75;
    if (max_load_factor < 0.25) max_load_factor = 0.25;
    set->max_load_factor = max_load_factor;
}

static inline int64_t Hash(int64_t key, int64_t capacity_mask) {
    return (int64_t)Splitmix64((uint64_t)key) & capacity_mask;
}

static inline int64_t NextIndex(int64_t index, int64_t capacity_mask) {
    return (index + 1) & capacity_mask;
}

static inline bool Expand(Int64HashSet *set, int64_t new_mask) {
    Int64HashSetEntry *new_entries = (Int64HashSetEntry *)GamesmanCallocWhole(
        new_mask + 1, sizeof(Int64HashSetEntry));
    if (new_entries == NULL) return false;

    for (int64_t i = 0; i <= set->capacity_mask; ++i) {
        if (set->entries[i].used) {
            int64_t new_index = Hash(set->entries[i].key, new_mask);
            while (new_entries[new_index].used) {
                new_index = NextIndex(new_index, new_mask);
            }
            new_entries[new_index] = set->entries[i];
        }
    }
    GamesmanFree(set->entries);
    set->entries = new_entries;
    set->capacity_mask = new_mask;

    return true;
}

static inline int64_t MinCapacityMask(int64_t capacity) {
    if (capacity <= 0) return -1;

    capacity--;
    capacity |= capacity >> 1;
    capacity |= capacity >> 2;
    capacity |= capacity >> 4;
    capacity |= capacity >> 8;
    capacity |= capacity >> 16;
    capacity |= capacity >> 32;

    return capacity;
}

/**
 * @brief Attempts to reserve space for \p size elements in \p set. If \c true
 * is returned, the target hash set \p set is guaranteed to have space for at
 * least \p size elements before it expands internally. If \c false is returned,
 * the hash set remains unchanged.
 *
 * @param set Target hash set.
 * @param size Number of elements to reserve space for.
 * @return \c true on success,
 * @return \c false otherwise.
 */
static inline bool Int64HashSetReserve(Int64HashSet *set, int64_t size) {
    int64_t target_capacity_mask =
        MinCapacityMask((int64_t)((double)size / set->max_load_factor));
    if (target_capacity_mask <= set->capacity_mask) return true;

    return Expand(set, target_capacity_mask);
}

/** @brief Deallocates the given \p set. */
static inline void Int64HashSetDestroy(Int64HashSet *set) {
    GamesmanFree(set->entries);
    set->entries = NULL;
    set->capacity_mask = -1;
    set->size = 0;
    set->max_load_factor = 0.0;
}

/**
 * @brief Adds \p key to \p set or does nothing if \p set already contains
 * \p key.
 *
 * @param set Set to add \p key to.
 * @param key Key to add to \p set.
 * @return \c true if \p key was added to \p set as a new key, or
 * @return \c false if \p set already contains \p key or an error occurred.
 */
static inline bool Int64HashSetAdd(Int64HashSet *set, int64_t key) {
    // Check if resizing is needed.
    if (set->capacity_mask < 0) {
        if (!Expand(set, 1)) return false;
    } else if ((double)(set->size + 1) >
               (double)(set->capacity_mask + 1) * set->max_load_factor) {
        int64_t new_capacity_mask = (set->capacity_mask << 1) | 1;
        if (!Expand(set, new_capacity_mask)) return false;
    }

    // Set value at key.
    int64_t index = Hash(key, set->capacity_mask);
    while (set->entries[index].used) {
        if (set->entries[index].key == key) return false;
        index = NextIndex(index, set->capacity_mask);
    }
    set->entries[index].key = key;
    set->entries[index].used = true;
    ++set->size;

    return true;
}

/**
 * @brief Tests if \p key is in \p set.
 *
 * @param set Set from which the given \p key is looked up.
 * @param key Key to look for.
 * @return true if \p set contains \p key, or
 * @return false otherwise.
 */
static inline bool Int64HashSetContains(const Int64HashSet *set, int64_t key) {
    // Edge case: return false if set is empty.
    if (set->capacity_mask < 0) return false;

    int64_t index = Hash(key, set->capacity_mask);
    while (set->entries[index].used) {
        if (set->entries[index].key == key) return true;
        index = NextIndex(index, set->capacity_mask);
    }

    return false;
}

#endif  // GAMESMANONE_CORE_DATA_STRUCTURES_INT64_HASH_SET_H_
