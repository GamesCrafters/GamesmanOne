#ifndef GAMESMANONE_CORE_TYPES_POSITION_ARRAY_H
#define GAMESMANONE_CORE_TYPES_POSITION_ARRAY_H

#include "core/data_structures/int64_array.h"
#include "core/types/base.h"

/** @brief Dynamic Position array. */
typedef Int64Array PositionArray;

void PositionArrayInit(PositionArray *array);
void PositionArrayDestroy(PositionArray *array);
bool PositionArrayAppend(PositionArray *array, Position position);
bool PositionArrayContains(PositionArray *array, Position position);

#endif  // GAMESMANONE_CORE_TYPES_POSITION_ARRAY_H
