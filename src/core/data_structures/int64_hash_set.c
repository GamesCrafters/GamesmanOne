/**
 * @file int64_hash_set.c
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Linear-probing int64_t hash set implementation.
 * @version 1.1.0
 * @date 2025-03-13
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

#include "core/data_structures/int64_hash_set.h"

#include <assert.h>   // assert
#include <math.h>     // INFINITY
#include <stdbool.h>  // bool, true, false
#include <stddef.h>   // NULL
#include <stdint.h>   // int64_t, uint64_t
#include <stdlib.h>   // calloc, free

#include "core/misc.h"  // NextPrime

static int64_t Hash(int64_t key, int64_t capacity) {
    return (int64_t)(((uint64_t)key) % capacity);
}

static int64_t NextIndex(int64_t index, int64_t capacity) {
    return (index + 1) % capacity;
}

void Int64HashSetInit(Int64HashSet *set, double max_load_factor) {
    set->entries = NULL;
    set->capacity = 0;
    set->size = 0;
    if (max_load_factor > 0.75) max_load_factor = 0.75;
    if (max_load_factor < 0.25) max_load_factor = 0.25;
    set->max_load_factor = max_load_factor;
}

static bool Expand(Int64HashSet *set, int64_t new_capacity) {
    Int64HashSetEntry *new_entries =
        (Int64HashSetEntry *)calloc(new_capacity, sizeof(Int64HashSetEntry));
    if (new_entries == NULL) return false;

    for (int64_t i = 0; i < set->capacity; ++i) {
        if (set->entries[i].used) {
            int64_t new_index = Hash(set->entries[i].key, new_capacity);
            while (new_entries[new_index].used) {
                new_index = NextIndex(new_index, new_capacity);
            }
            new_entries[new_index] = set->entries[i];
        }
    }
    free(set->entries);
    set->entries = new_entries;
    set->capacity = new_capacity;
    return true;
}

bool Int64HashSetReserve(Int64HashSet *set, int64_t size) {
    int64_t target_capacity =
        NextPrime((int64_t)((double)size / set->max_load_factor));
    if (target_capacity <= set->capacity) return true;

    return Expand(set, target_capacity);
}

void Int64HashSetDestroy(Int64HashSet *set) {
    free(set->entries);
    set->entries = NULL;
    set->capacity = 0;
    set->size = 0;
}

bool Int64HashSetAdd(Int64HashSet *set, int64_t key) {
    // Check if resizing is needed.
    double load_factor = (set->capacity == 0)
                             ? INFINITY
                             : (double)(set->size + 1) / (double)set->capacity;
    if (set->capacity == 0 || load_factor > set->max_load_factor) {
        int64_t new_capacity = NextPrime(set->capacity * 2);
        if (!Expand(set, new_capacity)) return false;
    }

    // Set value at key.
    int64_t capacity = set->capacity;
    int64_t index = Hash(key, capacity);
    while (set->entries[index].used) {
        if (set->entries[index].key == key) return true;
        index = NextIndex(index, capacity);
    }
    set->entries[index].key = key;
    set->entries[index].used = true;
    ++set->size;

    return true;
}

bool Int64HashSetContains(const Int64HashSet *set, int64_t key) {
    int64_t capacity = set->capacity;
    // Edge case: return false if set is empty.
    if (capacity == 0) return false;

    int64_t index = Hash(key, capacity);
    while (set->entries[index].used) {
        if (set->entries[index].key == key) return true;
        index = NextIndex(index, capacity);
    }

    return false;
}
