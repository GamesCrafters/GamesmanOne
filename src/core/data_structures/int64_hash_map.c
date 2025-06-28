/**
 * @file int64_hash_map.c
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Linear-probing (open addressing) int64_t to int64_t hash map.
 * @version 1.0.4
 * @date 2025-04-26
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

#include <assert.h>   // assert
#include <math.h>     // INFINITY
#include <stdbool.h>  // bool, true, false
#include <stddef.h>   // NULL
#include <stdint.h>   // int64_t, uint64_t
#include <string.h>   // memset

#include "core/data_structures/hash.h"
#include "core/gamesman_memory.h"

static int64_t Hash(int64_t key, int64_t capacity_mask) {
    return (int64_t)Splitmix64((uint64_t)key) & capacity_mask;
}

static int64_t NextIndex(int64_t index, int64_t capacity_mask) {
    return (index + 1) & capacity_mask;
}

void Int64HashMapInit(Int64HashMap *map, double max_load_factor) {
    Int64HashMapInitAllocator(map, max_load_factor, NULL);
}

void Int64HashMapInitAllocator(Int64HashMap *map, double max_load_factor,
                               GamesmanAllocator *allocator) {
    map->allocator = GamesmanAllocatorAddRef(allocator);
    map->entries = NULL;
    map->capacity_mask = -1;
    map->size = 0;
    if (max_load_factor > 0.75) max_load_factor = 0.75;
    if (max_load_factor < 0.25) max_load_factor = 0.25;
    map->max_load_factor = max_load_factor;
}

void Int64HashMapDestroy(Int64HashMap *map) {
    GamesmanAllocatorDeallocate(map->allocator, map->entries);
    GamesmanAllocatorRelease(map->allocator);
    map->allocator = NULL;
    map->entries = NULL;
    map->capacity_mask = -1;
    map->size = 0;
}

static Int64HashMapIterator NewIterator(const Int64HashMap *map,
                                        int64_t index) {
    Int64HashMapIterator iterator;
    iterator.map = map;
    iterator.index = index;
    return iterator;
}

Int64HashMapIterator Int64HashMapGet(const Int64HashMap *map, int64_t key) {
    // Edge case: return invalid iterator if map is empty.
    if (map->capacity_mask < 0) return NewIterator(map, -1);

    int64_t index = Hash(key, map->capacity_mask);
    while (map->entries[index].used) {
        if (map->entries[index].key == key) {
            return NewIterator(map, index);
        }
        index = NextIndex(index, map->capacity_mask);
    }

    return NewIterator(map, map->capacity_mask + 1);
}

static bool Expand(Int64HashMap *map, int64_t new_mask) {
    size_t alloc_size = (new_mask + 1) * sizeof(Int64HashMapEntry);
    Int64HashMapEntry *new_entries =
        (Int64HashMapEntry *)GamesmanAllocatorAllocate(map->allocator,
                                                       alloc_size);
    if (new_entries == NULL) return false;
    memset(new_entries, 0, alloc_size);

    for (int64_t i = 0; i <= map->capacity_mask; ++i) {
        if (map->entries[i].used) {
            int64_t new_index = Hash(map->entries[i].key, new_mask);
            while (new_entries[new_index].used) {
                new_index = NextIndex(new_index, new_mask);
            }
            new_entries[new_index] = map->entries[i];
        }
    }
    GamesmanAllocatorDeallocate(map->allocator, map->entries);
    map->entries = new_entries;
    map->capacity_mask = new_mask;

    return true;
}

bool Int64HashMapSet(Int64HashMap *map, int64_t key, int64_t value) {
    // Check if resizing is needed.
    if (map->capacity_mask < 0) {
        if (!Expand(map, 1)) return false;
    } else if ((double)(map->size + 1) >
               (double)(map->capacity_mask + 1) * map->max_load_factor) {
        int64_t new_capacity_mask = (map->capacity_mask << 1) | 1;
        if (!Expand(map, new_capacity_mask)) return false;
    }

    // Set value at key.
    int64_t index = Hash(key, map->capacity_mask);
    while (map->entries[index].used) {
        if (map->entries[index].key == key) {
            map->entries[index].value = value;
            return true;
        }
        index = NextIndex(index, map->capacity_mask);
    }
    map->entries[index].key = key;
    map->entries[index].value = value;
    map->entries[index].used = true;
    ++map->size;

    return true;
}

bool Int64HashMapContains(const Int64HashMap *map, int64_t key) {
    Int64HashMapIterator it = Int64HashMapGet(map, key);
    return Int64HashMapIteratorIsValid(&it);
}

Int64HashMapIterator Int64HashMapBegin(Int64HashMap *map) {
    return (Int64HashMapIterator){.map = map, .index = -1};
}

int64_t Int64HashMapIteratorKey(const Int64HashMapIterator *it) {
    return it->map->entries[it->index].key;
}

int64_t Int64HashMapIteratorValue(const Int64HashMapIterator *it) {
    return it->map->entries[it->index].value;
}

bool Int64HashMapIteratorIsValid(const Int64HashMapIterator *it) {
    return it->index >= 0 && it->index <= it->map->capacity_mask;
}

bool Int64HashMapIteratorNext(Int64HashMapIterator *it, int64_t *key,
                              int64_t *value) {
    const Int64HashMap *map = it->map;
    while (++it->index <= map->capacity_mask) {
        if (map->entries[it->index].used) {
            if (key) *key = map->entries[it->index].key;
            if (value) *value = map->entries[it->index].value;
            return true;
        }
    }

    return false;
}
