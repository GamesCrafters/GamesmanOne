/**
 * @file int64_array.c
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Dynamic int64_t array implementation.
 * @version 2.1.0
 * @date 2025-04-04
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

#include <assert.h>   // assert
#include <stdbool.h>  // bool, true, false
#include <stddef.h>   // NULL
#include <stdint.h>   // int64_t
#include <stdlib.h>   // qsort
#include <string.h>   // memset, memmove

#include "core/gamesman_memory.h"

void Int64ArrayInit(Int64Array *array) { Int64ArrayInitAllocator(array, NULL); }

void Int64ArrayInitAllocator(Int64Array *array, GamesmanAllocator *allocator) {
    array->array = NULL;
    array->size = 0;
    array->capacity = 0;
    array->allocator = allocator;
}

bool Int64ArrayInitCopy(Int64Array *dest, const Int64Array *src) {
    Int64ArrayInitAllocator(dest, src->allocator);
    if (src->size == 0) return true;

    dest->array = (int64_t *)GamesmanAllocatorAllocate(
        dest->allocator, src->size * sizeof(int64_t));
    if (dest->array == NULL) return false;

    memcpy(dest->array, src->array, src->size * sizeof(int64_t));
    dest->size = src->size;
    dest->capacity = src->size;

    return true;
}

void Int64ArrayDestroy(Int64Array *array) {
    GamesmanFree(array->array);
    array->array = NULL;
    array->size = 0;
    array->capacity = 0;
}

bool Int64ArrayExpand(Int64Array *array) {
    int64_t new_capacity = array->capacity == 0 ? 1 : array->capacity * 2;
    int64_t *new_array = (int64_t *)GamesmanAllocatorAllocate(
        array->allocator, new_capacity * sizeof(int64_t));
    if (!new_array) return false;

    // Copy contents over.
    memcpy(new_array, array->array, array->capacity * sizeof(int64_t));
    GamesmanAllocatorDeallocate(array->allocator, array->array);
    array->array = new_array;
    array->capacity = new_capacity;

    return true;
}

bool Int64ArrayPushBack(Int64Array *array, int64_t item) {
    // Expand the array if necessary.
    if (array->size == array->capacity) {
        if (!Int64ArrayExpand(array)) {
            return false;
        }
    }
    assert(array->size < array->capacity);
    array->array[array->size++] = item;
    return true;
}

void Int64ArrayPopBack(Int64Array *array) {
    assert(array->size > 0);
    --array->size;
}

int64_t Int64ArrayBack(const Int64Array *array) {
    assert(array->array && array->size > 0);
    return array->array[array->size - 1];
}

bool Int64ArrayEmpty(const Int64Array *array) { return array->size == 0; }

bool Int64ArrayContains(const Int64Array *array, int64_t item) {
    for (int64_t i = 0; i < array->size; ++i) {
        if (array->array[i] == item) return true;
    }
    return false;
}

static int Int64Comp(const void *a, const void *b) {
    int64_t aa = *(const int64_t *)a;
    int64_t bb = *(const int64_t *)b;

    return (aa > bb) - (aa < bb);
}

void Int64ArraySortAscending(Int64Array *array) {
    qsort(array->array, array->size, sizeof(int64_t), Int64Comp);
}

void Int64ArraySortExplicit(Int64Array *array,
                            int (*comp)(const void *, const void *)) {
    qsort(array->array, array->size, sizeof(int64_t), comp);
}

bool Int64ArrayResize(Int64Array *array, int64_t size) {
    if (size <= 0) {
        Int64ArrayDestroy(array);
        return true;
    }

    // Expand if necessary.
    if (array->capacity < size) {
        int64_t *new_array = (int64_t *)GamesmanAllocatorAllocate(
            array->allocator, size * sizeof(int64_t));
        if (new_array == NULL) return false;

        memcpy(new_array, array->array, array->size * sizeof(int64_t));
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

bool Int64ArrayRemoveIndex(Int64Array *array, int64_t index) {
    if (index < 0 || index >= array->size) return false;

    int64_t move_size = (array->size - index - 1) * (int64_t)sizeof(int64_t);
    memmove(&array->array[index], &array->array[index + 1], move_size);
    --array->size;

    return true;
}

bool Int64ArrayRemove(Int64Array *array, int64_t item) {
    for (int64_t i = 0; i < array->size; ++i) {
        if (array->array[i] == item) {
            return Int64ArrayRemoveIndex(array, i);
        }
    }

    // Item does not exist.
    return false;
}
