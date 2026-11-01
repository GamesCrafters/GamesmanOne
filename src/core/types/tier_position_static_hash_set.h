/**
 * @file tier_position_static_hash_set.h
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Fixed-capacity linear probing TierPosition hash set with sentinel
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

#ifndef GAMESMANONE_CORE_TYPES_TIER_POSITION_STATIC_HASH_SET_H_
#define GAMESMANONE_CORE_TYPES_TIER_POSITION_STATIC_HASH_SET_H_

#include <assert.h>
#include <stdalign.h>
#include <stdbool.h>
#include <stdint.h>

#include "config.h"
#include "core/data_structures/hash.h"
#include "core/types/base.h"

/**
 * @brief Sentinel value used to represent an empty slot in the hash set.
 *
 * A `TierPosition` with tier equal to `INT64_MIN` should never be inserted.
 * Otherwise the behavior is undefined.
 */
#define TIER_POSITION_STATIC_HASH_SET_EMPTY_TIER INT64_MIN

/**
 * @brief Fixed-capacity linear probing hash set for storing `TierPosition`
 * keys on the stack.
 */
typedef struct {
    alignas(GM_CACHE_LINE_SIZE) TierPosition *keys; /**< Key array. */
    uint64_t capacity_mask; /**< Bitmask used for indexing (`capacity - 1`). */
    int size;               /**< Current number of elements in the set. */
} TierPositionStaticHashSet;

/**
 * @brief Macro helper to declare and initialize a stack-allocated
 * `TierPositionStaticHashSet` instance along with its underlying key array.
 *
 * @param[out] name Name of the `TierPositionStaticHashSet` variable to create.
 * @param[in] cap Capacity of the hash set; must be a compile-time constant
 * positive integral value and a power of 2.
 */
#define DECLARE_TIER_POSITION_STATIC_HASH_SET(name, cap)                       \
    static_assert((cap) > 0 && ((cap) & ((cap) - 1)) == 0,                     \
                  "Static hash set capacity (" #cap ") must be a power of 2"); \
    TierPosition name##_keys[cap];                                             \
    for (uint64_t i = 0; i < (uint64_t)(cap); ++i) {                           \
        name##_keys[i].tier = TIER_POSITION_STATIC_HASH_SET_EMPTY_TIER;        \
    }                                                                          \
    TierPositionStaticHashSet name = {name##_keys, (cap) - 1, 0}

/**
 * @brief Adds a `TierPosition` key to the static hash set.
 *
 * @param[in,out] set The `TierPositionStaticHashSet` to modify.
 * @param[in] key The `TierPosition` value to add.
 *
 * @retval true The `key` was successfully inserted into the set.
 * @retval false The `key` already exists in the set.
 */
static inline bool TierPositionStaticHashSetAdd(TierPositionStaticHashSet *set,
                                                TierPosition key) {
    const uint64_t capacity_mask = set->capacity_mask;
    uint64_t idx = Hash128to64(key.tier, key.position) & capacity_mask;

    const TierPosition *keys = set->keys;
    while (keys[idx].tier != TIER_POSITION_STATIC_HASH_SET_EMPTY_TIER) {
        if (keys[idx].tier == key.tier && keys[idx].position == key.position) {
            return false;
        }
        idx = (idx + 1ULL) & capacity_mask;
    }
    set->keys[idx] = key;
    ++set->size;

    return true;
}

/**
 * @brief Returns the number of elements in the static hash set.
 *
 * @param[in] set The `TierPositionStaticHashSet` to inspect.
 *
 * @return The number of elements in `set`.
 */
static inline int TierPositionStaticHashSetGetSize(
    const TierPositionStaticHashSet *set) {
    return set->size;
}

#endif  // GAMESMANONE_CORE_TYPES_TIER_POSITION_STATIC_HASH_SET_H_
