/**
 * @file int64_hash_map_sc.h
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Separate chaining int64_t to int64_t hash map.
 * @details This hash map implementation allows removal of map entries at the
 * cost of being considerably slower than the regular open addressing hash map
 * provided by int64_hash_map.h.
 * @version 1.0.2
 * @date 2024-12-20
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

#ifndef GAMESMANONE_CORE_DATA_STRUCTURES_INT64_HASH_MAP_SC_H_
#define GAMESMANONE_CORE_DATA_STRUCTURES_INT64_HASH_MAP_SC_H_

#include <stdbool.h>  // bool
#include <stdint.h>   // int64_t

/**
 * @brief Entry object of an \c Int64HashMapSC. This struct is not meant to be
 * used by the user of this library. Always use accessor and mutator functions
 * instead.
 */
typedef struct Int64HashMapSCEntry {
    int64_t key;                      /**< Key to the entry. */
    int64_t value;                    /**< Value of the entry. */
    struct Int64HashMapSCEntry *next; /**< Next entry in the same bucket. */
} Int64HashMapSCEntry;

/**
 * @brief Separate chaining int64_t to int64_t hash map.
 */
typedef struct Int64HashMapSC {
    Int64HashMapSCEntry **buckets; /**< Dynamic array of buckets. */
    int64_t num_buckets;           /**< Number of buckets. */
    int64_t num_entries;           /**< Number of entries in map. */
    double max_load_factor;        /**< Hash map will automatically expand if
                                   (double)size/capacity is greater than this value. */
} Int64HashMapSC;

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
void Int64HashMapSCInit(Int64HashMapSC *map, double max_load_factor);

/** @brief Deallocates the given \p map. */
void Int64HashMapSCDestroy(Int64HashMapSC *map);

/**
 * @brief Returns whether \p key is in \p map.
 *
 * @param map Hash map in which the existence of \p key is looked for.
 * @param key Key to look for.
 * @return \c true if \p key exists in \p map, or
 * @return \c false otherwise.
 */
bool Int64HashMapSCContains(const Int64HashMapSC *map, int64_t key);

/**
 * @brief Sets \p value to the value of the entry containing the given \p key in
 * \p map and returns \c true if \p key exists in \p map. Otherwise, \p value is
 * unmodified and \c false is returned.
 *
 * @param map Hash map to get the entry from.
 * @param key Key to the desired entry.
 * @param value Pointer to preallocated space for value output if \p key exists.
 * Unmodified if \p key does not exist in \p map.
 * @return \c true on success, or
 * @return \c false on failure or if \p key does not exist in \p map.
 */
bool Int64HashMapSCGet(const Int64HashMapSC *map, int64_t key, int64_t *value);

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
bool Int64HashMapSCSet(Int64HashMapSC *map, int64_t key, int64_t value);

/**
 * @brief Removes the entry with \p key in \p map. Does nothing if \p key does
 * not exist.
 *
 * @param map Target hash map.
 * @param key Key to the entry to remove.
 */
void Int64HashMapSCRemove(Int64HashMapSC *map, int64_t key);

#endif  // GAMESMANONE_CORE_DATA_STRUCTURES_INT64_HASH_MAP_SC_H_
