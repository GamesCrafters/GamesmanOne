/**
 * @file position_array.h
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Dynamic Position array.
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

#ifndef GAMESMANONE_CORE_TYPES_POSITION_ARRAY_H_
#define GAMESMANONE_CORE_TYPES_POSITION_ARRAY_H_

#include <stdbool.h>

#include "core/data_structures/int64_array.h"
#include "core/gamesman_memory.h"
#include "core/types/base.h"

/**
 * @brief Dynamic `Position` array.
 */
typedef Int64Array PositionArray;

/**
 * @brief Initializes `array`.
 *
 * @param[out] array Array to initialize.
 */
static inline void PositionArrayInit(PositionArray *array) {
    Int64ArrayInit(array);
}

/**
 * @brief Initializes `array` using `allocator` as the underlying memory
 * allocator.
 *
 * @details If `allocator` is `NULL`, the function call is equivalent to
 * `PositionArrayInit(array)`. Note that this function does not transfer the
 * ownership of `allocator` to the new array object. The caller is responsible
 * for releasing its own copy of the allocator.
 *
 * @param[out] array Array to initialize.
 * @param[in,out] allocator Memory allocator to use.
 */
static inline void PositionArrayInitAllocator(PositionArray *array,
                                              GamesmanAllocator *allocator) {
    Int64ArrayInitAllocator(array, allocator);
}

/**
 * @brief Deallocates `array`.
 *
 * @param[in,out] array Array to deallocate.
 */
static inline void PositionArrayDestroy(PositionArray *array) {
    Int64ArrayDestroy(array);
}

/**
 * @brief Appends a new `position` to the back of the `array`.
 *
 * @param[in,out] array Destination array.
 * @param[in] position New position.
 *
 * @retval true on success.
 * @retval false otherwise.
 */
static inline bool PositionArrayAppend(PositionArray *array,
                                       Position position) {
    return Int64ArrayPushBack(array, position);
}

/**
 * @brief Returns whether the given `array` contains the given `position`.
 *
 * @param[in] array Array to check.
 * @param[in] position Position to look for.
 *
 * @retval true if the array contains the position.
 * @retval false otherwise.
 */
static inline bool PositionArrayContains(PositionArray *array,
                                         Position position) {
    return Int64ArrayContains(array, position);
}

#endif  // GAMESMANONE_CORE_TYPES_POSITION_ARRAY_H_
