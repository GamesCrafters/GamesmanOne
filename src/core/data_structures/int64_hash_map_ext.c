#include "core/data_structures/int64_hash_map_ext.h"

#include <math.h>     // INFINITY
#include <stdbool.h>  // bool, true, false
#include <stddef.h>   // NULL
#include <stdint.h>   // int64_t
#include <stdlib.h>   // calloc, free
#include <string.h>   // memset

#include "core/misc.h"  // NextPrime

void Int64HashMapExtInit(Int64HashMapExt *map, double max_load_factor) {
    map->buckets = NULL;
    map->num_buckets = 0;
    map->num_entries = 0;
    if (max_load_factor > 0.75) max_load_factor = 0.75;
    if (max_load_factor < 0.25) max_load_factor = 0.25;
    map->max_load_factor = max_load_factor;
}

void Int64HashMapExtDestroy(Int64HashMapExt *map) {
    for (int64_t i = 0; i < map->num_buckets; ++i) {
        Int64HashMapExtEntry *walker = map->buckets[i];
        while (walker) {
            Int64HashMapExtEntry *next = walker->next;
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

bool Int64HashMapExtContains(const Int64HashMapExt *map, int64_t key) {
    if (map->num_buckets == 0) return false;

    int64_t index = Hash(key, map->num_buckets);
    const Int64HashMapExtEntry *walker = map->buckets[index];
    while (walker != NULL) {
        if (walker->key == key) return true;
        walker = walker->next;
    }

    return false;
}

bool Int64HashMapExtGet(const Int64HashMapExt *map, int64_t key,
                        int64_t *value) {
    if (map->num_buckets == 0) return false;

    int64_t index = Hash(key, map->num_buckets);
    const Int64HashMapExtEntry *walker = map->buckets[index];
    while (walker != NULL) {
        if (walker->key == key) {
            *value = walker->value;
            return true;
        }
        walker = walker->next;
    }

    return false;
}

static bool Expand(Int64HashMapExt *map) {
    int64_t new_num_buckets = NextPrime(map->num_buckets * 2);
    Int64HashMapExtEntry **new_buckets = (Int64HashMapExtEntry **)calloc(
        new_num_buckets, sizeof(Int64HashMapExtEntry *));
    if (new_buckets == NULL) return false;

    for (int64_t i = 0; i < map->num_buckets; ++i) {
        Int64HashMapExtEntry *entry = map->buckets[i];
        while (entry) {
            int64_t new_index = Hash(entry->key, new_num_buckets);
            Int64HashMapExtEntry *next = entry->next;
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

bool Int64HashMapExtSet(Int64HashMapExt *map, int64_t key, int64_t value) {
    // Check if resizing is needed.
    double load_factor;
    if (map->num_buckets == 0) {
        load_factor = INFINITY;
    } else {
        load_factor = (double)(map->num_entries + 1) / (double)map->num_buckets;
    }
    if (load_factor > map->max_load_factor) {
        if (!Expand(map)) return false;
    }

    // Look for existing key to replace its value.
    int64_t index = Hash(key, map->num_buckets);
    Int64HashMapExtEntry *entry = map->buckets[index];  // Find the bucket.
    while (entry) {
        if (entry->key == key) {  // If key already exists, replace old value.
            entry->value = value;
            return true;
        }
        entry = entry->next;
    }

    // Key does not exist. Create a new entry.
    entry = malloc(sizeof(Int64HashMapExtEntry));
    if (!entry) return false;

    entry->key = key;
    entry->value = value;
    entry->next = map->buckets[index];
    map->buckets[index] = entry;
    ++map->num_entries;

    return true;
}

void Int64HashMapExtRemove(Int64HashMapExt *map, int64_t key) {
    int64_t index = Hash(key, map->num_buckets);
    Int64HashMapExtEntry *entry = map->buckets[index];
    Int64HashMapExtEntry **prev_next = &map->buckets[index];

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
