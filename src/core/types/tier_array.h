/**
 * @file tier_array.h
 * @author Robert Shi (robertyishi@berkeley.edu)
 *         GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Dynamic tier array.
 * @version 1.1.0
 * @date 2024-09-07
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

#ifndef GAMESMANONE_CORE_TYPES_TIER_ARRAY_H_
#define GAMESMANONE_CORE_TYPES_TIER_ARRAY_H_

#include "core/data_structures/int64_array.h"
#include "core/types/base.h"

/** @brief Dynamic Tier array. */
typedef Int64Array TierArray;

/** @brief Initializes the Tier ARRAY. */
void TierArrayInit(TierArray *array);

/** @brief Initializes the DEST tier array as a copy of tier array SRC. */
bool TierArrayInitCopy(TierArray *dest, const TierArray *src);

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

/**
 * @brief Pops the item at the back of the ARRAY. Calling this function on an
 * empty ARRAY results in undefined behavior.
 */
void TierArrayPopBack(TierArray *array);

/**
 * @brief Returns the Tier at the back of the array. Calling this function on an
 * empty ARRAY results in undefined behavior.
 */
Tier TierArrayBack(const TierArray *array);

/** @brief Returns true if ARRAY is empty, or false otherwise. */
bool TierArrayEmpty(const TierArray *array);

/**
 * @brief Sorts the given ARRAY according to the given comparison function.
 * @param array The array to be sorted.
 * @param comp 	Comparison function which returns ​a negative integer value if
 * the first argument is less than the second, a positive integer value if the
 * first argument is greater than the second, and zero if the arguments are
 * equivalent. Both parameters are assumed to be pointers to \c Tier objects.
 */
void TierArraySortExplicit(TierArray *array,
                           int (*comp)(const void *, const void *));

#endif  // GAMESMANONE_CORE_TYPES_TIER_ARRAY_H_
