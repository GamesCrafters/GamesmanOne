/**
 * @file tier_position_array.h
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Dynamic TierPosition array.
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

#ifndef GAMESMANONE_CORE_TYPES_TIER_POSITION_ARRAY_H_
#define GAMESMANONE_CORE_TYPES_TIER_POSITION_ARRAY_H_

#include <stdbool.h>
#include <stdint.h>

#include "core/types/base.h"

/**
 * @brief Dynamic `TierPosition` array.
 */
typedef struct TierPositionArray {
    TierPosition *array; /**< The actual array. */
    int64_t size;        /**< Number of items in the array. */
    int64_t capacity;    /**< Current capacity of the array. */
} TierPositionArray;

/**
 * @brief Initializes `array`.
 *
 * @param[out] array Array to initialize.
 */
void TierPositionArrayInit(TierPositionArray *array);

/**
 * @brief Deallocates `array`.
 *
 * @param[in,out] array Array to deallocate.
 */
void TierPositionArrayDestroy(TierPositionArray *array);

/**
 * @brief Appends a new `tier_position` to the back of the `array`.
 *
 * @param[in,out] array Destination array.
 * @param[in] tier_position New item.
 *
 * @retval true on success.
 * @retval false otherwise.
 */
bool TierPositionArrayAppend(TierPositionArray *array,
                             TierPosition tier_position);

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
TierPosition TierPositionArrayBack(const TierPositionArray *array);

#endif  // GAMESMANONE_CORE_TYPES_TIER_POSITION_ARRAY_H_
