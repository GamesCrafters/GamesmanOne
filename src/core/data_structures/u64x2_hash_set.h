/**
 * @file u64x2_hash_set.h
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
#ifndef GAMESMANONE_CORE_DATA_STRUCTURES_U64X2_HASH_SET_H_
#define GAMESMANONE_CORE_DATA_STRUCTURES_U64X2_HASH_SET_H_

#include <stdalign.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "config.h"
#include "core/data_structures/hash.h"
#include "core/types/simd.h"

#ifndef U64X2_HASH_SET_SIZE
/** Default capacity for the hash set if not defined at compile time. */
#define U64X2_HASH_SET_SIZE 1024ULL
#endif

/**
 * @brief Fixed-capacity linear probing `U64x2` hash set.
 *
 * @details The capacity of the hash set in each translation unit can be defined
 * at compile time by defining `U64X2_HASH_SET_SIZE` to a positive integer
 * value before including this header. If `U64X2_HASH_SET_SIZE` is not
 * defined at compile time, a default capacity of 1024 will be used.
 * `U64X2_HASH_SET_SIZE`, whether defined or not before the inclusion of
 * this header, will become undefined after the inclusion.
 *
 * Example usage:
 * ```c
 * #define U64X2_HASH_SET_SIZE 32ULL
 * #include "core/data_structures/U64X2_hash_set.h"
 * void foo(void) {
 *     U64x2HashSet set;
 *     U64x2HashSetInit(&set);
 *     // Add elements, test contains...
 *     // No dynamic allocation and no need to deallocate set
 * }
 * ```
 *
 * Implementation note: benchmark results show that keeping the state array is
 * faster than relying on a sentinel value for keys due to expensive SIMD
 * comparison operations.
 */
typedef struct {
    /** Elements in the set. */
    alignas(GM_CACHE_LINE_SIZE) U64x2 keys[U64X2_HASH_SET_SIZE];

    /** Bucket state: 0 (empty) or 1 (occupied). */
    uint8_t state[U64X2_HASH_SET_SIZE];

    /** Number of elements in the set. */
    int size;
} U64x2HashSet;

/**
 * @brief Initializes the given hash set `hs` to an empty set.
 *
 * @param[out] hs Hash set to initialize.
 */
static inline void U64x2HashSetInit(U64x2HashSet *hs) {
    hs->size = 0;
    memset(hs->state, 0, sizeof(hs->state));
}

// Suppress a analyzer warning about hs->keys being uninitialized;
// hs->keys are never used when the corresponding states are 0.
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#endif
/**
 * @brief Adds `key` as a new key in `hs`.
 *
 * @details Does nothing and returns `false` if `hs` already contains `key`. If
 * `hs` already contains `U64X2_HASH_SET_SIZE` elements (1024 by default),
 * the behavior is undefined.
 *
 * @param[in,out] hs Destination hash set.
 * @param[in] key Key to add to the hash set.
 *
 * @retval true If `key` is successfully added as a new key.
 * @retval false If `hs` already contains `key`.
 */
static inline bool U64x2HashSetAdd(U64x2HashSet *hs, U64x2 key) {
    uint64_t capacity_mask = U64X2_HASH_SET_SIZE - 1ULL;
    uint64_t idx = Hash128to64(key[0], key[1]) & capacity_mask;

    const U64x2 *keys = hs->keys;
    const uint8_t *state = hs->state;
    while (state[idx]) {
        if (U64x2Equal(keys[idx], key)) {
            return false;
        }
        idx = (idx + 1ULL) & capacity_mask;
    }
    hs->keys[idx] = key;
    hs->state[idx] = 1;
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
static inline bool U64x2HashSetContains(const U64x2HashSet *hs, U64x2 key) {
    uint64_t capacity_mask = U64X2_HASH_SET_SIZE - 1ULL;
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

#undef U64X2_HASH_SET_SIZE

#endif  // GAMESMANONE_CORE_DATA_STRUCTURES_U64X2_HASH_SET_H_
