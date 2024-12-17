/**
 * @file tier_hash_map.h
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Linear-probing Tier hash map that maps Tiers to 64-bit signed
 * integers.
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

#ifndef GAMESMANONE_CORE_TYPES_TIER_HASH_MAP_H_
#define GAMESMANONE_CORE_TYPES_TIER_HASH_MAP_H_

#include <stdbool.h>  // bool
#include <stdint.h>   // int64_t

#include "core/data_structures/int64_hash_map.h"
#include "core/types/base.h"

/** @brief Linear-probing Tier to int64_t hash map using Int64HashMap. */
typedef Int64HashMap TierHashMap;

/** @brief Iterator for TierHashMap. */
typedef Int64HashMapIterator TierHashMapIterator;

/**
 * @brief Initializes Tier hash map MAP to an empty map with maximum load
 * factor MAX_LOAD_FACTOR.
 *
 * @param map Map to initialize.
 * @param max_load_factor Set maximum load factor of MAP to this value. The hash
 * map will automatically expand its capacity if (double)size/capacity is
 * greater than this value. A small max_load_factor trades memory for
 * speed whereas a large max_load_factor trades speed for memory. This value is
 * restricted to be in the range [0.25, 0.75]. If the user passes a
 * max_load_factor that is smaller than 0.25 or greater than 0.75, the internal
 * value will be set to 0.25 and 0.75, respectively, regardless of the
 * user-specified value.
 */
void TierHashMapInit(TierHashMap *map, double max_load_factor);

/** @brief Destroys the Tier hash map MAP. */
void TierHashMapDestroy(TierHashMap *map);

/**
 * @brief Returns an iterator to the MAP entry containing the given KEY.
 * Returns an invalid iterator if KEY is not found in MAP. The user should
 * test if the iterator is valid using TierHashMapIteratorIsValid.
 *
 * @param map Tier hash map to get the entry from.
 * @param key Tier value as key to the desired entry.
 * @return TierHashMapIterator pointing to the entry with KEY, or invalid
 * if KEY is not found in MAP.
 */
TierHashMapIterator TierHashMapGet(TierHashMap *map, Tier key);

/**
 * @brief Sets the entry with key equal to TIER in MAP to the given VALUE and
 * returns true. Creates a new entry if TIER does not exist in MAP. If the
 * operation fails for any reason, MAP remains unchanged and the function
 * returns false.
 *
 * @param map Destination hash map.
 * @param tier Tier as the key to the entry.
 * @param value Value of the entry.
 * @return true on success,
 * @return false otherwise.
 */
bool TierHashMapSet(TierHashMap *map, Tier tier, int64_t value);

/**
 * @brief Returns true if the given MAP contains an entry with key equal to
 * TIER, or false otherwise.
 */
bool TierHashMapContains(const TierHashMap *map, Tier tier);

/**
 * @brief Returns an invalid iterator to the entry before the first entry of
 * MAP.
 *
 * @note This function is designed to be used in conjunction with
 * TierHashMapIteratorNext to iterate through all the entries in the Tier hash
 * map.
 *
 * @example
 * TierHashMapIterator it = TierHashMapBegin(map);
 * Tier key;
 * int64_t value;
 *
 * // While the next entry exists...
 * while (TierHashMapIteratorNext(&it, &key, &value)) {
 *     // Do stuff with key and value...
 * }
 */
TierHashMapIterator TierHashMapBegin(TierHashMap *map);

/**
 * @brief Returns the Tier that was used as the key to the entry IT is pointing
 * to. The user should validate the iterator using TierHashMapIteratorIsValid
 * before calling this function. Calling this function on an invalid iterator
 * results in undefined behavior.
 *
 * @param it Tier hash map iterator.
 * @return Key to the entry.
 */
Tier TierHashMapIteratorKey(const TierHashMapIterator *it);

/**
 * @brief Returns the value of the entry IT is pointing to. The user should
 * validate the iterator using TierHashMapIteratorIsValid before calling this
 * function. Calling this function on an invalid iterator results in undefined
 * behavior.
 *
 * @param it Tier hash map iterator.
 * @return Value of the entry.
 */
int64_t TierHashMapIteratorValue(const TierHashMapIterator *it);

/** @brief Returns true if the given IT-erator is valid, or false otherwise. */
bool TierHashMapIteratorIsValid(const TierHashMapIterator *it);

/**
 * @brief Moves iterator IT to the next valid entry inside the Tier hash map
 * that IT was initialized with, sets TIER to the key and VALUE to the value of
 * that next entry, and returns true. Sets IT to an invalid state and returns
 * false if no valid next entry exists.
 *
 * @note This function is designed to be used in conjunction with
 * TierHashMapBegin to iterate through all entries in the Tier hash map. Calling
 * this function on an uninitialized iterator results in undefined behavior.
 *
 * @param iterator Initialized iterator.
 * @param tier Pointer to enough space to hold a Tier object, or NULL if the
 * key to the next entry is not needed.
 * @param value Pointer to enough space to hold an int64_t object, or NULL if
 * the value to the next entry is not needed.
 * @return true if next entry exists,
 * @return false otherwise.
 */
bool TierHashMapIteratorNext(TierHashMapIterator *iterator, Tier *tier,
                             int64_t *value);

#endif  // GAMESMANONE_CORE_TYPES_TIER_HASH_MAP_H_
