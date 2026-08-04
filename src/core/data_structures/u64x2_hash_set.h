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
 * Copyright (c) 2011 Google, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * CityHash, by Geoff Pike and Jyrki Alakuijala
 * http://code.google.com/p/cityhash/
 *
 * @brief Hashes a 128-bit integer into a 64-bit integer.
 *
 * @param[in] v The 128-bit integer to hash.
 *
 * @returns The resulting 64-bit hash value.
 */
static inline uint64_t U64x2HashSetInternalHash128to64(U64x2 v) {
    // Murmur-inspired hashing.
    const uint64_t kMul = 0x9ddfea08eb382d69ULL;
    uint64_t a = (v[0] ^ v[1]) * kMul;
    a ^= (a >> 47);
    uint64_t b = (v[1] ^ a) * kMul;
    b ^= (b >> 47);
    b *= kMul;

    return b;
}

/**
 * @copyright Adapted from Stack Overflow user responses.
 * Source:
 * https://stackoverflow.com/questions/26880863/testing-equality-between-two-m128i-variables
 *
 * @brief Tests two `U64x2` variables for equality.
 *
 * @param[in] a The first `U64x2` variable.
 * @param[in] b The second `U64x2` variable.
 *
 * @retval true If `a` and `b` are bitwise equal.
 * @retval false If `a` and `b` are not bitwise equal.
 */
static inline bool U64x2HashSetInternalM128Equal(U64x2 a, U64x2 b) {
    return a[0] == b[0] && a[1] == b[1];
}

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
    uint64_t idx = U64x2HashSetInternalHash128to64(key) & capacity_mask;
    while (hs->state[idx]) {
        if (U64x2HashSetInternalM128Equal(hs->keys[idx], key)) {
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
    uint64_t start_idx = U64x2HashSetInternalHash128to64(key) & capacity_mask;
    uint64_t idx = start_idx;

    while (hs->state[idx]) {
        if (U64x2HashSetInternalM128Equal(hs->keys[idx], key)) {
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
