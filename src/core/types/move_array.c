/**
 * @file move_array.c
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Dynamic Move array implementation.
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

#include "core/types/move_array.h"

#include "core/data_structures/int64_array.h"
#include "core/types/base.h"

void MoveArrayInit(MoveArray *array) { Int64ArrayInit(array); }

void MoveArrayDestroy(MoveArray *array) { Int64ArrayDestroy(array); }

bool MoveArrayAppend(MoveArray *array, Move move) {
    return Int64ArrayPushBack(array, move);
}

bool MoveArrayPopBack(MoveArray *array) {
    if (array->size <= 0) return false;
    --array->size;
    return true;
}

void MoveArraySortExplicit(MoveArray *array,
                           int (*comp)(const void *, const void *)) {
    Int64ArraySortExplicit(array, comp);
}

bool MoveArrayContains(const MoveArray *array, Move move) {
    return Int64ArrayContains(array, move);
}
