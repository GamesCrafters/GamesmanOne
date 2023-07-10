#ifndef GAMESMANEXPERIMENT_CORE_INT64_HASH_MAP_H_
#define GAMESMANEXPERIMENT_CORE_INT64_HASH_MAP_H_
#include <stdbool.h>
#include <stdint.h>

typedef struct Int64HashMapEntry {
    struct Int64HashMapEntry *next;
    int64_t key;
    int64_t value;
} Int64HashMapEntry;

typedef struct {
    Int64HashMapEntry **buckets;
    int64_t num_buckets;
    int64_t num_elements;
    double max_load_factor;
} Int64HashMap;

typedef struct {
    Int64HashMap *map;
    int64_t bucket;
    Int64HashMapEntry *current;
} Int64HashMapIterator;

typedef Int64HashMapEntry *Int64HashMapEntryList;

void Int64HashMapEntryListDestroy(Int64HashMapEntryList list);

bool Int64HashMapInit(Int64HashMap *map, double max_load_factor);
void Int64HashMapDestroy(Int64HashMap *map);
Int64HashMapEntry *Int64HashMapFind(Int64HashMap *map, int64_t key);
void Int64HashMapSet(Int64HashMap *map, int64_t key, int64_t value);
Int64HashMapEntry *Int64HashMapDetach(Int64HashMap *map, int64_t key);
void Int64HashMapClear(Int64HashMap *map);

bool Int64HashMapIteratorInit(Int64HashMapIterator *it,
                                Int64HashMap *map);
Int64HashMapEntry *Int64HashMapIteratorGetNext(Int64HashMapIterator *it);

#endif  // GAMESMANEXPERIMENT_CORE_INT64_HASH_MAP_H_
