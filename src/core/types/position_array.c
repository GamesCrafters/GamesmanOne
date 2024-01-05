#include "core/types/position_array.h"

#include "core/data_structures/int64_array.h"
#include "core/types/base.h"

void PositionArrayInit(PositionArray *array) { Int64ArrayInit(array); }

void PositionArrayDestroy(PositionArray *array) { Int64ArrayDestroy(array); }

bool PositionArrayAppend(PositionArray *array, Position position) {
    return Int64ArrayPushBack(array, position);
}

bool PositionArrayContains(PositionArray *array, Position position) {
    return Int64ArrayContains(array, position);
}
