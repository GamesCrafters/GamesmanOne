/**
 * @file int64_hash_map.c
 * @author Robert Shi (robertyishi@berkeley.edu)
 *         GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Linear-probing int64_t to int64_t hash map implementation.
 * @version 1.0.0
 * @date 2023-08-19
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
#include <stdlib.h>   // calloc, free

#include "core/misc.h"  // NextPrime

static int64_t Hash(int64_t key, int64_t capacity) {
    return ((uint64_t)key) % capacity;
}

static int64_t NextIndex(int64_t index, int64_t capacity) {
    return (index + 1) % capacity;
}

void Int64HashMapInit(Int64HashMap *map, double max_load_factor) {
    map->entries = NULL;
    map->capacity = 0;
    map->size = 0;
    if (max_load_factor > 0.75) max_load_factor = 0.75;
    if (max_load_factor < 0.25) max_load_factor = 0.25;
    map->max_load_factor = max_load_factor;
}

void Int64HashMapDestroy(Int64HashMap *map) {
    free(map->entries);
    map->entries = NULL;
    map->capacity = 0;
    map->size = 0;
}

static Int64HashMapIterator NewIterator(Int64HashMap *map, int64_t index) {
    Int64HashMapIterator iterator;
    iterator.map = map;
    iterator.index = index;
    return iterator;
}

Int64HashMapIterator Int64HashMapGet(Int64HashMap *map, int64_t key) {
    int64_t capacity = map->capacity;
    // Edge case: return invalid iterator if map is empty.
    if (capacity == 0) return NewIterator(map, capacity);
    int64_t index = Hash(key, capacity);
    while (map->entries[index].used) {
        if (map->entries[index].key == key) {
            return NewIterator(map, index);
        }
        index = NextIndex(index, capacity);
    }
    return NewIterator(map, capacity);
}

static bool Expand(Int64HashMap *map) {
    int64_t new_capacity = NextPrime(map->capacity * 2);
    Int64HashMapEntry *new_entries =
        (Int64HashMapEntry *)calloc(new_capacity, sizeof(Int64HashMapEntry));
    if (new_entries == NULL) return false;
    for (int64_t i = 0; i < map->capacity; ++i) {
        if (map->entries[i].used) {
            int64_t new_index = Hash(map->entries[i].key, new_capacity);
            while (new_entries[new_index].used) {
                new_index = NextIndex(new_index, new_capacity);
            }
            new_entries[new_index] = map->entries[i];
        }
    }
    free(map->entries);
    map->entries = new_entries;
    map->capacity = new_capacity;
    return true;
}

bool Int64HashMapSet(Int64HashMap *map, int64_t key, int64_t value) {
    // Check if resizing is needed.
    double load_factor;
    if (map->capacity == 0) {
        load_factor = INFINITY;
    } else {
        load_factor = (double)(map->size + 1) / (double)map->capacity;
    }
    if (load_factor > map->max_load_factor) {
        if (!Expand(map)) return false;
    }

    // Set value at key.
    int64_t capacity = map->capacity;
    int64_t index = Hash(key, capacity);
    while (map->entries[index].used) {
        if (map->entries[index].key == key) {
            map->entries[index].value = value;
            return true;
        }
        index = NextIndex(index, capacity);
    }
    map->entries[index].key = key;
    map->entries[index].value = value;
    map->entries[index].used = true;
    ++map->size;
    return true;
}

bool Int64HashMapContains(Int64HashMap *map, int64_t key) {
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
    return it->index >= 0 && it->index < it->map->capacity;
}

bool Int64HashMapIteratorNext(Int64HashMapIterator *it, int64_t *key,
                              int64_t *value) {
    Int64HashMap *map = it->map;
    for (int64_t i = it->index + 1; i < map->capacity; ++i) {
        if (map->entries[i].used) {
            it->index = i;
            if (key) *key = map->entries[i].key;
            if (value) *value = map->entries[i].value;
            return true;
        }
    }
    it->index = map->capacity;
    return false;
}
