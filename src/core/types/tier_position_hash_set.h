/**
 * @file tier_position_hash_set.h
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Dynamically-sized linear probing TierPosition hash set with sentinel
 * value optimization.
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

#ifndef GAMESMANONE_CORE_TYPES_TIER_POSITION_HASH_SET_H_
#define GAMESMANONE_CORE_TYPES_TIER_POSITION_HASH_SET_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "core/data_structures/hash.h"
#include "core/gamesman_memory.h"
#include "core/types/base.h"

/**
 * @brief Sentinel values used to represent an empty slot in the hash set.
 *
 * The library is aggressively optimized and assumes that a TierPosition
 * with tier equal to `INT64_MIN` is never inserted as a key.
 */
#define TIER_POSITION_HASH_SET_EMPTY_TIER INT64_MIN

/**
 * @brief Highly optimized linear probing hash set for storing TierPosition
 * keys.
 */
typedef struct TierPositionHashSet {
    TierPosition *keys; /**< Array of keys in the hash set. */
    uint64_t mask;      /**< Bitmask used for indexing into the keys. */
    int64_t size;       /**< Current number of elements in the set. */
    int64_t max_size;   /**< Maximum elements before expansion is needed. */
    double inv_max_load_factor; /**< Equals to `1.0 / max_load_factor`. */
} TierPositionHashSet;

/**
 * @brief Initializes TierPosition hash set SET to an empty set with maximum
 * load factor MAX_LOAD_FACTOR.
 *
 * @param[out] set The `TierPositionHashSet` to initialize.
 * @param[in] max_load_factor The maximum load factor, clamped to [0.5, 0.8].
 */
static inline void TierPositionHashSetInit(TierPositionHashSet *set,
                                           double max_load_factor) {
    // Clamp max_load_factor to [0.5, 0.8]
    if (max_load_factor < 0.5) {
        max_load_factor = 0.5;
    } else if (max_load_factor > 0.8) {
        max_load_factor = 0.8;
    }

    set->keys = NULL;
    set->mask = 0x0ULL;
    set->size = 0L;
    set->max_size = 0L;
    set->inv_max_load_factor = 1.0 / max_load_factor;
}

/**
 * @brief [INTERNAL] Expands the hash set to a specific capacity derived from a
 * new mask.
 *
 * @warning Internal function exposed for optimization purposes.
 */
bool TierPositionHashSetInternalExpandExplicit(TierPositionHashSet *set,
                                               uint64_t new_mask);

/**
 * @brief Reserves space in the hash set for at least the specified size.
 *
 * @param[in,out] set The `TierPositionHashSet` to modify.
 * @param[in] size The minimum number of elements to reserve space for.
 *
 * @retval true The capacity is sufficient or was successfully expanded.
 * @retval false Memory allocation failed during expansion.
 */
static inline bool TierPositionHashSetReserve(TierPositionHashSet *set,
                                              int64_t size) {
    if (size <= set->max_size) {
        return true;
    }

    uint64_t required_capacity =
        (uint64_t)((double)size * set->inv_max_load_factor);

    // Prevents UB (shifting left by 64). No need to check if required_capacity
    // == 0 here because size is strictly positive and inv_max_load_factor > 1.
    if (required_capacity >= (1ULL << 63)) {
        return false;
    }

    // Calculate the next power of 2 strictly greater than required_capacity
    // Subtracting from 64 gives the position of the highest set bit + 1.
    uint64_t needed_capacity = 1ULL
                               << (64 - __builtin_clzll(required_capacity));

    return TierPositionHashSetInternalExpandExplicit(set, needed_capacity - 1);
}

/**
 * @brief Frees the memory associated with the hash set and resets its state.
 *
 * @param[in,out] set The `TierPositionHashSet` to destroy.
 */
static inline void TierPositionHashSetDestroy(TierPositionHashSet *set) {
    GamesmanFree(set->keys);  // NULL-safe
    set->keys = NULL;
    set->mask = 0x0ULL;
    set->size = 0L;
    set->max_size = 0L;
    set->inv_max_load_factor = 0.0;
}

/**
 * @brief Returns true if the TierPosition hash set SET contains KEY, or
 * false otherwise.
 *
 * @param[in] set The `TierPositionHashSet` to search.
 * @param[in] key The `TierPosition` key to look up.
 *
 * @retval true The `key` is present in the set.
 * @retval false The `key` is not in the set, or the set is empty.
 */
static inline bool TierPositionHashSetContains(const TierPositionHashSet *set,
                                               TierPosition key) {
    const TierPosition *__restrict keys = set->keys;

    // Return false if set has not been lazily initialized
    if (!keys) {
        return false;
    }

    const uint64_t mask = set->mask;

    // Look for key in the set
    uint64_t index = Hash128to64(key.tier, key.position) & mask;
    while (keys[index].tier != TIER_POSITION_HASH_SET_EMPTY_TIER) {
        if (keys[index].tier == key.tier &&
            keys[index].position == key.position) {
            return true;
        }
        index = (index + 1) & mask;
        // We don't need to worry about infinite loop here because
        // max_load_factor is strictly less than 0.8.
    }

    return false;
}

/**
 * @brief [INTERNAL] Expands the internal capacity of the hash set.
 *
 * @warning Internal function exposed for optimization purposes.
 */
bool TierPositionHashSetInternalExpand(TierPositionHashSet *set);

/**
 * @brief Adds a TierPosition key to the hash set.
 *
 * @param[in,out] set The `TierPositionHashSet` to add the key to.
 * @param[in] key The `TierPosition` value to add. The tier must not be equal to
 * `TIER_POSITION_HASH_SET_EMPTY_TIER` (`INT64_MIN`).
 *
 * @retval true The `key` was successfully added.
 * @retval false The `key` already exists, or memory allocation failed.
 */
static inline bool TierPositionHashSetAdd(TierPositionHashSet *set,
                                          TierPosition key) {
    if (set->size >= set->max_size) {
        if (!TierPositionHashSetInternalExpand(set)) {
            return false;
        }
    }

    // Hoist pointers and values to locals so the compiler
    // doesn't worry about memory aliasing during the loop.
    TierPosition *__restrict keys = set->keys;
    uint64_t mask = set->mask;

    // Add key to the set
    uint64_t index = Hash128to64(key.tier, key.position) & mask;
    while (keys[index].tier != TIER_POSITION_HASH_SET_EMPTY_TIER) {
        if (keys[index].tier == key.tier &&
            keys[index].position == key.position) {
            return false;
        }
        index = (index + 1) & mask;
    }
    keys[index] = key;
    ++set->size;

    return true;
}

#endif  // GAMESMANONE_CORE_TYPES_TIER_POSITION_HASH_SET_H_
