/**
 * @file tier_hash_set.h
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Linear-probing Tier hash set.
 * @version 1.0.2
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

#ifndef GAMESMANONE_CORE_TYPES_TIER_HASH_SET_H_
#define GAMESMANONE_CORE_TYPES_TIER_HASH_SET_H_

#include "core/data_structures/int64_hash_set.h"
#include "core/types/base.h"

/** @brief Linear-probing Tier hash set using Int64HashSet as underlying type.
 */
typedef Int64HashSet TierHashSet;

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
void TierHashSetInit(TierHashSet *set, double max_load_factor);

/** @brief Deallocates the given \p set. */
void TierHashSetDestroy(TierHashSet *set);

/**
 * @brief Tests if \p tier is in \p set.
 *
 * @param set Set from which the given \p tier is looked up.
 * @param tier Tier value to look for.
 * @return true if \p set contains \p tier, or
 * @return false otherwise.
 */
bool TierHashSetContains(const TierHashSet *set, Tier tier);

/**
 * @brief Adds \p tier to the given \p set, or does nothing if \p set
 * already contains \p tier.
 *
 * @param set Set to add \p tier to.
 * @param tier Tier to add.
 * @return true on success,
 * @return false otherwise.
 */
bool TierHashSetAdd(TierHashSet *set, Tier tier);

#endif  // GAMESMANONE_CORE_TYPES_TIER_HASH_SET_H_
