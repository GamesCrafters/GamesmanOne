/**
 * @file int64_hash_map.c
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief `Int64HashMap` implementation (cold-path functions).
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

#include "core/data_structures/int64_hash_map.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "core/data_structures/hash.h"
#include "core/gamesman_memory.h"

static bool ExpandTo(Int64HashMap *map, uint64_t new_mask) {
    uint64_t new_capacity = new_mask + 1;

    // Allocate new key array.
    int64_t *__restrict new_keys = (int64_t *)GamesmanAllocatorAllocate(
        map->allocator, new_capacity * sizeof(int64_t));
    if (new_keys == NULL) {
        return false;
    }

    // Allocate new value array.
    int64_t *__restrict new_values = (int64_t *)GamesmanAllocatorAllocate(
        map->allocator, new_capacity * sizeof(int64_t));
    if (new_values == NULL) {
        GamesmanAllocatorDeallocate(map->allocator, new_keys);
        return false;
    }

    // Initialize all key slots to the sentinel.
    for (uint64_t i = 0; i < new_capacity; ++i) {
        new_keys[i] = INT64_HASH_MAP_EMPTY_KEY;
    }

    // Hoist old pointers to locals.
    int64_t *__restrict old_keys = map->keys;
    int64_t *__restrict old_values = map->values;

    // Rehash existing entries if any.
    if (old_keys != NULL) {
        uint64_t old_mask = map->mask;
        for (uint64_t i = 0; i <= old_mask; ++i) {
            int64_t key = old_keys[i];
            if (key != INT64_HASH_MAP_EMPTY_KEY) {
                uint64_t new_index = Splitmix64(key) & new_mask;
                while (new_keys[new_index] != INT64_HASH_MAP_EMPTY_KEY) {
                    new_index = (new_index + 1) & new_mask;
                }
                new_keys[new_index] = key;
                new_values[new_index] = old_values[i];
            }
        }
    }

    GamesmanAllocatorDeallocate(map->allocator, old_keys);
    GamesmanAllocatorDeallocate(map->allocator, old_values);
    map->keys = new_keys;
    map->mask = new_mask;
    map->max_size = (int64_t)((new_mask + 1) / map->inv_max_load_factor);
    map->values = new_values;

    return true;
}

bool Int64HashMapInternalExpand(Int64HashMap *map) {
    // If keys is non-NULL, this is a normal expansion step;
    // if keys is NULL, this is the lazy initialization step.
    // Initial capacity is 128, so the mask is 127 (0x7F).
    uint64_t new_mask = map->keys ? ((map->mask << 1) | 1ULL) : 0x7F;

    return ExpandTo(map, new_mask);
}
