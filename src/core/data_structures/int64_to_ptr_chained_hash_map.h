/**
 * @file int64_to_ptr_chained_hash_map.h
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Separate chaining int64_t to generic pointer (void *) hash map.
 * @version 1.0.0
 * @date 2025-06-09
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

#include <stdbool.h>  // bool
#include <stdint.h>   // int64_t

/**
 * @brief Entry in a \c Int64ToPtrChainedHashMap. This struct is not meant to be
 * used directly. Always use accessor and mutator functions instead.
 */
typedef struct Int64ToPtrChainedHashMapEntry {
    int64_t key; /**< Key to the entry. */
    void *value; /**< Value of the entry. */
    struct Int64ToPtrChainedHashMapEntry
        *next; /**< Next entry in the same bucket. */
} Int64ToPtrChainedHashMapEntry;

/**
 * @brief Separate chaining int64_t to int64_t hash map.
 */
typedef struct Int64ToPtrChainedHashMap {
    Int64ToPtrChainedHashMapEntry **buckets; /**< Dynamic array of buckets. */
    int64_t capacity_mask;                   /**< Number of buckets - 1 */
    int64_t size;           /**< Number of entries in the map. */
    double max_load_factor; /**< Hash map will automatically expand if
                            (double)size/capacity is greater than this value. */
} Int64ToPtrChainedHashMap;

typedef struct Int64ToPtrChainedHashMapIterator {
    const Int64ToPtrChainedHashMap *map;
    int64_t bucket_index;
    Int64ToPtrChainedHashMapEntry *cur;
} Int64ToPtrChainedHashMapIterator;

/**
 * @brief Initializes the given \p map.
 *
 * @param map Map to initialize.
 * @param max_load_factor Set maximum load factor of MAP to this value. The hash
 * map will automatically expand its capacity if (double)size/capacity is
 * greater than the max_load_factor. A small max_load_factor trades memory for
 * speed whereas a large max_load_factor trades speed for memory. This value is
 * restricted to be in the range [0.25, 0.75]. If the user passes a
 * max_load_factor that is smaller than 0.25 or greater than 0.75, the internal
 * value will be set to 0.25 and 0.75, respectively, regardless of the
 * user-specified value.
 */
void Int64ToPtrChainedHashMapInit(Int64ToPtrChainedHashMap *map,
                                  double max_load_factor);

/** @brief Deallocates the given \p map. */
void Int64ToPtrChainedHashMapDestroy(Int64ToPtrChainedHashMap *map);

/**
 * @brief Returns whether \p key is in \p map.
 *
 * @param map Hash map in which the existence of \p key is looked for.
 * @param key Key to look for.
 * @return \c true if \p key exists in \p map, or
 * @return \c false otherwise.
 */
bool Int64ToPtrChainedHashMapContains(const Int64ToPtrChainedHashMap *map,
                                      int64_t key);

/**
 * @brief Returns an iterator to the entry containing the given \p key in \p map
 * . Returns an invalid iterator if \p key is not found in \p map . The iterator
 * returned must be tested by Int64ToPtrChainedHashMapIteratorIsValid for
 * validity before using.
 *
 * @param map Hash map to get the entry from.
 * @param key Key to the desired entry.
 * @return Int64ToPtrChainedHashMapIterator pointing to the entry with \p key ,
 * or an invalid iterator if \p key is not found in \p map .
 */
Int64ToPtrChainedHashMapIterator Int64ToPtrChainedHashMapGet(
    const Int64ToPtrChainedHashMap *map, int64_t key);

/**
 * @brief Sets the entry with \p key in \p map to the given \p value and returns
 * \c true. Creates a new entry if \p key does not exist in \p map. If the
 * operation fails for any reason, \p map remains unchanged and the function
 * returns \c false.
 *
 * @param map Destination hash map.
 * @param key Key of the entry.
 * @param value Value of the entry.
 * @return \c true on success,
 * @return \c false otherwise.
 */
bool Int64ToPtrChainedHashMapSet(Int64ToPtrChainedHashMap *map, int64_t key,
                                 void *value);

/**
 * @brief Removes the entry with \p key in \p map. Does nothing if \p key does
 * not exist.
 *
 * @param map Target hash map.
 * @param key Key to the entry to remove.
 */
void Int64ToPtrChainedHashMapRemove(Int64ToPtrChainedHashMap *map, int64_t key);

/**
 * @brief Returns an iterator to the first entry in \p map . Returns an invalid
 * iterator if \p map is empty.
 *
 * @param map Map the get the first entry from.
 * @return Int64ToPtrChainedHashMapIterator to the first entry in \p map , or
 * an invalid iterator if \p map is empty.
 */
Int64ToPtrChainedHashMapIterator Int64ToPtrChainedHashMapBegin(
    const Int64ToPtrChainedHashMap *map);

/**
 * @brief Returns the key of the entry that \p it is pointing to. The user
 * should validate the iterator using Int64ToPtrChainedHashMapIteratorIsValid
 * before calling this function.
 *
 * @param it Iterator.
 * @return Key to the entry pointed to by \p it.
 */
int64_t Int64ToPtrChainedHashMapIteratorKey(
    const Int64ToPtrChainedHashMapIterator *it);

/**
 * @brief Returns the value of the entry that \p it is pointing to. The user
 * should validate the iterator using Int64ToPtrChainedHashMapIteratorIsValid
 * before calling this function.
 *
 * @param it Iterator.
 * @return Value of the entry pointed to by \p it.
 */
void *Int64ToPtrChainedHashMapIteratorValue(
    const Int64ToPtrChainedHashMapIterator *it);

/** @brief Returns true if the given IT-erator is valid, or false otherwise. */
bool Int64ToPtrChainedHashMapIteratorIsValid(
    const Int64ToPtrChainedHashMapIterator *it);

/**
 * @brief Moves iterator \p it to the next entry inside the map that \p it was
 * initialized with.
 *
 * @param iterator Iterator.
 * @return \c true if the next entry exists,
 * @return \c false otherwise.
 */
bool Int64ToPtrChainedHashMapIteratorNext(Int64ToPtrChainedHashMapIterator *it);

#endif  // GAMESMANONE_CORE_DATA_STRUCTURES_INT64_TO_PTR_CHAINED_HASH_MAP_H_
