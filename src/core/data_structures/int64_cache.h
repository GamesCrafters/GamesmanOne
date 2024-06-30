#ifndef GAMESMANONE_CORE_DATA_STRUCTURES_INT64_HASH_MAP_EXT_H_
#define GAMESMANONE_CORE_DATA_STRUCTURES_INT64_HASH_MAP_EXT_H_

#include <stddef.h>   // size_t
#include <stdbool.h>  // bool
#include <stdint.h>   // int64_t

typedef struct Int64Cache Int64Cache;
typedef struct Int64CacheAllocator {
    void *(*alloc)(size_t size);
    void (*free)(void *ptr);
} Int64CacheAllocator;

Int64Cache *Int64CacheInit(size_t size, Int64CacheAllocator *allocator);
int Int64CacheDestroy(Int64Cache *cache);

void *Int64CachePut(Int64Cache *cache, int64_t key, size_t size);
void *Int64CacheGet(Int64Cache *cache, int64_t key, size_t *size);

#endif  // GAMESMANONE_CORE_DATA_STRUCTURES_INT64_HASH_MAP_EXT_H_
