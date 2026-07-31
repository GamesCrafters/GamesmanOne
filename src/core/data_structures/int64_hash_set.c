/**
 * @file int64_hash_set.c
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Int64HashSet implementation.
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

#include <stdbool.h>
#include <stdint.h>

#include "core/data_structures/hash.h"
#include "core/gamesman_memory.h"

bool Int64HashSetInternalExpand(Int64HashSet *set) {
    // If old_keys is non-NULL, this is a normal expansion step;
    // if old_keys in NULL, this is the lazy initialization step.
    // Initial capacity is 128, so the mask 127 (0x7F).
    uint64_t new_mask = set->keys ? ((set->mask << 1) | 1ULL) : 0x7F;

    return Int64HashSetInternalExpandExplicit(set, new_mask);
}

bool Int64HashSetInternalExpandExplicit(Int64HashSet *set, uint64_t new_mask) {
    // Allocate new array and initialize it
    int64_t *__restrict new_keys =
        (int64_t *)GamesmanMalloc((new_mask + 1) * sizeof(int64_t));
    if (new_keys == NULL) {
        return false;
    }
    for (uint64_t i = 0; i <= new_mask; ++i) {
        new_keys[i] = INT64_HASH_SET_EMPTY_KEY;
    }

    // Hoist pointer to local so the compiler doesn't worry about memory
    // aliasing during the loop.
    int64_t *__restrict old_keys = set->keys;

    // Only attempt to rehash if we had existing keys
    if (old_keys != NULL) {
        uint64_t old_mask = set->mask;
        for (uint64_t i = 0; i <= old_mask; ++i) {
            int64_t key = old_keys[i];
            if (key != INT64_HASH_SET_EMPTY_KEY) {
                uint64_t new_index = Splitmix64(key) & new_mask;
                while (new_keys[new_index] != INT64_HASH_SET_EMPTY_KEY) {
                    new_index = (new_index + 1) & new_mask;
                }
                new_keys[new_index] = key;
            }
        }
    }

    // Update internal data
    GamesmanFree(old_keys);
    set->keys = new_keys;
    set->mask = new_mask;
    set->max_size = (int64_t)((new_mask + 1) / set->inv_max_load_factor);

    return true;
}
