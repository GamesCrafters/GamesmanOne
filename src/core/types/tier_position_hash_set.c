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

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "core/data_structures/hash.h"
#include "core/gamesman_memory.h"
#include "core/types/base.h"

void TierPositionHashSetInit(TierPositionHashSet *set, double max_load_factor) {
    set->entries = NULL;
    set->size = 0;
    if (max_load_factor > 0.75) max_load_factor = 0.75;
    if (max_load_factor < 0.25) max_load_factor = 0.25;
    set->max_load_factor = max_load_factor;
    set->capacity_mask = -1;
}

static int64_t TierPositionHashSetHash(TierPosition key, int64_t mask) {
    int64_t a = (int64_t)key.tier;
    int64_t b = (int64_t)key.position;

    return Hash128to64(a, b) & mask;
}

static bool TierPositionHashSetExpand(TierPositionHashSet *set,
                                      int64_t new_mask) {
    TierPositionHashSetEntry *new_entries =
        (TierPositionHashSetEntry *)GamesmanCallocWhole(
            new_mask + 1, sizeof(TierPositionHashSetEntry));
    if (new_entries == NULL) return false;

    for (int64_t i = 0; i <= set->capacity_mask; ++i) {
        if (set->entries[i].used) {
            int64_t new_index =
                TierPositionHashSetHash(set->entries[i].key, new_mask);
            while (new_entries[new_index].used) {
                new_index = (new_index + 1) & new_mask;
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

bool TierPositionHashSetReserve(TierPositionHashSet *set, int64_t size) {
    int64_t target_capacity_mask =
        MinCapacityMask((int64_t)((double)size / set->max_load_factor));
    if (target_capacity_mask <= set->capacity_mask) return true;

    return TierPositionHashSetExpand(set, target_capacity_mask);
}

void TierPositionHashSetDestroy(TierPositionHashSet *set) {
    GamesmanFree(set->entries);
    set->entries = NULL;
    set->size = 0;
    set->max_load_factor = 0.0;
    set->capacity_mask = -1;
}

bool TierPositionHashSetContains(TierPositionHashSet *set, TierPosition key) {
    // Edge case: return false if set is empty.
    if (set->capacity_mask < 0) return false;

    int64_t index = TierPositionHashSetHash(key, set->capacity_mask);
    while (set->entries[index].used) {
        TierPosition this_key = set->entries[index].key;
        if (this_key.tier == key.tier && this_key.position == key.position) {
            return true;
        }
        index = (index + 1) & set->capacity_mask;
    }

    return false;
}

// Assumes 64-bit integer overflow is not possible.
bool TierPositionHashSetAdd(TierPositionHashSet *set, TierPosition key) {
    // Check if resizing is needed.
    if (set->capacity_mask < 0) {
        if (!TierPositionHashSetExpand(set, 1)) return false;
    } else if ((double)(set->size + 1) >
               (double)(set->capacity_mask + 1) * set->max_load_factor) {
        int64_t new_capacity_mask = (set->capacity_mask << 1) | 1;
        if (!TierPositionHashSetExpand(set, new_capacity_mask)) return false;
    }

    // Set value at key.
    int64_t index = TierPositionHashSetHash(key, set->capacity_mask);
    while (set->entries[index].used) {
        TierPosition this_key = set->entries[index].key;
        if (this_key.tier == key.tier && this_key.position == key.position) {
            return false;
        }
        index = (index + 1) & set->capacity_mask;
    }
    set->entries[index].key = key;
    set->entries[index].used = true;
    ++set->size;

    return true;
}
