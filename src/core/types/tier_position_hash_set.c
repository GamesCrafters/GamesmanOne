/**
 * @file tier_position_hash_set.c
 * @author Robert Shi (robertyishi@berkeley.edu)
 *         GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Linear-probing TierPosition hash set implementation.
 *
 * @version 1.0.0
 * @date 2024-01-24
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
#include <stdlib.h>   // free, calloc

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

void TierPositionHashSetDestroy(TierPositionHashSet *set) {
    free(set->entries);
    set->entries = NULL;
    set->capacity = 0;
    set->size = 0;
}

static int64_t TierPositionHashSetHash(TierPosition key, int64_t capacity) {
    int64_t a = (int64_t)key.tier;
    int64_t b = (int64_t)key.position;
    int64_t cantor_pairing = (a + b) * (a + b + 1) / 2 + a;
    return ((uint64_t)cantor_pairing) % capacity;
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

static bool TierPositionHashSetExpand(TierPositionHashSet *set) {
    int64_t new_capacity = NextPrime(set->capacity * 2);
    TierPositionHashSetEntry *new_entries = (TierPositionHashSetEntry *)calloc(
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
    free(set->entries);
    set->entries = new_entries;
    set->capacity = new_capacity;
    return true;
}

bool TierPositionHashSetAdd(TierPositionHashSet *set, TierPosition key) {
    // Check if resizing is needed.
    double load_factor;
    if (set->capacity == 0) {
        load_factor = INFINITY;
    } else {
        load_factor = (double)(set->size + 1) / (double)set->capacity;
    }
    if (load_factor > set->max_load_factor) {
        if (!TierPositionHashSetExpand(set)) return false;
    }

    // Set value at key.
    int64_t capacity = set->capacity;
    int64_t index = TierPositionHashSetHash(key, capacity);
    while (set->entries[index].used) {
        TierPosition this_key = set->entries[index].key;
        if (this_key.tier == key.tier && this_key.position == key.position) {
            return true;
        }
        index = (index + 1) % capacity;
    }
    set->entries[index].key = key;
    set->entries[index].used = true;
    ++set->size;
    return true;
}
