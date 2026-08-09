/**
 * @file int64_array.c
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Dynamic int64_t array implementation.
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

#include "core/data_structures/int64_array.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "core/gamesman_memory.h"

bool Int64ArrayInitCopy(Int64Array *dest, const Int64Array *src) {
    if (src->size == 0) {
        Int64ArrayInitAllocator(dest, src->allocator);
        return true;
    }

    dest->array = (int64_t *)GamesmanAllocatorAllocate(
        src->allocator, src->size * sizeof(int64_t));
    if (dest->array == NULL) {
        return false;
    }

    memcpy(dest->array, src->array, src->size * sizeof(int64_t));
    dest->size = src->size;
    dest->capacity = src->size;
    GamesmanAllocatorAddRef(src->allocator);
    dest->allocator = src->allocator;

    return true;
}

bool Int64ArrayInternalExpand(Int64Array *array) {
    // The minimum capacity should at least fill up a cache line.
    static const int64_t kMinimumCapacity =
        GM_CACHE_LINE_SIZE / sizeof(int64_t);
    int64_t new_capacity =
        array->capacity == 0 ? kMinimumCapacity : array->capacity * 2;
    int64_t *new_array = (int64_t *)GamesmanAllocatorAllocate(
        array->allocator, new_capacity * sizeof(int64_t));
    if (!new_array) {
        return false;
    }

    // Copy contents over.
    if (array->capacity) {
        memcpy(new_array, array->array, array->capacity * sizeof(int64_t));
    }
    GamesmanAllocatorDeallocate(array->allocator, array->array);
    array->array = new_array;
    array->capacity = new_capacity;

    return true;
}

bool Int64ArrayContains(const Int64Array *array, int64_t item) {
    for (int64_t i = 0; i < array->size; ++i) {
        if (array->array[i] == item) {
            return true;
        }
    }
    return false;
}

void Int64ArraySortExplicit(Int64Array *array,
                            int (*comp)(const void *, const void *)) {
    qsort(array->array, array->size, sizeof(int64_t), comp);
}

bool Int64ArrayResize(Int64Array *array, int64_t size) {
    if (size <= 0) {
        array->size = 0;
        return true;
    }

    // Expand if necessary.
    if (array->capacity < size) {
        int64_t *new_array = (int64_t *)GamesmanAllocatorAllocate(
            array->allocator, size * sizeof(int64_t));
        if (new_array == NULL) {
            return false;
        }

        if (array->array) {
            memcpy(new_array, array->array, array->size * sizeof(int64_t));
        }
        GamesmanAllocatorDeallocate(array->allocator, array->array);
        array->array = new_array;
        array->capacity = size;
    }

    int64_t pad_length = size - array->size;
    if (pad_length > 0) {
        memset(&array->array[array->size], 0, pad_length * sizeof(int64_t));
    }
    array->size = size;

    return true;
}

static void Int64ArrayRemoveIndexUnordered(Int64Array *array, int64_t index) {
    array->array[index] = array->array[--array->size];
}

bool Int64ArrayRemoveUnordered(Int64Array *array, int64_t item) {
    for (int64_t i = 0; i < array->size; ++i) {
        if (array->array[i] == item) {
            Int64ArrayRemoveIndexUnordered(array, i);
            return true;
        }
    }

    // Item does not exist.
    return false;
}
