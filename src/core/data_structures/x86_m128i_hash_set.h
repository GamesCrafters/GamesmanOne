/**
 * @file x86_m128i_hash_set.h
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Fixed-capacity inear-probing __m128i hash set.
 * @version 1.0.0
 * @date 2025-06-03
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
#ifndef GAMESMANONE_CORE_DATA_STRUCTURES_X86_M128I_HASH_SET_H_
#define GAMESMANONE_CORE_DATA_STRUCTURES_X86_M128I_HASH_SET_H_

#include <immintrin.h>
#include <stdalign.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#ifndef X86_M128I_HASH_SET_SIZE
#define X86_M128I_HASH_SET_SIZE 1024ULL
#endif

/**
 * @brief Fixed-capacity linear probing __m128i hash set. The capacity of the
 * hash set in each translation unit can be defined at compile time by defining
 * X86_M128I_HASH_SET_SIZE to a positive integer value before including this
 * header. If X86_M128I_HASH_SET_SIZE is not defined at compile time, a default
 * capacity of 1024 will be used. X86_M128I_HASH_SET_SIZE, whether defined or
 * not before the inclusion of this header, will become undefined after the
 * inclusion.
 *
 * @example The following example shows how to use a hash set with a fixed
 * capacity equal to 32:
 *
 *     #define X86_M128I_HASH_SET_SIZE 32ULL
 *     #include "core/data_structures/x86_m128i_hash_set.h"
 *     void foo(void) {
 *         X86M128iHashSet set;
 *         X86M128iHashSetInit(&set);
 *         // Add elements, test contains...
 *         // No dynamic allocation and no need to deallocate set
 *     }
 */
typedef struct {
    /** Elements in the set. */
    __m128i keys[X86_M128I_HASH_SET_SIZE];

    /** Bucket state: 0 (empty) or 1 (occupied) */
    uint8_t state[X86_M128I_HASH_SET_SIZE];

    /** Number of elements in the set. */
    int size;
} X86M128iHashSet;

// This function is adapted from Google CityHash
// Copyright (c) 2011 Google, Inc.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
// CityHash, by Geoff Pike and Jyrki Alakuijala
//
// http://code.google.com/p/cityhash/
static inline uint64_t __X86M128iHashSetHash128to64(__m128i v) {
    alignas(16) uint64_t bits[2];
    _mm_store_si128((__m128i *)bits, v);

    // Murmur-inspired hashing.
    const uint64_t kMul = 0x9ddfea08eb382d69ULL;
    uint64_t a = (bits[0] ^ bits[1]) * kMul;
    a ^= (a >> 47);
    uint64_t b = (bits[1] ^ a) * kMul;
    b ^= (b >> 47);
    b *= kMul;

    return b;
}

// https://stackoverflow.com/questions/26880863/testing-equality-between-two-m128i-variables
static inline bool __X86M128iHashSetM128Equal(__m128i a, __m128i b) {
    __m128i neq = _mm_xor_si128(a, b);
    return _mm_test_all_zeros(neq, neq);
}

/**
 * @brief Initializes the given hash set \p hs to an empty set.
 *
 * @param hs Hash set to initialize.
 */
static inline void X86M128iHashSetInit(X86M128iHashSet *hs) {
    hs->size = 0;
    memset(hs->state, 0, sizeof(hs->state));
}

/**
 * @brief Adds \p key as a new key in \p hs and returns \c true , or does
 * nothing and returns \c false if \p hs already contains \p key . If \p hs
 * already contains \c X86_M128I_HASH_SET_SIZE elements (1024 by default), the
 * behavior is undefined.
 *
 * @param hs Destination hash set.
 * @param key Key to add.
 * @return \c true if \p key is added as a new key, or
 * @return \c false if \p hs already contains \p key .
 */
static inline bool X86M128iHashSetAdd(X86M128iHashSet *hs, __m128i key) {
    uint64_t capacity_mask = X86_M128I_HASH_SET_SIZE - 1ULL;
    uint64_t idx = __X86M128iHashSetHash128to64(key) & capacity_mask;
    while (hs->state[idx]) {
        if (__X86M128iHashSetM128Equal(hs->keys[idx], key)) return false;
        idx = (idx + 1ULL) & capacity_mask;
    }
    hs->keys[idx] = key;
    hs->state[idx] = 1;
    ++hs->size;

    return true;
}

/**
 * @brief Tests if \p hs contains \p key .
 *
 * @param hs Hash set.
 * @param key Key to test.
 * @return \c true if \p hs contains \c key ,
 * @return \c false otherwise.
 */
static inline bool X86M128iHashSetContains(const X86M128iHashSet *hs,
                                           __m128i key) {
    uint64_t capacity_mask = X86_M128I_HASH_SET_SIZE - 1ULL;
    uint64_t idx = __X86M128iHashSetHash128to64(key) & capacity_mask;
    while (hs->state[idx]) {
        if (__X86M128iHashSetM128Equal(hs->keys[idx], key)) return true;
        idx = (idx + 1ULL) & capacity_mask;
    }

    return false;
}

#undef X86_M128I_HASH_SET_SIZE

#endif  // GAMESMANONE_CORE_DATA_STRUCTURES_X86_M128I_HASH_SET_H_
