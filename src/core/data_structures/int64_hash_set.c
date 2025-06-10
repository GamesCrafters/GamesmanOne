/**
 * @file int64_hash_set.c
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Linear-probing int64_t hash set implementation.
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

#include "core/data_structures/int64_hash_set.h"

#include <assert.h>   // assert
#include <math.h>     // INFINITY
#include <stdbool.h>  // bool, true, false
#include <stddef.h>   // NULL
#include <stdint.h>   // int64_t, uint64_t

#include "core/gamesman_memory.h"

static int64_t Hash(int64_t key, int64_t capacity_mask) {
    uint64_t x = (uint64_t)key;  // sign-extend → cast
    x = (x ^ (x >> 30)) * UINT64_C(0xbf58476d1ce4e5b9);
    x = (x ^ (x >> 27)) * UINT64_C(0x94d049bb133111eb);
    return (x ^ (x >> 31)) & capacity_mask;
}

static int64_t NextIndex(int64_t index, int64_t capacity_mask) {
    return (index + 1) & capacity_mask;
}

void Int64HashSetInit(Int64HashSet *set, double max_load_factor) {
    set->entries = NULL;
    set->size = 0;
    if (max_load_factor > 0.75) max_load_factor = 0.75;
    if (max_load_factor < 0.25) max_load_factor = 0.25;
    set->max_load_factor = max_load_factor;
    set->capacity_mask = -1;
}

static bool Expand(Int64HashSet *set, int64_t new_mask) {
    Int64HashSetEntry *new_entries = (Int64HashSetEntry *)GamesmanCallocWhole(
        new_mask + 1, sizeof(Int64HashSetEntry));
    if (new_entries == NULL) return false;

    for (int64_t i = 0; i < set->capacity_mask + 1; ++i) {
        if (set->entries[i].used) {
            int64_t new_index = Hash(set->entries[i].key, new_mask);
            while (new_entries[new_index].used) {
                new_index = NextIndex(new_index, new_mask);
            }
            new_entries[new_index] = set->entries[i];
        }
    }
    GamesmanFree(set->entries);
    set->entries = new_entries;
    set->capacity_mask = new_mask;

    return true;
}

static int64_t MinCapacityMask(int64_t capacity) {
    if (capacity <= 0) return -1;

    capacity--;
    capacity |= capacity >> 1;
    capacity |= capacity >> 2;
    capacity |= capacity >> 4;
    capacity |= capacity >> 8;
    capacity |= capacity >> 16;
    capacity |= capacity >> 32;

    return capacity;
}

bool Int64HashSetReserve(Int64HashSet *set, int64_t size) {
    int64_t target_capacity_mask =
        MinCapacityMask((int64_t)((double)size / set->max_load_factor));
    if (target_capacity_mask <= set->capacity_mask) return true;

    return Expand(set, target_capacity_mask);
}

void Int64HashSetDestroy(Int64HashSet *set) {
    GamesmanFree(set->entries);
    set->entries = NULL;
    set->size = 0;
    set->max_load_factor = 0.0;
    set->capacity_mask = -1;
}

bool Int64HashSetAdd(Int64HashSet *set, int64_t key) {
    // Check if resizing is needed.
    if (set->capacity_mask < 0) {
        if (!Expand(set, 1)) return false;
    } else if ((double)(set->size + 1) >
               (double)(set->capacity_mask + 1) * set->max_load_factor) {
        int64_t new_capacity_mask = (set->capacity_mask << 1) | 1;
        if (!Expand(set, new_capacity_mask)) return false;
    }

    // Set value at key.
    int64_t index = Hash(key, set->capacity_mask);
    while (set->entries[index].used) {
        if (set->entries[index].key == key) return false;
        index = NextIndex(index, set->capacity_mask);
    }
    set->entries[index].key = key;
    set->entries[index].used = true;
    ++set->size;

    return true;
}

bool Int64HashSetContains(const Int64HashSet *set, int64_t key) {
    // Edge case: return false if set is empty.
    if (set->capacity_mask < 0) return false;

    int64_t index = Hash(key, set->capacity_mask);
    while (set->entries[index].used) {
        if (set->entries[index].key == key) return true;
        index = NextIndex(index, set->capacity_mask);
    }

    return false;
}
