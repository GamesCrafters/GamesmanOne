/**
 * @file tier_position_array.c
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Dynamic TierPosition array implementation.
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

#include "core/types/tier_position_array.h"

#include <assert.h>   // assert
#include <stdbool.h>  // bool, true, false
#include <stddef.h>   // NULL
#include <stdint.h>   // int64_t
#include <stdlib.h>   // free, realloc

#include "core/constants.h"
#include "core/types/base.h"

void TierPositionArrayInit(TierPositionArray *array) {
    array->array = NULL;
    array->size = 0;
    array->capacity = 0;
}

void TierPositionArrayDestroy(TierPositionArray *array) {
    free(array->array);
    array->array = NULL;
    array->size = 0;
    array->capacity = 0;
}

static bool TierPositionArrayExpand(TierPositionArray *array) {
    int64_t new_capacity = array->capacity == 0 ? 1 : array->capacity * 2;
    TierPosition *new_array = (TierPosition *)realloc(
        array->array, new_capacity * sizeof(TierPosition));
    if (!new_array) return false;
    array->array = new_array;
    array->capacity = new_capacity;
    return true;
}

bool TierPositionArrayAppend(TierPositionArray *array,
                             TierPosition tier_position) {
    if (array->size == array->capacity) {
        if (!TierPositionArrayExpand(array)) return false;
    }
    assert(array->size < array->capacity);
    array->array[array->size++] = tier_position;
    return true;
}

TierPosition TierPositionArrayBack(const TierPositionArray *array) {
    return array->array[array->size - 1];
}

bool TierPositionArrayContains(const TierPositionArray *array,
                               TierPosition target) {
    for (int64_t i = 0; i < array->size; ++i) {
        if (array->array[i].position == target.position &&
            array->array[i].tier == target.tier) {
            return true;
        }
    }

    return false;
}
