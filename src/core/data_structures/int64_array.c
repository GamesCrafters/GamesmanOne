#include "core/data_structures/int64_array.h"

#include <assert.h>   // assert
#include <malloc.h>   // free, realloc
#include <stdbool.h>  // bool, true, false
#include <stddef.h>   // NULL
#include <stdint.h>   //int64_t

void Int64ArrayInit(Int64Array *array) {
    array->array = NULL;
    array->size = 0;
    array->capacity = 0;
}

void Int64ArrayDestroy(Int64Array *array) {
    free(array->array);
    array->array = NULL;
    array->size = 0;
    array->capacity = 0;
}

bool Int64ArrayExpand(Int64Array *array) {
    int64_t new_capacity = array->capacity == 0 ? 1 : array->capacity * 2;
    int64_t *new_array =
        (int64_t *)realloc(array->array, new_capacity * sizeof(int64_t));
    if (!new_array) return false;
    array->array = new_array;
    array->capacity = new_capacity;
    return true;
}

bool Int64ArrayPushBack(Int64Array *array, int64_t item) {
    // Expand array if necessary.
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
