#ifndef GAMESMANONE_CORE_DATA_STRUCTURES_INT64_HASH_MAP_EXT_H_
#define GAMESMANONE_CORE_DATA_STRUCTURES_INT64_HASH_MAP_EXT_H_

#include <stdint.h>   // int64_t
#include <stdbool.h>  // bool

typedef struct Int64HashMapExtEntry {
    int64_t key;
    int64_t value;
    struct Int64HashMapExtEntry *next;
} Int64HashMapExtEntry;

typedef struct Int64HashMapExt {
    Int64HashMapExtEntry **buckets;
    int64_t num_buckets;
    int64_t num_entries;
    double max_load_factor;
} Int64HashMapExt;

void Int64HashMapExtInit(Int64HashMapExt *map, double max_load_factor);
void Int64HashMapExtDestroy(Int64HashMapExt *map);

bool Int64HashMapExtContains(const Int64HashMapExt *map, int64_t key);
bool Int64HashMapExtGet(const Int64HashMapExt *map, int64_t key, int64_t *value);
bool Int64HashMapExtSet(Int64HashMapExt *map, int64_t key, int64_t value);
void Int64HashMapExtRemove(Int64HashMapExt *map, int64_t key);

#endif  // GAMESMANONE_CORE_DATA_STRUCTURES_INT64_HASH_MAP_EXT_H_
