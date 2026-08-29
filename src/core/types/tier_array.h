/**
 * @file tier_array.h
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Dynamic tier array.
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

#include <stdbool.h>

#include "core/data_structures/int64_array.h"
#include "core/types/base.h"

/**
 * @brief Dynamic `Tier` array.
 */
typedef Int64Array TierArray;

/**
 * @brief Initializes `array`.
 *
 * @param[out] array Array to initialize.
 */
static inline void TierArrayInit(TierArray *array) { Int64ArrayInit(array); }

/**
 * @brief Initializes `dest` array to be a copy of the `src` array.
 *
 * @details If `src` uses a custom memory allocator, a new reference will be
 * copied to the `dest` array.
 *
 * @param[out] dest Array to initialize.
 * @param[in] src Source array to copy from.
 *
 * @retval true on success.
 * @retval false otherwise.
 */
static inline bool TierArrayInitCopy(TierArray *dest, const TierArray *src) {
    return Int64ArrayInitCopy(dest, src);
}

/**
 * @brief Deallocates `array`.
 *
 * @param[in,out] array Array to deallocate.
 */
static inline void TierArrayDestroy(TierArray *array) {
    Int64ArrayDestroy(array);
}

/**
 * @brief Appends a new `tier` to the back of the `array`.
 *
 * @param[in,out] array Destination array.
 * @param[in] tier New tier.
 *
 * @retval true on success.
 * @retval false otherwise.
 */
static inline bool TierArrayAppend(TierArray *array, Tier tier) {
    return Int64ArrayPushBack(array, tier);
}

/**
 * @brief Removes the first occurrence of `tier` from `array`, if it exists.
 *
 * @param[in,out] array Array of `Tier`.
 * @param[in] tier Value to remove.
 *
 * @retval true if `tier` exists in `array` and was removed.
 * @retval false otherwise.
 */
static inline bool TierArrayRemoveUnordered(TierArray *array, Tier tier) {
    return Int64ArrayRemoveUnordered(array, tier);
}

/**
 * @brief Pops the item at the back of the `array`.
 *
 * @details Calling this function on an empty `array` results in undefined
 * behavior.
 *
 * @param[in,out] array Array to pop the item from.
 */
static inline void TierArrayPopBack(TierArray *array) {
    Int64ArrayPopBack(array);
}

/**
 * @brief Returns the item at the back of `array`.
 *
 * @details Calling this function on an empty `array` results in undefined
 * behavior.
 *
 * @param[in] array Array to get the item from.
 *
 * @return Item at the back of `array`.
 */
static inline Tier TierArrayBack(const TierArray *array) {
    return Int64ArrayBack(array);
}

/**
 * @brief Returns whether the given `array` is empty.
 *
 * @param[in] array Array to check.
 *
 * @retval true if the array is empty.
 * @retval false otherwise.
 */
static inline bool TierArrayEmpty(const TierArray *array) {
    return Int64ArrayEmpty(array);
}

/**
 * @brief Sorts the given `array` according to the given comparison function.
 *
 * @param[in,out] array The array to be sorted.
 * @param[in] comp Comparison function which returns a negative integer value if
 * the first argument is less than the second, a positive integer value if the
 * first argument is greater than the second, and zero if the arguments are
 * equivalent.
 */
static inline void TierArraySortExplicit(TierArray *array,
                                         int (*comp)(const void *,
                                                     const void *)) {
    Int64ArraySortExplicit(array, comp);
}

#endif  // GAMESMANONE_CORE_TYPES_TIER_ARRAY_H_
