/**
 * @file tier_position_array.c
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Dynamic TierPosition array implementation.
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

#include "core/types/tier_position_array.h"

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "core/gamesman_memory.h"
#include "core/types/base.h"

void TierPositionArrayInit(TierPositionArray *array) {
    array->array = NULL;
    array->size = 0;
    array->capacity = 0;
}

void TierPositionArrayDestroy(TierPositionArray *array) {
    GamesmanFree(array->array);
    array->array = NULL;
    array->size = 0;
    array->capacity = 0;
}

static bool TierPositionArrayExpand(TierPositionArray *array) {
    int64_t new_capacity = array->capacity == 0 ? 1 : array->capacity * 2;
    TierPosition *new_array =
        (TierPosition *)GamesmanMalloc(new_capacity * sizeof(TierPosition));
    if (!new_array) {
        return false;
    }

    memcpy(new_array, array->array, array->capacity * sizeof(TierPosition));
    GamesmanFree(array->array);
    array->array = new_array;
    array->capacity = new_capacity;

    return true;
}

bool TierPositionArrayAppend(TierPositionArray *array,
                             TierPosition tier_position) {
    if (array->size == array->capacity) {
        if (!TierPositionArrayExpand(array)) {
            return false;
        }
    }
    array->array[array->size++] = tier_position;

    return true;
}

TierPosition TierPositionArrayBack(const TierPositionArray *array) {
    return array->array[array->size - 1];
}
