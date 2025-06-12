/**
 * @file int64_to_ptr_chained_hash_map.c
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Separate chaining int64_t to generic pointer (void *) hash map
 * implementation.
 * @version 1.0.0
 * @date 2025-06-09
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

#include "core/data_structures/int64_to_ptr_chained_hash_map.h"

#include <math.h>     // INFINITY
#include <stdbool.h>  // bool, true, false
#include <stddef.h>   // NULL
#include <stdint.h>   // int64_t
#include <string.h>   // memset

#include "core/data_structures/hash.h"
#include "core/gamesman_memory.h"

void Int64ToPtrChainedHashMapInit(Int64ToPtrChainedHashMap *map,
                                  double max_load_factor) {
    map->buckets = NULL;
    map->capacity_mask = -1;
    map->size = 0;
    if (max_load_factor > 0.75) max_load_factor = 0.75;
    if (max_load_factor < 0.25) max_load_factor = 0.25;
    map->max_load_factor = max_load_factor;
}

void Int64ToPtrChainedHashMapDestroy(Int64ToPtrChainedHashMap *map) {
    for (int64_t i = 0; i < map->capacity_mask + 1; ++i) {
        Int64ToPtrChainedHashMapEntry *walker = map->buckets[i];
        while (walker) {
            Int64ToPtrChainedHashMapEntry *next = walker->next;
            GamesmanFree(walker);
            walker = next;
        }
    }
    GamesmanFree(map->buckets);
    map->size = 0;
    map->buckets = NULL;
    map->capacity_mask = -1;
    map->max_load_factor = 0.0;
}

static int64_t Hash(int64_t key, int64_t capacity_mask) {
    return (int64_t)Splitmix64((uint64_t)key) & capacity_mask;
}

bool Int64ToPtrChainedHashMapContains(const Int64ToPtrChainedHashMap *map,
                                      int64_t key) {
    Int64ToPtrChainedHashMapIterator it = Int64ToPtrChainedHashMapGet(map, key);

    return Int64ToPtrChainedHashMapIteratorIsValid(&it);
}

static Int64ToPtrChainedHashMapIterator NewIterator(
    const Int64ToPtrChainedHashMap *map, int64_t bucket_index,
    Int64ToPtrChainedHashMapEntry *cur) {
    Int64ToPtrChainedHashMapIterator ret;
    ret.map = map;
    ret.bucket_index = bucket_index;
    ret.cur = cur;

    return ret;
}

Int64ToPtrChainedHashMapIterator Int64ToPtrChainedHashMapGet(
    const Int64ToPtrChainedHashMap *map, int64_t key) {
    if (map->capacity_mask < 0) return NewIterator(map, -1, NULL);

    int64_t index = Hash(key, map->capacity_mask);
    Int64ToPtrChainedHashMapEntry *walker = map->buckets[index];
    while (walker != NULL) {
        if (walker->key == key) {
            return NewIterator(map, index, walker);
        }
        walker = walker->next;
    }

    return NewIterator(map, -1, NULL);
}

static bool Expand(Int64ToPtrChainedHashMap *map, int64_t new_mask) {
    Int64ToPtrChainedHashMapEntry **new_buckets =
        (Int64ToPtrChainedHashMapEntry **)GamesmanCallocWhole(
            new_mask + 1, sizeof(Int64ToPtrChainedHashMapEntry *));
    if (new_buckets == NULL) return false;

    for (int64_t i = 0; i < map->capacity_mask + 1; ++i) {
        Int64ToPtrChainedHashMapEntry *entry = map->buckets[i];
        while (entry) {
            int64_t new_index = Hash(entry->key, new_mask);
            Int64ToPtrChainedHashMapEntry *next = entry->next;
            entry->next = new_buckets[new_index];
            new_buckets[new_index] = entry;
            entry = next;
        }
    }

    GamesmanFree(map->buckets);
    map->buckets = new_buckets;
    map->capacity_mask = new_mask;

    return true;
}

bool Int64ToPtrChainedHashMapSet(Int64ToPtrChainedHashMap *map, int64_t key,
                                 void *value) {
    // Check if resizing is needed.
    if (map->capacity_mask < 0) {
        if (!Expand(map, 1)) return false;
    } else if ((double)(map->size + 1) >
               (double)(map->capacity_mask + 1) * map->max_load_factor) {
        int64_t new_capacity_mask = (map->capacity_mask << 1) | 1;
        if (!Expand(map, new_capacity_mask)) return false;
    }

    // Look for existing key to replace its value.
    int64_t index = Hash(key, map->capacity_mask);
    Int64ToPtrChainedHashMapEntry *entry =
        map->buckets[index];  // Find the bucket.
    while (entry) {
        if (entry->key == key) {  // If key already exists, replace old value.
            entry->value = value;
            return true;
        }
        entry = entry->next;
    }

    // Key does not exist. Create a new entry.
    entry = (Int64ToPtrChainedHashMapEntry *)GamesmanMalloc(
        sizeof(Int64ToPtrChainedHashMapEntry));
    if (!entry) return false;

    entry->key = key;
    entry->value = value;
    entry->next = map->buckets[index];
    map->buckets[index] = entry;
    ++map->size;

    return true;
}

void Int64ToPtrChainedHashMapRemove(Int64ToPtrChainedHashMap *map,
                                    int64_t key) {
    if (map->capacity_mask < 0) return;

    int64_t index = Hash(key, map->capacity_mask);
    Int64ToPtrChainedHashMapEntry *entry = map->buckets[index];
    Int64ToPtrChainedHashMapEntry **prev_next = &map->buckets[index];

    while (entry) {
        if (entry->key == key) {
            *prev_next = entry->next;
            GamesmanFree(entry);
            --map->size;
            return;
        }
        prev_next = &entry->next;
        entry = entry->next;
    }
}

Int64ToPtrChainedHashMapIterator Int64ToPtrChainedHashMapBegin(
    const Int64ToPtrChainedHashMap *map) {
    Int64ToPtrChainedHashMapIterator it;
    it.map = map;
    it.bucket_index = 0;
    it.cur = NULL;

    // Advance to first valid bucket
    while (it.bucket_index < map->capacity_mask + 1) {
        if (map->buckets[it.bucket_index]) {
            it.cur = map->buckets[it.bucket_index];
            break;
        }
        ++it.bucket_index;
    }

    return it;
}

bool Int64ToPtrChainedHashMapIteratorIsValid(
    const Int64ToPtrChainedHashMapIterator *it) {
    return it->cur != NULL;
}

int64_t Int64ToPtrChainedHashMapIteratorKey(
    const Int64ToPtrChainedHashMapIterator *it) {
    return it->cur->key;
}

void *Int64ToPtrChainedHashMapIteratorValue(
    const Int64ToPtrChainedHashMapIterator *it) {
    return it->cur->value;
}

bool Int64ToPtrChainedHashMapIteratorNext(
    Int64ToPtrChainedHashMapIterator *it) {
    if (!it->cur) return false;
    if (it->cur->next) {
        it->cur = it->cur->next;
        return true;
    }

    // Move to next non-empty bucket
    ++it->bucket_index;
    it->cur = NULL;
    while (it->bucket_index < it->map->capacity_mask + 1) {
        if (it->map->buckets[it->bucket_index]) {
            it->cur = it->map->buckets[it->bucket_index];
            return true;
        }
        ++it->bucket_index;
    }

    return false;
}
