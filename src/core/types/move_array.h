/**
 * @file move_array.h
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Dynamic Move array.
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

#ifndef GAMESMANONE_CORE_TYPES_MOVE_ARRAY_H_
#define GAMESMANONE_CORE_TYPES_MOVE_ARRAY_H_

#include <stdbool.h>

#include "core/data_structures/int64_array.h"
#include "core/types/base.h"

/**
 * @brief Dynamic `Move` array.
 */
typedef Int64Array MoveArray;

/**
 * @brief Initializes `array`.
 *
 * @param[out] array Array to initialize.
 */
static inline void MoveArrayInit(MoveArray *array) { Int64ArrayInit(array); }

/**
 * @brief Deallocates `array`.
 *
 * @param[in,out] array Array to deallocate.
 */
static inline void MoveArrayDestroy(MoveArray *array) {
    Int64ArrayDestroy(array);
}

/**
 * @brief Appends a new `move` to the back of the `array`.
 *
 * @param[in,out] array Destination array.
 * @param[in] move New move.
 *
 * @retval true on success.
 * @retval false otherwise.
 */
static inline bool MoveArrayAppend(MoveArray *array, Move move) {
    return Int64ArrayPushBack(array, move);
}

/**
 * @brief Pops the move at the back of the `array`.
 *
 * @param[in,out] array Array to pop the move from.
 *
 * @retval true on success.
 * @retval false if the array is empty.
 */
static inline bool MoveArrayPopBack(MoveArray *array) {
    if (array->size <= 0) return false;
    --array->size;
    return true;
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
static inline void MoveArraySortExplicit(MoveArray *array,
                                         int (*comp)(const void *,
                                                     const void *)) {
    Int64ArraySortExplicit(array, comp);
}

/**
 * @brief Returns whether the given `array` contains the given `move`.
 *
 * @param[in] array Array to check.
 * @param[in] move Move to look for.
 *
 * @retval true if the array contains the move.
 * @retval false otherwise.
 */
static inline bool MoveArrayContains(const MoveArray *array, Move move) {
    return Int64ArrayContains(array, move);
}

#endif  // GAMESMANONE_CORE_TYPES_MOVE_ARRAY_H_
