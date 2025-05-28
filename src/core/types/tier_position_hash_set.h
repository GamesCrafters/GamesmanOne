/**
 * @file tier_position_hash_set.h
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Linear-probing TierPosition hash set.
 * @version 2.0.0
 * @date 2025-05-11
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

#ifndef GAMESMANONE_CORE_TYPES_TIER_POSITION_HASH_SET_H_
#define GAMESMANONE_CORE_TYPES_TIER_POSITION_HASH_SET_H_

#include <stdbool.h>  // bool
#include <stdint.h>   // int64_t

#include "core/types/base.h"

/** @brief Entry in a TierPositionHashSet. */
typedef struct TierPositionHashSetEntry {
    TierPosition key; /**< Item, which is also used as key for lookup. */
    bool used;        /**< True if bucket is full, false if empty. */
} TierPositionHashSetEntry;

/** @brief Linear-probing TierPosition hash set. */
typedef struct TierPositionHashSet {
    TierPositionHashSetEntry *entries; /**< Array of buckets. */
    int64_t capacity;                  /**< Number of buckets allocated. */
    int64_t size;           /**< Number of items stored in the set. */
    double max_load_factor; /**< Maximum load factor of the set. */
} TierPositionHashSet;

/**
 * @brief Initializes TierPosition hash set SET to an empty set with maximum
 * load factor MAX_LOAD_FACTOR.
 *
 * @param set Set to initialize.
 * @param max_load_factor Set maximum load factor of SET to this value. The hash
 * set will automatically expand its capacity if (double)size/capacity is
 * greater than this value. A small max_load_factor trades memory for
 * speed whereas a large max_load_factor trades speed for memory. This value is
 * restricted to be in the range [0.25, 0.75]. If the user passes a
 * max_load_factor that is smaller than 0.25 or greater than 0.75, the internal
 * value will be set to 0.25 and 0.75, respectively, regardless of the
 * user-specified value.
 */
void TierPositionHashSetInit(TierPositionHashSet *set, double max_load_factor);

/**
 * @brief Attempts to reserve space for \p size Tier Positions in \p set. If
 * \c true is returned, the target hash set \p set is guaranteed to have space
 * for at least \p size Tier Positions before it expands internally. If \c false
 * is returned, the hash set remains unchanged.
 * @note This function takes O( \p size ) time due to the initialization of the
 * internal array. This may become a bottleneck in a hot loop if the size of the
 * set exceeds L1 cache size.
 *
 * @param set Target hash set.
 * @param size Number of Tier Positions to reserve space for.
 * @return \c true on success,
 * @return \c false otherwise.
 */
bool TierPositionHashSetReserve(TierPositionHashSet *set, int64_t size);

/** @brief Destroys the TierPosition hash set SET. */
void TierPositionHashSetDestroy(TierPositionHashSet *set);

/**
 * @brief Returns true if the TierPosition hash set SET contains KEY, or
 * false otherwise.
 */
bool TierPositionHashSetContains(TierPositionHashSet *set, TierPosition key);

/**
 * @brief Adds \p key to \p set or does nothing if \p set already contains
 * \p key.
 *
 * @param set Set to add \p key to.
 * @param key Key to add to \p set.
 * @return \c true if \p key was added to \p set as a new key, or
 * @return \c false if \p set already contains \p key or an error occurred.
 */
bool TierPositionHashSetAdd(TierPositionHashSet *set, TierPosition key);

#endif  // GAMESMANONE_CORE_TYPES_TIER_POSITION_HASH_SET_H_
