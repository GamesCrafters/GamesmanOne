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

void MoveArraySort(MoveArray *array, int (*comp)(const void *, const void *)) {
    Int64ArraySort(array, comp);
}

bool MoveArrayContains(const MoveArray *array, Move move) {
    return Int64ArrayContains(array, move);
}
