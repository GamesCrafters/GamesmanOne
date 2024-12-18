/**
 * @file int64_cache.h
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

#ifndef GAMESMANONE_CORE_DATA_STRUCTURES_INT64_CACHE_H_
#define GAMESMANONE_CORE_DATA_STRUCTURES_INT64_CACHE_H_

#include <stdbool.h>  // bool
#include <stddef.h>   // size_t
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

#endif  // GAMESMANONE_CORE_DATA_STRUCTURES_INT64_CACHE_H_
