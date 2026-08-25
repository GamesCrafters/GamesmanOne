/**
 * @file tier_to_ptr_chained_hash_map.h
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Separate chaining Tier to generic pointer (void *) hash map.
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

#ifndef GAMESMANONE_CORE_TYPES_TIER_TO_PTR_CHAINED_HASH_MAP_H_
#define GAMESMANONE_CORE_TYPES_TIER_TO_PTR_CHAINED_HASH_MAP_H_

#include <stdbool.h>
#include <stdint.h>

#include "core/data_structures/int64_to_ptr_chained_hash_map.h"
#include "core/types/base.h"

/**
 * @brief Separate chaining \c Tier to \c int64_t hash map.
 */
typedef Int64ToPtrChainedHashMap TierToPtrChainedHashMap;

/**
 * @brief \c TierToPtrChainedHashMap iterator, which is usually returned by an
 * \c Int64ToPtrChainedHashMap accessor function. No internal states are
 * intended to be inspect or manipulated directly. Use the provided API
 * functions instead.
 */
typedef struct Int64ToPtrChainedHashMapIterator TierToPtrChainedHashMapIterator;

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
static inline void TierToPtrChainedHashMapInit(TierToPtrChainedHashMap *map,
                                               double max_load_factor) {
    Int64ToPtrChainedHashMapInit(map, max_load_factor);
}

/** @brief Deallocates the given \p map. */
static inline void TierToPtrChainedHashMapDestroy(
    TierToPtrChainedHashMap *map) {
    Int64ToPtrChainedHashMapDestroy(map);
}

/**
 * @brief Returns an iterator to the entry containing the given \p tier in \p
 * map . Returns an invalid iterator if \p tier is not found in \p map . The
 * iterator returned must be tested by Int64ToPtrChainedHashMapIteratorIsValid
 * for validity before using.
 *
 * @param map Hash map to get the entry from.
 * @param tier Tier to the desired entry.
 * @return Int64ToPtrChainedHashMapIterator pointing to the entry with \p tier ,
 * or an invalid iterator if \p tier is not found in \p map .
 */
static inline TierToPtrChainedHashMapIterator TierToPtrChainedHashMapGet(
    const TierToPtrChainedHashMap *map, Tier tier) {
    return Int64ToPtrChainedHashMapGet(map, tier);
}

/**
 * @brief Sets the entry with \p tier in \p map to the given \p value and
 * returns
 * \c true. Creates a new entry if \p tier does not exist in \p map. If the
 * operation fails for any reason, \p map remains unchanged and the function
 * returns \c false.
 *
 * @param map Destination hash map.
 * @param tier Tier of the entry.
 * @param value Value of the entry.
 * @return \c true on success,
 * @return \c false otherwise.
 */
static inline bool TierToPtrChainedHashMapSet(TierToPtrChainedHashMap *map,
                                              Tier tier, void *value) {
    return Int64ToPtrChainedHashMapSet(map, tier, value);
}

/**
 * @brief Removes the entry with \p tier in \p map. Does nothing if \p tier does
 * not exist.
 *
 * @param map Target hash map.
 * @param tier Tier to the entry to remove.
 */
static inline void TierToPtrChainedHashMapRemove(TierToPtrChainedHashMap *map,
                                                 Tier tier) {
    Int64ToPtrChainedHashMapRemove(map, tier);
}

/**
 * @brief Returns an iterator to the first entry in \p map . Returns an invalid
 * iterator if \p map is empty.
 *
 * @param map Map the get the first entry from.
 * @return Int64ToPtrChainedHashMapIterator to the first entry in \p map , or
 * an invalid iterator if \p map is empty.
 */
static inline TierToPtrChainedHashMapIterator TierToPtrChainedHashMapBegin(
    const TierToPtrChainedHashMap *map) {
    return Int64ToPtrChainedHashMapBegin(map);
}

/**
 * @brief Returns the key of the entry that \p it is pointing to. The user
 * should validate the iterator using \c Int64ToPtrChainedHashMapIteratorIsValid
 * before calling this function.
 *
 * @param it Iterator.
 * @return Key to the entry pointed to by \p it.
 */
static inline int64_t TierToPtrChainedHashMapIteratorKey(
    const TierToPtrChainedHashMapIterator *it) {
    return Int64ToPtrChainedHashMapIteratorKey(it);
}

/**
 * @brief Returns the value of the entry that \p it is pointing to. The user
 * should validate the iterator using \c Int64ToPtrChainedHashMapIteratorIsValid
 * before calling this function.
 *
 * @param it Iterator.
 * @return Value of the entry pointed to by \p it.
 */
static inline void *TierToPtrChainedHashMapIteratorValue(
    const TierToPtrChainedHashMapIterator *it) {
    return Int64ToPtrChainedHashMapIteratorValue(it);
}

/**
 * @brief Returns \c true if \p it is a valid iterator, or \c false otherwise.
 */
static inline bool TierToPtrChainedHashMapIteratorIsValid(
    const TierToPtrChainedHashMapIterator *it) {
    return Int64ToPtrChainedHashMapIteratorIsValid(it);
}

/**
 * @brief Advances iterator \p it to the next valid entry in the hash map.
 *
 * @param it Non-NULL pointer to the iterator.
 * @return \c true if the next entry exists,
 * @return \c false otherwise.
 */
static inline bool TierToPtrChainedHashMapIteratorNext(
    TierToPtrChainedHashMapIterator *it) {
    return Int64ToPtrChainedHashMapIteratorNext(it);
}

#endif  // GAMESMANONE_CORE_TYPES_TIER_TO_PTR_CHAINED_HASH_MAP_H_
