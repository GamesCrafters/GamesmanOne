/**
 * @file move_array.h
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Dynamic Move array.
 * @version 2.0.0
 * @date 2024-12-10
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

#include "core/data_structures/int64_array.h"
#include "core/types/base.h"

/** @brief Dynamic Move array. */
typedef Int64Array MoveArray;

/** @brief Initializes the move ARRAY to an empty array. */
void MoveArrayInit(MoveArray *array);

/** @brief Destroyes the move ARRAY. */
void MoveArrayDestroy(MoveArray *array);

/**
 * @brief Appends MOVE to the end of ARRAY.
 *
 * @param array Destination array.
 * @param move Move to append.
 * @return true on success,
 * @return false otherwise.
 */
bool MoveArrayAppend(MoveArray *array, Move move);

/**
 * @brief Removes the last Move in ARRAY.
 *
 * @param array Target array.
 * @return true on success,
 * @return false if ARRAY is empty.
 */
bool MoveArrayPopBack(MoveArray *array);

/**
 * @brief Sorts the move ARRAY in ascending order using the given COMP-arison
 * function.
 *
 * @param array Array to be sorted.
 * @param comp Comparison function which returns ​a negative integer value if
 * the first argument is less than the second, a positive integer value if the
 * first argument is greater than the second and zero if the arguments are
 * equivalent.
 */
void MoveArraySortExplicit(MoveArray *array,
                           int (*comp)(const void *, const void *));

/** @brief Returns true if ARRAY contains MOVE, or false otherwise. */
bool MoveArrayContains(const MoveArray *array, Move move);

#endif  // GAMESMANONE_CORE_TYPES_MOVE_ARRAY_H_
