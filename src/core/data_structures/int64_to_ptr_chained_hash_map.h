/**
 * @file int64_to_ptr_chained_hash_map.h
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @author GamesCrafters Research Group, UC Berkeley
 * @author Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Separate chaining int64_t to generic pointer (void *) hash map.
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

#ifndef GAMESMANONE_CORE_DATA_STRUCTURES_INT64_TO_PTR_CHAINED_HASH_MAP_H_
#define GAMESMANONE_CORE_DATA_STRUCTURES_INT64_TO_PTR_CHAINED_HASH_MAP_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/**
 * @brief Entry in a `Int64ToPtrChainedHashMap`. This struct is not meant to be
 * used directly. Always use accessor and mutator functions instead.
 */
typedef struct Int64ToPtrChainedHashMapEntry {
    /** Key to the entry. */
    int64_t key;

    /** Value of the entry. */
    void *value;

    /** Next entry in the same bucket. */
    struct Int64ToPtrChainedHashMapEntry *next;
} Int64ToPtrChainedHashMapEntry;

/**
 * @brief Separate chaining int64_t to generic pointer (void *) hash map.
 */
typedef struct Int64ToPtrChainedHashMap {
    /** Dynamic array of buckets. */
    Int64ToPtrChainedHashMapEntry **buckets;

    /** Number of buckets - 1; 0 if unallocated. */
    uint64_t capacity_mask;

    /** Number of entries in the map. */
    int64_t size;

    /**
     * Hash map will automatically expand if (double)size/capacity is greater
     * than this value.
     */
    double max_load_factor;
} Int64ToPtrChainedHashMap;

/**
 * @brief `Int64ToPtrChainedHashMap` iterator, which is usually returned by an
 * `Int64ToPtrChainedHashMap` accessor function. No internal states are
 * intended to be inspected or manipulated directly. Use the provided API
 * functions instead.
 */
typedef struct Int64ToPtrChainedHashMapIterator {
    /** The map this iterator is associated with. */
    const Int64ToPtrChainedHashMap *map;

    /** Index to the internal bucket array. */
    int64_t bucket_index;

    /** Pointer to the current entry. */
    Int64ToPtrChainedHashMapEntry *cur;
} Int64ToPtrChainedHashMapIterator;

/**
 * @brief Initializes the given `map`.
 *
 * @param[out] map Map to initialize.
 * @param[in] max_load_factor Set maximum load factor of `map` to this value.
 * The hash map will automatically expand its capacity if
 * (double)size/capacity is greater than the max_load_factor. A small
 * max_load_factor trades memory for speed whereas a large max_load_factor
 * trades speed for memory. This value is clamped to the range [0.5, 2.0]. If
 * the user passes a max_load_factor that is smaller than 0.5 or greater than
 * 2.0, the internal value will be set to 0.5 and 2.0, respectively, regardless
 * of the user-specified value.
 */
static inline void Int64ToPtrChainedHashMapInit(Int64ToPtrChainedHashMap *map,
                                                double max_load_factor) {
    map->buckets = NULL;
    map->capacity_mask = 0ULL;
    map->size = 0;

    if (max_load_factor > 2.0) {
        max_load_factor = 2.0;
    }
    if (max_load_factor < 0.5) {
        max_load_factor = 0.5;
    }
    map->max_load_factor = max_load_factor;
}

/**
 * @brief Deallocates the internal memory used by the given `map`.
 *
 * @param[in,out] map Target hash map.
 */
void Int64ToPtrChainedHashMapDestroy(Int64ToPtrChainedHashMap *map);

/**
 * @brief Returns an iterator to the entry containing the given `key` in
 * `map`. Returns an invalid iterator if `key` is not found in `map`. The
 * iterator returned must be tested by
 * `Int64ToPtrChainedHashMapIteratorIsValid` for validity before using.
 *
 * @param[in] map Hash map to get the entry from.
 * @param[in] key Key to the desired entry.
 *
 * @return `Int64ToPtrChainedHashMapIterator` pointing to the entry with
 * `key`, or an invalid iterator if `key` is not found in `map`.
 */
Int64ToPtrChainedHashMapIterator Int64ToPtrChainedHashMapGet(
    const Int64ToPtrChainedHashMap *map, int64_t key);

/**
 * @brief Sets the entry with `key` in `map` to the given `value`. Creates a
 * new entry if `key` does not exist in `map`. If memory allocation fails for
 * the new entry, the function returns `false`. Note that the map may have
 * already been successfully expanded and rehashed prior to this failure.
 *
 * @param[in,out] map Destination hash map.
 * @param[in] key Key of the entry.
 * @param[in] value Value of the entry.
 *
 * @retval true on success.
 * @retval false otherwise.
 */
bool Int64ToPtrChainedHashMapSet(Int64ToPtrChainedHashMap *map, int64_t key,
                                 void *value);

/**
 * @brief Removes the entry with `key` in `map`. Does nothing if `key` does
 * not exist.
 *
 * @param[in,out] map Target hash map.
 * @param[in] key Key to the entry to remove.
 */
void Int64ToPtrChainedHashMapRemove(Int64ToPtrChainedHashMap *map, int64_t key);

/**
 * @brief Returns an iterator to the first entry in `map`. Returns an invalid
 * iterator if `map` is empty.
 *
 * @param[in] map Map to get the first entry from.
 *
 * @return `Int64ToPtrChainedHashMapIterator` to the first entry in `map`, or
 * an invalid iterator if `map` is empty.
 */
Int64ToPtrChainedHashMapIterator Int64ToPtrChainedHashMapBegin(
    const Int64ToPtrChainedHashMap *map);

/**
 * @brief Returns the key of the entry that `it` is pointing to. The user
 * should validate the iterator using `Int64ToPtrChainedHashMapIteratorIsValid`
 * before calling this function.
 *
 * @param[in] it Iterator.
 *
 * @return Key to the entry pointed to by `it`.
 */
static inline int64_t Int64ToPtrChainedHashMapIteratorKey(
    const Int64ToPtrChainedHashMapIterator *it) {
    return it->cur->key;
}

/**
 * @brief Returns the value of the entry that `it` is pointing to. The user
 * should validate the iterator using `Int64ToPtrChainedHashMapIteratorIsValid`
 * before calling this function.
 *
 * @param[in] it Iterator.
 *
 * @return Value of the entry pointed to by `it`.
 */
static inline void *Int64ToPtrChainedHashMapIteratorValue(
    const Int64ToPtrChainedHashMapIterator *it) {
    return it->cur->value;
}

/**
 * @brief Returns `true` if `it` is a valid iterator, or `false` otherwise.
 *
 * @param[in] it Iterator to validate.
 *
 * @retval true if the iterator is valid.
 * @retval false if the iterator is invalid.
 */
static inline bool Int64ToPtrChainedHashMapIteratorIsValid(
    const Int64ToPtrChainedHashMapIterator *it) {
    return it->cur != NULL;
}

/**
 * @brief Advances iterator `it` to the next valid entry in the hash map.
 *
 * @param[in,out] it Non-NULL pointer to the iterator.
 *
 * @retval true if the next entry exists.
 * @retval false otherwise.
 */
bool Int64ToPtrChainedHashMapIteratorNext(Int64ToPtrChainedHashMapIterator *it);

#endif  // GAMESMANONE_CORE_DATA_STRUCTURES_INT64_TO_PTR_CHAINED_HASH_MAP_H_
