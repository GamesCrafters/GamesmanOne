/**
 * @file position_array.h
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Dynamic Position array.
 * @version 1.0.1
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

#ifndef GAMESMANONE_CORE_TYPES_POSITION_ARRAY_H_
#define GAMESMANONE_CORE_TYPES_POSITION_ARRAY_H_

#include "core/data_structures/int64_array.h"
#include "core/types/base.h"

/** @brief Dynamic Position array. */
typedef Int64Array PositionArray;

/** @brief Initializes the position ARRAY to an empty array. */
void PositionArrayInit(PositionArray *array);

/** @brief Destroyes the position ARRAY. */
void PositionArrayDestroy(PositionArray *array);

/**
 * @brief Appends POSITION to the end of ARRAY.
 *
 * @param array Destination array.
 * @param position Position to append.
 * @return true on success,
 * @return false otherwise.
 */
bool PositionArrayAppend(PositionArray *array, Position position);

/** @brief Returns true if ARRAY contains POSITION, or false otherwise. */
bool PositionArrayContains(PositionArray *array, Position position);

#endif  // GAMESMANONE_CORE_TYPES_POSITION_ARRAY_H_
