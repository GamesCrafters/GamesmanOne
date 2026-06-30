/**
 * @file hash.h
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Non-cryptographic hash functions.
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

#ifndef GAMESMANONE_CORE_DATA_STRUCTURES_HASH_H_
#define GAMESMANONE_CORE_DATA_STRUCTURES_HASH_H_

#include <stdint.h>

/**
 * @brief Advances the SplitMix64 state and returns a 64-bit pseudorandom
 * number.
 *
 * SplitMix64 is a fast, fixed-increment splittable pseudorandom number
 * generator (PRNG). It is highly deterministic and widely used to initialize
 * seeds for other generators (like Xoshiro/Xoroshiro) because it possesses
 * excellent avalanche characteristics.
 *
 * @param[in] state The current internal 64-bit state/seed value.
 *
 * @return A 64-bit pseudo-randomly scrambled unsigned integer.
 *
 * @see https://rosettacode.org/wiki/Pseudo-random_numbers/Splitmix64
 */
static inline uint64_t Splitmix64(uint64_t state) {
    uint64_t x = state + UINT64_C(0x9e3779b97f4a7c15);
    x = (x ^ (x >> 30)) * UINT64_C(0xbf58476d1ce4e5b9);
    x = (x ^ (x >> 27)) * UINT64_C(0x94d049bb133111eb);
    return x ^ (x >> 31);
}

/**
 * @copyright Copyright (c) 2011 Google, Inc.
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
 *
 * @brief Mixes a 128-bit value down to a 64-bit hash.
 *
 * This function uses a MurmurHash3-inspired multiplication and xorshift
 * avalanche technique to combine two 64-bit integers (`lo` and `hi`) into
 * a single, highly distinct 64-bit hash value.
 *
 * It provides excellent bit distribution and is ideal for constructing hash
 * table keys out of composite types, UUIDs, or large 128-bit identifiers.
 *
 * @note This function is adapted from Google CityHash
 *
 * @param[in] lo The lower 64 bits of the 128-bit value to hash.
 * @param[in] hi The upper 64 bits of the 128-bit value to hash.
 *
 * @return A thoroughly scrambled 64-bit unsigned integer hash.
 *
 * @see [CityHash Source](http://code.google.com/p/cityhash/)
 */
static inline uint64_t Hash128to64(uint64_t lo, uint64_t hi) {
    // Murmur-inspired hashing.
    const uint64_t kMul = 0x9ddfea08eb382d69ULL;
    uint64_t a = (lo ^ hi) * kMul;
    a ^= (a >> 47);
    uint64_t b = (hi ^ a) * kMul;
    b ^= (b >> 47);
    b *= kMul;

    return b;
}

#endif  // GAMESMANONE_CORE_DATA_STRUCTURES_HASH_H_
