/**
 * @file int64_hash_set.h
 * @author Robert Shi (robertyishi@berkeley.edu)
 *         GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Linear-probing int64_t hash set.
 * @version 1.0.0
 * @date 2024-11-28
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

#ifndef GAMESMANONE_CORE_DATA_STRUCTURES_INT64_HASH_SET_H_
#define GAMESMANONE_CORE_DATA_STRUCTURES_INT64_HASH_SET_H_

#include <stdbool.h>  // bool
#include <stdint.h>   // int64_t

/**
 * @brief Entry object of an \c Int64HashSet. This struct is not meant to be
 * used by the user of this library. Always use accessor and mutator functions
 * instead.
 */
typedef struct Int64HashSetEntry {
    int64_t key; /**< Key to the entry. */
    bool used;   /**< True iff this bucket contains an actual record. */
} Int64HashSetEntry;

/**
 * @brief Linear-probing int64_t hash set.
 *
 * @example
 * Int64HashSet myset;
 * Int64HashSetInit(&myset, 0.5);  // Sets max_load_factor to 0.5.
 * Int64HashSetAdd(&myset, 42);
 * Int64HashSetAdd(&myset, 43);
 * Int64HashSetAdd(&myset, 55);
 * if (Int64HashSetContains(&myset, 42)) {  // returns true
 *     printf("myset contains 42\n");
 * }
 * if (!Int64HashSetContains(&myset, 0)) {  // returns false
 *     printf("myset does not contain 0\n");
 * }
 * Int64HashSetDestroy(&myset);
 */
typedef struct Int64HashSet {
    Int64HashSetEntry *entries; /**< Dynamic array of buckets. */
    int64_t capacity;           /**< Current capacity of the hash set. */
    int64_t size;               /**< Number of entries in the hash set. */
    double max_load_factor;     /**< Hash set will automatically expand if
                                (double)size/capacity is greater than this value. */
} Int64HashSet;

/**
 * @brief Initializes the given \p set to an empty set with maximum load
 * factor \p max_load_factor.
 *
 * @param set Set to initialize.
 * @param max_load_factor Set maximum load factor of \p set to this value. The
 * hash set will automatically expand its capacity if (double)size/capacity is
 * greater than \p max_load_factor. A small value trades memory for speed
 * whereas a large value trades speed for memory. This value is restricted to be
 * in the range [0.25, 0.75] to provide optimal performance. The actual max load
 * factor is capped at 0.25 and 0.75 respectively if the user passes a value
 * that is smaller than 0.25 or greater than 0.75.
 */
void Int64HashSetInit(Int64HashSet *set, double max_load_factor);

/** @brief Deallocates the given \p set. */
void Int64HashSetDestroy(Int64HashSet *set);

/**
 * @brief Adds \p key to \p set or does nothing if \p set already contains \p
 * key.
 *
 * @param set Set to add \p key to.
 * @param key Key to add to \p set.
 * @return true on success,
 * @return false otherwise.
 */
bool Int64HashSetAdd(Int64HashSet *set, int64_t key);

/**
 * @brief Tests if \p key is in \p set.
 *
 * @param set Set from which the given \p key is looked up.
 * @param key Key to look for.
 * @return true if \p set contains \p key, or
 * @return false otherwise.
 */
bool Int64HashSetContains(const Int64HashSet *set, int64_t key);

#endif  // GAMESMANONE_CORE_DATA_STRUCTURES_INT64_HASH_SET_H_
