#ifndef GAMESMANEXPERIMENT_CORE_DATA_STRUCTURES_INT64_HASH_MAP_H_
#define GAMESMANEXPERIMENT_CORE_DATA_STRUCTURES_INT64_HASH_MAP_H_

#include <stdbool.h>  // bool
#include <stdint.h>   // int64_t

typedef struct Int64HashMapEntry {
    int64_t key;
    int64_t value;
    bool used;
} Int64HashMapEntry;

typedef struct {
    Int64HashMapEntry *entries;
    int64_t capacity;
    int64_t size;
    double max_load_factor;
} Int64HashMap;

/**
 * @note
 * Example usage:
 *   Int64HashMapIterator it;
 *   Int64HashMapIteratorInit(&it);
 *    while (Int64HashMapIteratorNext) {
 *        // Do stuff.
 *    }
 */
typedef struct {
    Int64HashMap *map;
    int64_t index;
} Int64HashMapIterator;

void Int64HashMapInit(Int64HashMap *map, double max_load_factor);
void Int64HashMapDestroy(Int64HashMap *map);
Int64HashMapIterator Int64HashMapGet(Int64HashMap *map, int64_t key);
bool Int64HashMapSet(Int64HashMap *map, int64_t key, int64_t value);
bool Int64HashMapContains(Int64HashMap *map, int64_t key);

void Int64HashMapIteratorInit(Int64HashMapIterator *it, Int64HashMap *map);
int64_t Int64HashMapIteratorKey(const Int64HashMapIterator *it);
int64_t Int64HashMapIteratorValue(const Int64HashMapIterator *it);
bool Int64HashMapIteratorIsValid(const Int64HashMapIterator *it);
bool Int64HashMapIteratorNext(Int64HashMapIterator *iterator, int64_t *key,
                              int64_t *value);

#endif  // GAMESMANEXPERIMENT_CORE_DATA_STRUCTURES_INT64_HASH_MAP_H_
