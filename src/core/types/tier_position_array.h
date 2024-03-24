/**
 * @file tier_position_array.h
 * @author Robert Shi (robertyishi@berkeley.edu)
 *         GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Dynamic TierPosition array.
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

#ifndef GAMESMANONE_CORE_TYPES_TIER_POSITION_ARRAY_H
#define GAMESMANONE_CORE_TYPES_TIER_POSITION_ARRAY_H

#include <stdbool.h>  // bool
#include <stdint.h>   // int64_t

#include "core/types/base.h"

/** @brief Dynamic TierPosition Array. */
typedef struct TierPositionArray {
    TierPosition *array; /**< Array contents. */
    int64_t size;        /**< Size of the array in number of items. */
    int64_t capacity;    /**< Capacity of the array in number of items. */
} TierPositionArray;

/** @brief Initializes the TierPosition ARRAY to an empty array. */
void TierPositionArrayInit(TierPositionArray *array);

/** @brief Destroyes the TierPosition ARRAY. */
void TierPositionArrayDestroy(TierPositionArray *array);

/**
 * @brief Appends TIER_POSITION to the end of ARRAY.
 *
 * @param array Destination array.
 * @param position Tier position to append.
 * @return true on success,
 * @return false otherwise.
 */
bool TierPositionArrayAppend(TierPositionArray *array,
                             TierPosition tier_position);

/**
 * @brief Returns the last TierPosition in ARRAY.
 *
 * @param array Source tier position array.
 * @return Last item in ARRAY.
 */
TierPosition TierPositionArrayBack(const TierPositionArray *array);

/** @brief Returns whether the given TierPosition ARRAY contains the TARGET. */
bool TierPositionArrayContains(const TierPositionArray *array,
                               TierPosition target);

#endif  // GAMESMANONE_CORE_TYPES_TIER_POSITION_ARRAY_H
