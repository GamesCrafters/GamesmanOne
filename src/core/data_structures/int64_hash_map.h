/**
 * @file int64_hash_map.h
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Linear-probing (open addressing) int64_t to int64_t hash map.
 * @version 1.0.1
 * @date 2024-09-02
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

#include <stdbool.h>  // bool
#include <stdint.h>   // int64_t

/**
 * @brief Entry object of an \c Int64HashMap. This struct is not meant to be
 * used by the user of this library. Always use accessor and mutator functions
 * instead.
 */
typedef struct Int64HashMapEntry {
    int64_t key;   /**< Key to the entry. */
    int64_t value; /**< Value of the entry. */
    bool used;     /**< True iff this bucket contains an actual record. */
} Int64HashMapEntry;

/**
 * @brief Linear-probing int64_t to int64_t hash map.
 *
 * @example
 * Int64HashMap mymap;
 * Int64HashMapInit(&mymap, 0.5);  // Sets max_load_factor to 0.5.
 * Int64HashMapSet(&mymap, 0, 1);
 * Int64HashMapSet(&mymap, -4, 7);
 * Int64HashMapSet(&mymap, 2, 4);
 * Int64HashMapIterator it = Int64HashMapGet(&mymap, -4);
 * if (Int64HashMapIteratorIsValid(&it)) {
 *     int64_t key = Int64HashMapIteratorKey(&it);
 *     int64_t value = Int64HashMapIteratorValue(&it);
 *
 *     // key: -4, value: 7
 *     printf("key: %" PRId64 ", value: %" PRId64 "\n", key, value);
 * }
 * Int64HashMapDestroy(&mymap);
 */
typedef struct Int64HashMap {
    Int64HashMapEntry *entries; /**< Dynamic array of buckets. */
    int64_t capacity;           /**< Current capacity of the hash map. */
    int64_t size;               /**< Number of entries in the hash map. */
    double max_load_factor;     /**< Hash map will automatically expand if
                                (double)size/capacity is greater than this value. */
} Int64HashMap;

/**
 * @brief Int64HashMap iterator. This is usually returned by an Int64HashMap
 * accessor function.
 *
 * @example
 * Int64HashMapIterator it;
 * Int64HashMapIteratorInit(&it);
 * while (Int64HashMapIteratorNext) {
 *     // Do stuff.
 * }
 */
typedef struct Int64HashMapIterator {
    const Int64HashMap
        *map;      /**< The Int64HashMap this iterator is being used on. */
    int64_t index; /**< Internal index into the Int64HashMap.entries array. */
} Int64HashMapIterator;

/**
 * @brief Initializes the given MAP.
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
void Int64HashMapInit(Int64HashMap *map, double max_load_factor);

/** @brief Deallocates the given MAP. */
void Int64HashMapDestroy(Int64HashMap *map);

/**
 * @brief Returns an iterator to the MAP entry containing the given KEY.
 * Returns an invalid iterator if KEY is not found in MAP. The user should
 * test if the iterator is valid using Int64HashMapIteratorIsValid.
 *
 * @param map Hash map to get the entry from.
 * @param key Key to the desired entry.
 * @return Int64HashMapIterator pointing to the entry with KEY, or invalid
 * if KEY is not found in MAP.
 */
Int64HashMapIterator Int64HashMapGet(const Int64HashMap *map, int64_t key);

/**
 * @brief Sets the entry with KEY in MAP to the given VALUE and returns true.
 * Creates a new entry if KEY does not exist in MAP. If the operation fails
 * for any reason, MAP remains unchanged and the function returns false.
 *
 * @param map Destination hash map.
 * @param key Key of the entry.
 * @param value Value of the entry.
 * @return true on success,
 * @return false otherwise.
 */
bool Int64HashMapSet(Int64HashMap *map, int64_t key, int64_t value);

/**
 * @brief Returns true if the given MAP contains an entry with the given KEY,
 * or false otherwise.
 */
bool Int64HashMapContains(const Int64HashMap *map, int64_t key);

/**
 * @brief Returns an invalid iterator to the entry before the first entry of
 * MAP.
 *
 * @note This function is designed to be used in conjunction with
 * Int64HashMapIteratorNext to iterate through all the entries in the hash map.
 *
 * @example
 * Int64HashMapIterator it = Int64HashMapBegin(map);
 * int64_t key, value;
 *
 * // While the next entry exists...
 * while (Int64HashMapIteratorNext(&it, &key, &value)) {
 *     // Do stuff with key and value...
 * }
 */
Int64HashMapIterator Int64HashMapBegin(Int64HashMap *map);

/**
 * @brief Returns the key of the entry IT is pointing to. The user should
 * validate the iterator using Int64HashMapIteratorIsValid before calling this
 * function. Calling this function on an invalid iterator results in undefined
 * behavior.
 *
 * @param it Int64HashMapIterator.
 * @return Key to the entry.
 */
int64_t Int64HashMapIteratorKey(const Int64HashMapIterator *it);

/**
 * @brief Returns the value of the entry IT is pointing to. The user should
 * validate the iterator using Int64HashMapIteratorIsValid before calling this
 * function. Calling this function on an invalid iterator results in undefined
 * behavior.
 *
 * @param it Int64HashMapIterator.
 * @return Value of the entry.
 */
int64_t Int64HashMapIteratorValue(const Int64HashMapIterator *it);

/** @brief Returns true if the given IT-erator is valid, or false otherwise. */
bool Int64HashMapIteratorIsValid(const Int64HashMapIterator *it);

/**
 * @brief Moves iterator IT to the next valid entry inside the hash map IT was
 * initialized with, sets KEY and VALUE to the key and value of that next entry,
 * and returns true. Sets IT to an invalid state and returns false if no valid
 * next entry exists.
 *
 * @note This function is designed to be used in conjunction with
 * Int64HashMapBegin to iterate through all entries in the hash map. Calling
 * this function on an uninitialized iterator results in undefined behavior.
 *
 * @param iterator Initialized iterator.
 * @param key Pointer to enough space to hold an int64_t object, or NULL if the
 * key to the next entry is not needed.
 * @param value Pointer to enough space to hold an int64_t object, or NULL if
 * the value to the next entry is not needed.
 * @return true if next entry exists,
 * @return false otherwise.
 */
bool Int64HashMapIteratorNext(Int64HashMapIterator *it, int64_t *key,
                              int64_t *value);

#endif  // GAMESMANONE_CORE_DATA_STRUCTURES_INT64_HASH_MAP_H_
