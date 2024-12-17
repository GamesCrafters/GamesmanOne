/**
 * @file int64_cache.c
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief (UNFINISHED) 64-bit-integer-indexed cache
 * @version 0.0.0
 * @date 2024-07-09
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

#include "core/data_structures/int64_cache.h"

#include <stdbool.h>  // bool
#include <stddef.h>   // NULL, size_t
#include <stdint.h>   // int64_t
#include <stdlib.h>   // calloc, malloc, free

#include "core/misc.h"

static const double kMaxLoadFactor = 0.5;

static void *(*alloc_func)(size_t size);
static void (*free_func)(void *ptr);

typedef struct Entry {
    int64_t key;          /**< Key to the entry. */
    void *data;           /**< Data allocated using \p alloc_func. */
    size_t size;          /**< Size of \p data in bytes. */
    struct Entry *d_prev; /**< Doubly-linked list previous entry. */
    struct Entry *d_next; /**< Doubly-linked list next entry. */
    struct Entry *s_next; /**< Singly-linked list next entry. */
} Entry;

struct Int64Cache {
    Entry **hash_table;   // calloc'ed array of buckets.
    Entry *head;          // calloc'ed head node.
    Entry *tail;          // calloc'ed tail node.
    int64_t num_entries;  // Number of cache entries.
    int64_t num_buckets;  // Number of hash table buckets.
    size_t size;          // Size of the cache in bytes.
};

Int64Cache *Int64CacheInit(size_t size, Int64CacheAllocator *allocator) {
    Int64Cache *cache = (Int64Cache *)calloc(1, sizeof(Int64Cache));
    if (cache == NULL) return NULL;

    cache->head = (Entry *)calloc(1, sizeof(Entry));
    cache->tail = (Entry *)calloc(1, sizeof(Entry));
    if (cache->head == NULL || cache->tail == NULL) {
        free(cache->head);
        free(cache->tail);
        free(cache);
        return NULL;
    }

    cache->head->d_next = cache->tail;
    cache->tail->d_prev = cache->head;
    if (allocator != NULL) {
        alloc_func = allocator->alloc ? allocator->alloc : &malloc;
        free_func = allocator->free ? allocator->free : &free;
    } else {
        alloc_func = &malloc;
        free_func = &free;
    }

    return cache;
}

int Int64CacheDestroy(Int64Cache *cache) {
    if (cache == NULL) return 0;
    free(cache->hash_table);  // free the buckets first.
    Entry *walker = cache->head;
    Entry *free_this;
    while (walker != NULL) {
        free_this = walker;
        walker = walker->d_next;
        free_func(free_this->data);
        free(free_this);
    }
    free(cache);
    return 0;
}

static int64_t Hash(int64_t key, int64_t num_buckets) {
    return ((uint64_t)key) % num_buckets;
}

void *Int64CachePut(Int64Cache *cache, int64_t key, size_t size) {
    // TODO:
    // First verify that the cache size is enough for SIZE. If not
    // enough, return NULL immediately without evicting entries.
    // look for existing key, if exists, free old data and reallocate
    // if not found, allocate a new entry and custom-alloc data of the
    // given size if remaining space is enough. Otherwise, keep evicting
    // entries off the tail of the doubly-linked list until enough room
    // is made.
    return NULL;
}

void *Int64CacheGet(Int64Cache *cache, int64_t key, int64_t *size) {
    int slot = Hash(key, cache->num_buckets);
    Entry *walker = cache->hash_table[slot];
    while (walker != NULL) {
        if (walker->key == key) {
            // Bring this entry to the front of the list.
            walker->d_next->d_prev = walker->d_prev;
            walker->d_prev->d_next = walker->d_next;
            walker->d_prev = cache->head;
            walker->d_next = cache->head->d_next;
            cache->head->d_next = walker;
            walker->d_next->d_prev = walker;

            *size = walker->size;
            return walker->data;
        }
        walker = walker->s_next;
    }

    return NULL;
}
