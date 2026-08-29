/**
 * @file tier_hash_set.h
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Linear-probing Tier hash set.
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

#include <stdbool.h>

#include "core/data_structures/int64_hash_set.h"
#include "core/types/base.h"

/**
 * @brief A linear-probing Tier hash set using `Int64HashSet` as the underlying
 * type.
 */
typedef Int64HashSet TierHashSet;

/**
 * @brief Initializes a Tier hash set.
 *
 * @param[out] set The `TierHashSet` to initialize.
 * @param[in] max_load_factor The maximum load factor, clamped to [0.5, 0.8].
 */
static inline void TierHashSetInit(TierHashSet *set, double max_load_factor) {
    Int64HashSetInit(set, max_load_factor);
}

/**
 * @brief Frees the memory associated with the hash set and resets its state.
 *
 * @param[in,out] set The `TierHashSet` to destroy.
 */
static inline void TierHashSetDestroy(TierHashSet *set) {
    Int64HashSetDestroy(set);
}

/**
 * @brief Checks if a specific Tier is present in the hash set.
 *
 * @param[in] set The `TierHashSet` to search.
 * @param[in] tier The `Tier` value to look for.
 *
 * @retval true The `tier` is present in the set.
 * @retval false The `tier` is not present, or the set is uninitialized.
 */
static inline bool TierHashSetContains(const TierHashSet *set, Tier tier) {
    return Int64HashSetContains(set, tier);
}

/**
 * @brief Adds a Tier value to the hash set.
 *
 * @param[in,out] set The `TierHashSet` to add the Tier to.
 * @param[in] tier The `Tier` value to add.
 *
 * @retval true The `tier` was successfully added.
 * @retval false The `tier` already exists, or memory allocation failed.
 */
static inline bool TierHashSetAdd(TierHashSet *set, Tier tier) {
    return Int64HashSetAdd(set, tier);
}

#endif  // GAMESMANONE_CORE_TYPES_TIER_HASH_SET_H_
