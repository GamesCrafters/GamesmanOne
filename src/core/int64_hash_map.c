#include "int64_hash_map.h"

#include <assert.h>   // assert
#include <malloc.h>   // malloc, free
#include <stdbool.h>  // bool
#include <stddef.h>   // NULL
#include <stdint.h>   // int64_t

static inline int64_t GetBucketIndex(int64_t key, int64_t num_buckets) {
    int ret = key % num_buckets;
    if (ret < 0) return ret + num_buckets;
    return ret;
}

void Int64HashMapEntryListDestroy(Int64HashMapEntryList list) {
    while (list != NULL) {
        Int64HashMapEntry *next = list->next;
        free(list);
        list = next;
    }
}

inline bool Int64HashMapInit(Int64HashMap *map) {
    map->buckets = NULL;
    map->num_buckets = 0;
    map->num_elements = 0;
}

void Int64HashMapDestroy(Int64HashMap *map) {
    if (map->buckets) {
        for (int64_t i = 0; i < map->num_buckets; ++i) {
            Int64HashMapEntryListDestroy(map->buckets[i]);
        }
        free(map->buckets);
        map->buckets = NULL;
        map->num_buckets = 0;
        map->num_elements = 0;
    }
}

// Returns the pointer to the pointer that points to key if key is in map;
// otherwise, returns a pointer to the NULL pointer at the end of the
// corresponding bucket list.
static Int64HashMapEntry **FindPointerTo(Int64HashMap *map,
                                           int64_t key) {
    int64_t bucket_index = GetBucketIndex(key, map->num_buckets);
    Int64HashMapEntry **walker = &(map->buckets[bucket_index]);
    while ((*walker) != NULL && (*walker)->key != key) {
        walker = &((*walker)->next);
    }
    return walker;
}

Int64HashMapEntry *Int64HashMapFind(Int64HashMap *map, int64_t key) {
    return *(FindPointerTo(map, key));
}

// Adds key-value pair only if key is not in map.
void Int64HashMapSet(Int64HashMap *map, int64_t key, int64_t value) {
    if (*(FindPointerTo(map, key)) != NULL) return;
    Int64HashMapEntry *new_entry =
        (Int64HashMapEntry *)malloc(sizeof(Int64HashMapEntry));
    int64_t bucket_index = GetBucketIndex(key, map->num_buckets);
    new_entry->key = key;

    // Insert new entry at the beginning of the list.
    new_entry->next = map->buckets[bucket_index];
    map->buckets[bucket_index] = new_entry;
}

Int64HashMapEntry *Int64HashMapDetach(Int64HashMap *map, int64_t key) {
    Int64HashMapEntry **pointer_to = FindPointerTo(map, key);
    if (!*pointer_to) return NULL;
    Int64HashMapEntry *ret = *pointer_to;
    *pointer_to = (*pointer_to)->next;
    return ret;
}

void Int64HashMapClear(Int64HashMap *map) {
    // There's no difference between clearing and destroying
    // in this version.
    Int64HashMapDestroy(map);
}

inline bool Int64HashMapIteratorInit(Int64HashMapIterator *it,
                                       Int64HashMap *map) {
    it->map = map;
    it->bucket = -1;
    it->current = NULL;
}

Int64HashMapEntry *Int64HashMapIteratorGetNext(Int64HashMapIterator *it) {
    if (it->bucket == it->map->num_buckets) {
        // Reached the end of it->map.
        assert(it->current == NULL);
        return NULL;
    } else if (it->bucket >= 0) {
        // it->current was previously returned as a valid entry in map.
        it->current = it->current->next;
    }
    while (it->current == NULL) {
        if (++it->bucket == it->map->num_buckets) break;
        it->current = it->map->buckets[it->bucket];
    }
    return it->current;
}
