/**
 * @file int64_hash_map.h
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Dynamically-sized linear probing `int64_t` to `int64_t` hash map with
 * sentinel value optimization.
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

#ifndef GAMESMANONE_CORE_DATA_STRUCTURES_INT64_HASH_MAP_H_
#define GAMESMANONE_CORE_DATA_STRUCTURES_INT64_HASH_MAP_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "core/data_structures/hash.h"
#include "core/gamesman_memory.h"

/**
 * @brief Sentinel value used to represent an empty slot in the hash map.
 *
 * @details The user of `Int64HashMap` is responsible for making sure that
 * `INT64_MIN` is never inserted as a key. The library is aggressively
 * optimized and will not check for such insertions.
 */
#define INT64_HASH_MAP_EMPTY_KEY INT64_MIN

/**
 * @brief Dynamically-sized linear probing `int64_t` to `int64_t` hash map.
 *
 * @details Uses a parallel arrays layout where keys and values are stored
 * separate parallel `int64_t` arrays. This design keeps the probing loop
 * operating on keys only, maximizing cache line utilization during lookups.
 *
 * @note `INT64_MIN` (`INT64_HASH_MAP_EMPTY_KEY`) is reserved as a sentinel
 * value for the hash table to represent an empty slot for optimization
 * purposes. Because of this, `Int64HashMap` cannot be used to store
 * `INT64_MIN` as a key. The user must make sure that `INT64_MIN` is never
 * inserted as a key.
 *
 * @warning This struct should be treated as opaque. Use accessor and mutator
 * functions to interact with the hash map.
 */
typedef struct Int64HashMap {
    /** Key array. */
    int64_t *keys;

    /** Parallel value array, indexed identically to `keys`. */
    int64_t *values;

    /** Bitmask used for indexing into the keys. */
    uint64_t mask;

    /** Current number of key-value pairs in the map. */
    int64_t size;

    /** Maximum number of keys before expansion is needed. */
    int64_t max_size;

    /** Equals to `1.0 / max_load_factor`. */
    double inv_max_load_factor;

    /** Custom allocator, or `NULL` for default allocation. */
    GamesmanAllocator *allocator;
} Int64HashMap;

/**
 * @brief Iterator for `Int64HashMap`.
 *
 * @warning This struct should be treated as opaque. Use accessor and mutator
 * functions to interact with the iterator.
 */
typedef struct Int64HashMapIterator {
    /** The `Int64HashMap` this iterator is being used on. */
    const Int64HashMap *map;

    /** Internal index into the hash table arrays. */
    int64_t index;
} Int64HashMapIterator;

// ================================= Lifecycle =================================

/**
 * @brief Initializes the given `map` using the given memory `allocator`.
 *
 * @details No memory is allocated until the first insertion or reservation.
 * The `max_load_factor` is clamped to [0.5, 0.8]. If `allocator` is `NULL`,
 * the effect is equivalent to calling `Int64HashMapInit`.
 *
 * @param[out] map The `Int64HashMap` to initialize.
 * @param[in] max_load_factor The maximum load factor, clamped to [0.5, 0.8].
 * @param[in] allocator Memory allocator to use, or `NULL` for default.
 */
static inline void Int64HashMapInitAllocator(Int64HashMap *map,
                                             double max_load_factor,
                                             GamesmanAllocator *allocator) {
    // Clamp max_load_factor to [0.5, 0.8]
    if (max_load_factor < 0.5) {
        max_load_factor = 0.5;
    } else if (max_load_factor > 0.8) {
        max_load_factor = 0.8;
    }

    map->keys = NULL;
    map->values = NULL;
    map->mask = 0x0ULL;
    map->size = 0L;
    map->max_size = 0L;
    map->inv_max_load_factor = 1.0 / max_load_factor;
    map->allocator = GamesmanAllocatorAddRef(allocator);
}

/**
 * @brief Initializes the given `map`.
 *
 * @details No memory is allocated until the first insertion or reservation.
 * The `max_load_factor` is clamped to [0.5, 0.8].
 *
 * @param[out] map The `Int64HashMap` to initialize.
 * @param[in] max_load_factor The maximum load factor, clamped to [0.5, 0.8].
 */
static inline void Int64HashMapInit(Int64HashMap *map, double max_load_factor) {
    Int64HashMapInitAllocator(map, max_load_factor, NULL);
}

/**
 * @brief Frees the memory associated with the hash map and resets its state.
 *
 * @param[in,out] map The `Int64HashMap` to destroy.
 */
static inline void Int64HashMapDestroy(Int64HashMap *map) {
    GamesmanAllocatorDeallocate(map->allocator, map->keys);
    GamesmanAllocatorDeallocate(map->allocator, map->values);
    GamesmanAllocatorRelease(map->allocator);
    map->keys = NULL;
    map->values = NULL;
    map->mask = 0x0ULL;
    map->size = 0L;
    map->max_size = 0L;
    map->inv_max_load_factor = 0.0;
    map->allocator = NULL;
}

// ================================== Getters ==================================

/**
 * @brief Returns the number of key-value pairs stored in `map`.
 *
 * @param[in] map The `Int64HashMap` to query.
 *
 * @return The number of key-value pairs currently in the map.
 */
static inline int64_t Int64HashMapSize(const Int64HashMap *map) {
    return map->size;
}

// =========================== Insertion and Lookup ===========================

/**
 * @brief Sets the value associated with `key` in `map` to `value`.
 *
 * @details Creates a new entry if `key` does not exist in `map`. If `key`
 * already exists, its value is updated to `value`. If the operation fails for
 * any reason, `map` remains unchanged.
 *
 * @param[in,out] map The `Int64HashMap` to modify.
 * @param[in] key The key of the entry. Must not be equal to `INT64_MIN`
 * (`INT64_HASH_MAP_EMPTY_KEY`).
 * @param[in] value The value to associate with `key`.
 *
 * @retval true The entry was successfully set.
 * @retval false Memory allocation failed during expansion.
 */
static inline bool Int64HashMapSet(Int64HashMap *map, int64_t key,
                                   int64_t value) {
    if (map->size >= map->max_size) {
        // Declared in int64_hash_map.c; handles parallel values rehashing.
        bool Int64HashMapInternalExpand(Int64HashMap * map);
        if (!Int64HashMapInternalExpand(map)) {
            return false;
        }
    }

    // Hoist pointers and values to locals so the compiler
    // doesn't worry about memory aliasing during the loop.
    int64_t *__restrict keys = map->keys;
    int64_t *__restrict values = map->values;
    uint64_t mask = map->mask;

    uint64_t index = Splitmix64(key) & mask;
    while (keys[index] != INT64_HASH_MAP_EMPTY_KEY) {
        if (keys[index] == key) {
            values[index] = value;
            return true;
        }
        index = (index + 1) & mask;
    }
    keys[index] = key;
    values[index] = value;
    ++map->size;

    return true;
}

/**
 * @brief Returns an iterator pointing to the entry with the given `key` in
 * `map`, or an invalid iterator if `key` is not found.
 *
 * @details Use `Int64HashMapIteratorIsValid` to check whether `key` was found.
 *
 * @param[in] map The `Int64HashMap` to search.
 * @param[in] key The key to search for.
 *
 * @return An `Int64HashMapIterator` pointing to the entry with `key`, or an
 * invalid iterator if `key` is not found.
 */
static inline Int64HashMapIterator Int64HashMapGet(const Int64HashMap *map,
                                                   int64_t key) {
    Int64HashMapIterator result;
    result.map = map;

    const int64_t *__restrict keys = map->keys;

    // Return invalid iterator if map has not been lazily initialized.
    if (!keys) {
        result.index = -1;
        return result;
    }

    const uint64_t mask = map->mask;
    uint64_t index = Splitmix64(key) & mask;
    while (keys[index] != INT64_HASH_MAP_EMPTY_KEY) {
        if (keys[index] == key) {
            result.index = (int64_t)index;
            return result;
        }
        index = (index + 1) & mask;
    }

    result.index = -1;
    return result;
}

/**
 * @brief Returns whether `map` contains an entry with the given `key`.
 *
 * @param[in] map The `Int64HashMap` to search.
 * @param[in] key The key to search for.
 *
 * @retval true The `key` is present in the map.
 * @retval false The `key` is not present, or the map is not initialized.
 */
static inline bool Int64HashMapContains(const Int64HashMap *map, int64_t key) {
    const int64_t *__restrict keys = map->keys;

    if (!keys) {
        return false;
    }

    const uint64_t mask = map->mask;
    uint64_t index = Splitmix64(key) & mask;
    while (keys[index] != INT64_HASH_MAP_EMPTY_KEY) {
        if (keys[index] == key) {
            return true;
        }
        index = (index + 1) & mask;
    }

    return false;
}

// ================================= Iteration =================================

/**
 * @brief Returns an iterator positioned before the first entry of `map`.
 *
 * @details This function is designed to be used with `Int64HashMapIteratorNext`
 * to iterate through all entries in the hash map.
 *
 * @param[in] map The `Int64HashMap` to iterate over.
 *
 * @return An `Int64HashMapIterator` positioned before the first entry.
 */
static inline Int64HashMapIterator Int64HashMapBegin(const Int64HashMap *map) {
    Int64HashMapIterator result;
    result.map = map;
    result.index = -1;
    return result;
}

/**
 * @brief Returns the key of the entry that `it` is pointing to.
 *
 * @warning Calling this function on an invalid iterator results in undefined
 * behavior. Use `Int64HashMapIteratorIsValid` to validate the iterator first.
 *
 * @param[in] it The `Int64HashMapIterator` to read.
 *
 * @return The key of the entry.
 */
static inline int64_t Int64HashMapIteratorKey(const Int64HashMapIterator *it) {
    return it->map->keys[it->index];
}

/**
 * @brief Returns the value of the entry that `it` is pointing to.
 *
 * @warning Calling this function on an invalid iterator results in undefined
 * behavior. Use `Int64HashMapIteratorIsValid` to validate the iterator first.
 *
 * @param[in] it The `Int64HashMapIterator` to read.
 *
 * @return The value of the entry.
 */
static inline int64_t Int64HashMapIteratorValue(
    const Int64HashMapIterator *it) {
    return it->map->values[it->index];
}

/**
 * @brief Returns whether the given iterator `it` is valid.
 *
 * @param[in] it The `Int64HashMapIterator` to validate.
 *
 * @retval true The iterator points to a valid entry.
 * @retval false The iterator is invalid.
 */
static inline bool Int64HashMapIteratorIsValid(const Int64HashMapIterator *it) {
    return it->index >= 0 && it->index <= (int64_t)it->map->mask;
}

/**
 * @brief Advances `it` to the next valid entry and returns `true`, or returns
 * `false` if no more entries exist.
 *
 * @details This function is designed to be used with `Int64HashMapBegin` to
 * iterate through all entries in the hash map.
 *
 * @param[in,out] it The `Int64HashMapIterator` to advance.
 * @param[out] key Pointer to receive the key, or `NULL` if not needed.
 * @param[out] value Pointer to receive the value, or `NULL` if not needed.
 *
 * @retval true The iterator was advanced to a valid entry.
 * @retval false No more entries exist.
 */
static inline bool Int64HashMapIteratorNext(Int64HashMapIterator *it,
                                            int64_t *key, int64_t *value) {
    const Int64HashMap *map = it->map;
    const int64_t *__restrict keys = map->keys;

    // Return false if map has not been lazily initialized.
    if (!keys) {
        return false;
    }

    const int64_t capacity = (int64_t)(map->mask + 1);

    while (++it->index < capacity) {
        if (keys[it->index] != INT64_HASH_MAP_EMPTY_KEY) {
            if (key) {
                *key = keys[it->index];
            }
            if (value) {
                *value = map->values[it->index];
            }
            return true;
        }
    }

    return false;
}

#endif  // GAMESMANONE_CORE_DATA_STRUCTURES_INT64_HASH_MAP_H_
