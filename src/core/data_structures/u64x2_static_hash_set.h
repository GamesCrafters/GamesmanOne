/**
 * @file u64x2_static_hash_set.h
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @author GamesCrafters Research Group, UC Berkeley
 * @brief Fixed-capacity linear-probing U64x2 (packed unsigned 64-bit integer
 * x2) hash set.
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

#ifndef GAMESMANONE_CORE_DATA_STRUCTURES_U64X2_STATIC_HASH_SET_H_
#define GAMESMANONE_CORE_DATA_STRUCTURES_U64X2_STATIC_HASH_SET_H_

#include <assert.h>
#include <stdalign.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "config.h"
#include "core/data_structures/hash.h"
#include "core/types/simd.h"

/**
 * @brief Fixed-capacity linear probing `U64x2` hash set for storing keys on
 * the stack.
 *
 * @details Implementation note: benchmark results show that keeping the state
 * array is faster than relying on a sentinel value for keys due to expensive
 * SIMD comparison operations.
 */
typedef struct {
    alignas(GM_CACHE_LINE_SIZE) U64x2 *keys; /**< Key array. */
    uint8_t *state;         /**< Bucket state: 0 (empty) or 1 (occupied). */
    uint64_t capacity_mask; /**< Bitmask used for indexing (`capacity - 1`). */
    int size;               /**< Current number of elements in the set. */
} U64x2StaticHashSet;

/**
 * @brief Macro helper to declare and initialize a stack-allocated
 * `U64x2StaticHashSet` instance along with its underlying arrays.
 *
 * @param[out] name Name of the `U64x2StaticHashSet` variable to create.
 * @param[in] cap Capacity of the hash set; must be a compile-time constant
 * positive integral value and a power of 2.
 */
#define DECLARE_U64X2_STATIC_HASH_SET(name, cap)                               \
    static_assert((cap) > 0 && ((cap) & ((cap) - 1)) == 0,                     \
                  "Static hash set capacity (" #cap ") must be a power of 2"); \
    alignas(GM_CACHE_LINE_SIZE) U64x2 name##_keys[cap];                        \
    uint8_t name##_state[cap] = {0};                                           \
    U64x2StaticHashSet name = {name##_keys, name##_state, (cap) - 1, 0}

// Suppress an analyzer warning about hs->keys being uninitialized;
// hs->keys are never used when the corresponding states are 0.
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#endif
/**
 * @brief Adds `key` as a new key in `hs`.
 *
 * @details Does nothing and returns `false` if `hs` already contains `key`. If
 * `hs` is completely full, the behavior is undefined.
 *
 * @param[in,out] hs Destination hash set.
 * @param[in] key Key to add to the hash set.
 *
 * @retval true If `key` is successfully added as a new key.
 * @retval false If `hs` already contains `key`.
 */
static inline bool U64x2StaticHashSetAdd(U64x2StaticHashSet *hs, U64x2 key) {
    const uint64_t capacity_mask = hs->capacity_mask;
    uint64_t idx = Hash128to64(key[0], key[1]) & capacity_mask;

    U64x2 *keys = hs->keys;
    uint8_t *state = hs->state;
    while (state[idx]) {
        if (U64x2Equal(keys[idx], key)) {
            return false;
        }
        idx = (idx + 1ULL) & capacity_mask;
    }
    keys[idx] = key;
    state[idx] = 1;
    ++hs->size;

    return true;
}
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic pop
#endif

/**
 * @brief Tests if `hs` contains `key`.
 *
 * @param[in] hs Hash set to evaluate.
 * @param[in] key Key to look for.
 *
 * @retval true If `hs` contains `key`.
 * @retval false If `hs` does not contain `key`.
 */
static inline bool U64x2StaticHashSetContains(const U64x2StaticHashSet *hs,
                                              U64x2 key) {
    const uint64_t capacity_mask = hs->capacity_mask;
    uint64_t start_idx = Hash128to64(key[0], key[1]) & capacity_mask;
    uint64_t idx = start_idx;

    const U64x2 *keys = hs->keys;
    const uint8_t *state = hs->state;
    while (state[idx]) {
        if (U64x2Equal(keys[idx], key)) {
            return true;
        }

        idx = (idx + 1ULL) & capacity_mask;

        // Break to avoid an infinite loop if the set is completely full
        // and we have checked every single bucket.
        if (idx == start_idx) {
            break;
        }
    }

    return false;
}

/**
 * @brief Returns the number of elements in the static hash set.
 *
 * @param[in] set The `U64x2StaticHashSet` to inspect.
 *
 * @return The number of elements in `set`.
 */
static inline int U64x2StaticHashSetGetSize(const U64x2StaticHashSet *set) {
    return set->size;
}

#endif  // GAMESMANONE_CORE_DATA_STRUCTURES_U64X2_STATIC_HASH_SET_H_
