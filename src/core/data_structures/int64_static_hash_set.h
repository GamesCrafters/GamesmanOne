/**
 * @file int64_static_hash_set.h
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Fixed-capacity linear probing 64-bit integer hash set with sentinel
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

#ifndef GAMESMANONE_CORE_DATA_STRUCTURES_INT64_STATIC_HASH_SET_H_
#define GAMESMANONE_CORE_DATA_STRUCTURES_INT64_STATIC_HASH_SET_H_

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
 * A key with value equal to `INT64_MIN` should never be inserted. Otherwise the
 * behavior is undefined.
 */
#define INT64_STATIC_HASH_SET_EMPTY_KEY INT64_MIN

/**
 * @brief Fixed-capacity linear probing hash set for storing `int64_t`
 * keys on the stack.
 */
typedef struct {
    alignas(GM_CACHE_LINE_SIZE) int64_t *keys; /**< Key array. */
    uint64_t capacity_mask; /**< Bitmask used for indexing (`capacity - 1`). */
    int size;               /**< Current number of elements in the set. */
} Int64StaticHashSet;

/**
 * @brief Macro helper to declare and initialize a stack-allocated
 * `Int64StaticHashSet` instance along with its underlying key array.
 *
 * @param[out] name Name of the `Int64StaticHashSet` variable to create.
 * @param[in] cap Capacity of the hash set; must be a compile-time constant
 * positive integral value and a power of 2.
 */
#define DECLARE_INT64_STATIC_HASH_SET(name, cap)                               \
    static_assert((cap) > 0 && ((cap) & ((cap) - 1)) == 0,                     \
                  "Static hash set capacity (" #cap ") must be a power of 2"); \
    int64_t name##_keys[cap];                                                  \
    for (uint64_t i = 0; i < (uint64_t)(cap); ++i) {                           \
        name##_keys[i] = INT64_STATIC_HASH_SET_EMPTY_KEY;                      \
    }                                                                          \
    Int64StaticHashSet name = {name##_keys, (cap) - 1, 0}

/**
 * @brief Adds a `key` to the static hash set.
 *
 * @param[in,out] set The `Int64StaticHashSet` to modify.
 * @param[in] key The value to add.
 *
 * @retval true The `key` was successfully inserted into the set.
 * @retval false The `key` already exists in the set.
 */
static inline bool Int64StaticHashSetAdd(Int64StaticHashSet *set, int64_t key) {
    const uint64_t capacity_mask = set->capacity_mask;
    uint64_t idx = Splitmix64(key) & capacity_mask;

    const int64_t *keys = set->keys;
    while (keys[idx] != INT64_STATIC_HASH_SET_EMPTY_KEY) {
        if (keys[idx] == key) {
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
 * @param[in] set The `Int64StaticHashSet` to inspect.
 *
 * @return The number of elements in `set`.
 */
static inline int Int64StaticHashSetGetSize(const Int64StaticHashSet *set) {
    return set->size;
}

#endif  // GAMESMANONE_CORE_DATA_STRUCTURES_INT64_STATIC_HASH_SET_H_
