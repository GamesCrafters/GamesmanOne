/**
 * @file int64_array.c
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Dynamic int64_t array implementation.
 * @version 2.1.0
 * @date 2025-03-18
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
#include <stdio.h>    // fprintf, stderr
#include <stdlib.h>   // free, aligned_alloc, realloc, qsort
#include <string.h>   // memset, memmove

void Int64ArrayInit(Int64Array *array) { Int64ArrayInitAligned(array, 0); }

int Int64ArrayInitAligned(Int64Array *array, int align) {
    // Check alignemnt requirements.
    if (align % sizeof(void *) || (align & (align - 1))) {
        return 1;
    }

    array->array = NULL;
    array->size = 0;
    array->capacity = 0;
    array->align = align;

    return 0;
}

bool Int64ArrayInitCopy(Int64Array *dest, const Int64Array *src) {
    Int64ArrayInit(dest);
    if (src->size == 0) return true;

    dest->array = (int64_t *)malloc(src->size * sizeof(int64_t));
    if (dest->array == NULL) return false;

    memcpy(dest->array, src->array, src->size * sizeof(int64_t));
    dest->size = src->size;
    dest->capacity = src->size;
    dest->align = src->align;

    return true;
}

void Int64ArrayDestroy(Int64Array *array) {
    free(array->array);
    array->array = NULL;
    array->size = 0;
    array->capacity = 0;
    array->align = 0;
}

static int64_t NextMultiple(int64_t n, int64_t mult) {
    return (n + mult - 1) / mult * mult;
}

static bool ExpandAligned(Int64Array *array, int64_t desired_capacity) {
    size_t required_bytes = desired_capacity * sizeof(int64_t);

    // aligned_alloc requires the allocation size to be a multiple of the
    // alignment. Adjust alloc_size to the smallest multiple of array->align
    // that is >= required_bytes.
    size_t alloc_size = NextMultiple(required_bytes, array->align);

    // Update new capacity based on the actual allocation size (avoiding wasted
    // space)
    int64_t new_capacity = alloc_size / sizeof(int64_t);

    // Allocate new memory with the specified alignment.
    int64_t *new_array = aligned_alloc(array->align, alloc_size);
    if (!new_array) return false;

    // If the array already contains data, copy it to the new memory block.
    if (array->array) {
        memcpy(new_array, array->array, array->size * sizeof(int64_t));
        free(array->array);
    }

    // Update the array structure with the new allocation and capacity.
    array->array = new_array;
    array->capacity = new_capacity;

    return true;
}

static bool ExpandUnaligned(Int64Array *array, int new_capacity) {
    int64_t *new_array =
        (int64_t *)realloc(array->array, new_capacity * sizeof(int64_t));
    if (!new_array) return false;
    array->array = new_array;
    array->capacity = new_capacity;

    return true;
}

static bool Int64ArrayExpand(Int64Array *array) {
    int64_t desired_capacity = (array->capacity == 0 ? 1 : array->capacity * 2);
    if (array->align) return ExpandAligned(array, desired_capacity);

    return ExpandUnaligned(array, desired_capacity);
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
        if (array->align) {
            if (!ExpandAligned(array, size)) return false;
        } else {
            if (!ExpandUnaligned(array, size)) return false;
        }
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
