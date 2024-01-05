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

bool TierPositionArrayResize(TierPositionArray *array, int64_t size) {
    if (size < 0) size = 0;

    // Expand if necessary.
    if (array->capacity < size) {
        TierPosition *new_array =
            (TierPosition *)realloc(array->array, size * sizeof(TierPosition));
        if (new_array == NULL) return false;

        array->array = new_array;
        array->capacity = size;
    }

    for (int64_t i = array->size; i < size; ++i) {
        array->array[i] = kIllegalTierPosition;
    }

    array->size = size;
    return true;
}
