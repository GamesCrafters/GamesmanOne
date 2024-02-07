/**
 * @file tier_array.h
 * @author Robert Shi (robertyishi@berkeley.edu)
 *         GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Dynamic tier array.
 *
 * @version 1.0.0
 * @date 2024-01-24
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

#ifndef GAMESMANONE_CORE_TYPES_TIER_ARRAY_H
#define GAMESMANONE_CORE_TYPES_TIER_ARRAY_H

#include "core/data_structures/int64_array.h"
#include "core/types/base.h"

/** @brief Dynamic Tier array. */
typedef Int64Array TierArray;

/** @brief Initializes the Tier ARRAY. */
void TierArrayInit(TierArray *array);

/** @brief Destroys the Tier ARRAY. */
void TierArrayDestroy(TierArray *array);

/**
 * @brief Appends TIER to the end of the given Tier ARRAY.
 * 
 * @param array Destination Tier array.
 * @param tier Tier to append.
 * @return true on success,
 * @return false otherwise.
 */
bool TierArrayAppend(TierArray *array, Tier tier);

/**
 * @brief Removes the first occurrence of TIER from ARRAY if it exists.
 * 
 * @param array Tier array.
 * @param tier Tier to remove.
 * @return true if TIER exists in ARRAY, or
 * @return false otherwise.
 */
bool TierArrayRemove(TierArray *array, Tier tier);

/** @brief Returns true if ARRAY is empty, or false otherwise. */
bool TierArrayEmpty(const TierArray *array);

#endif  // GAMESMANONE_CORE_TYPES_TIER_ARRAY_H
