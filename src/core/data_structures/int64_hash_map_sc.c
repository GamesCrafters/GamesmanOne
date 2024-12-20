/**
 * @file int64_hash_map_sc.c
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Separate chaining int64_t to int64_t hash map.
 * @details This hash map implementation allows removal of map entries at the
 * cost of being considerably slower than the regular open addressing hash map
 * provided by int64_hash_map.h.
 * @version 1.0.2
 * @date 2024-12-20
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

#include "core/data_structures/int64_hash_map_sc.h"

#include <math.h>     // INFINITY
#include <stdbool.h>  // bool, true, false
#include <stddef.h>   // NULL
#include <stdint.h>   // int64_t
#include <stdlib.h>   // calloc, free
#include <string.h>   // memset

#include "core/misc.h"  // NextPrime

void Int64HashMapSCInit(Int64HashMapSC *map, double max_load_factor) {
    map->buckets = NULL;
    map->num_buckets = 0;
    map->num_entries = 0;
    if (max_load_factor > 0.75) max_load_factor = 0.75;
    if (max_load_factor < 0.25) max_load_factor = 0.25;
    map->max_load_factor = max_load_factor;
}

void Int64HashMapSCDestroy(Int64HashMapSC *map) {
    for (int64_t i = 0; i < map->num_buckets; ++i) {
        Int64HashMapSCEntry *walker = map->buckets[i];
        while (walker) {
            Int64HashMapSCEntry *next = walker->next;
            free(walker);
            walker = next;
        }
    }
    free(map->buckets);
    memset(map, 0, sizeof(*map));
}

static int64_t Hash(int64_t key, int64_t num_buckets) {
    return ((uint64_t)key) % num_buckets;
}

bool Int64HashMapSCContains(const Int64HashMapSC *map, int64_t key) {
    if (map->num_buckets == 0) return false;

    int64_t index = Hash(key, map->num_buckets);
    const Int64HashMapSCEntry *walker = map->buckets[index];
    while (walker != NULL) {
        if (walker->key == key) return true;
        walker = walker->next;
    }

    return false;
}

bool Int64HashMapSCGet(const Int64HashMapSC *map, int64_t key, int64_t *value) {
    if (map->num_buckets == 0) return false;

    int64_t index = Hash(key, map->num_buckets);
    const Int64HashMapSCEntry *walker = map->buckets[index];
    while (walker != NULL) {
        if (walker->key == key) {
            *value = walker->value;
            return true;
        }
        walker = walker->next;
    }

    return false;
}

static bool Expand(Int64HashMapSC *map) {
    int64_t new_num_buckets = NextPrime(map->num_buckets * 2);
    Int64HashMapSCEntry **new_buckets = (Int64HashMapSCEntry **)calloc(
        new_num_buckets, sizeof(Int64HashMapSCEntry *));
    if (new_buckets == NULL) return false;

    for (int64_t i = 0; i < map->num_buckets; ++i) {
        Int64HashMapSCEntry *entry = map->buckets[i];
        while (entry) {
            int64_t new_index = Hash(entry->key, new_num_buckets);
            Int64HashMapSCEntry *next = entry->next;
            entry->next = new_buckets[new_index];
            new_buckets[new_index] = entry;
            entry = next;
        }
    }

    free(map->buckets);
    map->buckets = new_buckets;
    map->num_buckets = new_num_buckets;

    return true;
}

bool Int64HashMapSCSet(Int64HashMapSC *map, int64_t key, int64_t value) {
    // Check if resizing is needed.
    double load_factor =
        (map->num_buckets == 0)
            ? INFINITY
            : (double)(map->num_entries + 1) / (double)map->num_buckets;
    if ((map->num_buckets == 0) || load_factor > map->max_load_factor) {
        if (!Expand(map)) return false;
    }

    // Look for existing key to replace its value.
    int64_t index = Hash(key, map->num_buckets);
    Int64HashMapSCEntry *entry = map->buckets[index];  // Find the bucket.
    while (entry) {
        if (entry->key == key) {  // If key already exists, replace old value.
            entry->value = value;
            return true;
        }
        entry = entry->next;
    }

    // Key does not exist. Create a new entry.
    entry = (Int64HashMapSCEntry *)malloc(sizeof(Int64HashMapSCEntry));
    if (!entry) return false;

    entry->key = key;
    entry->value = value;
    entry->next = map->buckets[index];
    map->buckets[index] = entry;
    ++map->num_entries;

    return true;
}

void Int64HashMapSCRemove(Int64HashMapSC *map, int64_t key) {
    if (map->num_buckets == 0) return;

    int64_t index = Hash(key, map->num_buckets);
    Int64HashMapSCEntry *entry = map->buckets[index];
    Int64HashMapSCEntry **prev_next = &map->buckets[index];

    while (entry) {
        if (entry->key == key) {
            *prev_next = entry->next;
            free(entry);
            --map->num_entries;
            return;
        }
        prev_next = &entry->next;
        entry = entry->next;
    }
}
