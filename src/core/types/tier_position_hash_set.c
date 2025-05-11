/**
 * @file tier_position_hash_set.c
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Linear-probing TierPosition hash set implementation.
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

#include "core/types/tier_position_hash_set.h"

#include <math.h>     // INFINITY
#include <stdbool.h>  // bool
#include <stddef.h>   // NULL
#include <stdint.h>   // int64_t

#include "core/gamesman_memory.h"
#include "core/misc.h"
#include "core/types/base.h"

void TierPositionHashSetInit(TierPositionHashSet *set, double max_load_factor) {
    set->entries = NULL;
    set->capacity = 0;
    set->size = 0;
    if (max_load_factor > 0.75) max_load_factor = 0.75;
    if (max_load_factor < 0.25) max_load_factor = 0.25;
    set->max_load_factor = max_load_factor;
}

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
static uint64_t Hash128to64(uint64_t lo, uint64_t hi) {
    // Murmur-inspired hashing.
    const uint64_t kMul = 0x9ddfea08eb382d69ULL;
    uint64_t a = (lo ^ hi) * kMul;
    a ^= (a >> 47);
    uint64_t b = (hi ^ a) * kMul;
    b ^= (b >> 47);
    b *= kMul;

    return b;
}

static int64_t TierPositionHashSetHash(TierPosition key, int64_t capacity) {
    int64_t a = (int64_t)key.tier;
    int64_t b = (int64_t)key.position;

    return Hash128to64(a, b) % capacity;
}

static bool TierPositionHashSetExpand(TierPositionHashSet *set,
                                      int64_t new_capacity) {
    TierPositionHashSetEntry *new_entries =
        (TierPositionHashSetEntry *)GamesmanCallocWhole(
            new_capacity, sizeof(TierPositionHashSetEntry));
    if (new_entries == NULL) return false;
    for (int64_t i = 0; i < set->capacity; ++i) {
        if (set->entries[i].used) {
            int64_t new_index =
                TierPositionHashSetHash(set->entries[i].key, new_capacity);
            while (new_entries[new_index].used) {
                new_index = (new_index + 1) % new_capacity;
            }
            new_entries[new_index] = set->entries[i];
        }
    }
    GamesmanFree(set->entries);
    set->entries = new_entries;
    set->capacity = new_capacity;
    return true;
}

bool TierPositionHashSetReserve(TierPositionHashSet *set, int64_t size) {
    int64_t target_capacity =
        NextPrime((int64_t)((double)size / set->max_load_factor));
    if (target_capacity <= set->capacity) return true;

    return TierPositionHashSetExpand(set, target_capacity);
}

void TierPositionHashSetDestroy(TierPositionHashSet *set) {
    GamesmanFree(set->entries);
    set->entries = NULL;
    set->capacity = 0;
    set->size = 0;
}

bool TierPositionHashSetContains(TierPositionHashSet *set, TierPosition key) {
    int64_t capacity = set->capacity;
    // Edge case: return false if set is empty.
    if (set->capacity == 0) return false;
    int64_t index = TierPositionHashSetHash(key, capacity);
    while (set->entries[index].used) {
        TierPosition this_key = set->entries[index].key;
        if (this_key.tier == key.tier && this_key.position == key.position) {
            return true;
        }
        index = (index + 1) % capacity;
    }
    return false;
}

bool TierPositionHashSetAdd(TierPositionHashSet *set, TierPosition key) {
    // Check if resizing is needed.
    double load_factor = (set->capacity == 0)
                             ? INFINITY
                             : (double)(set->size + 1) / (double)set->capacity;
    if (set->capacity == 0 || load_factor > set->max_load_factor) {
        int64_t new_capacity = NextPrime(set->capacity * 2);
        if (!TierPositionHashSetExpand(set, new_capacity)) return false;
    }

    // Set value at key.
    int64_t capacity = set->capacity;
    int64_t index = TierPositionHashSetHash(key, capacity);
    while (set->entries[index].used) {
        TierPosition this_key = set->entries[index].key;
        if (this_key.tier == key.tier && this_key.position == key.position) {
            return false;
        }
        index = (index + 1) % capacity;
    }
    set->entries[index].key = key;
    set->entries[index].used = true;
    ++set->size;
    return true;
}
