#ifndef GAMESMANONE_CORE_TYPES_MOVE_ARRAY_H
#define GAMESMANONE_CORE_TYPES_MOVE_ARRAY_H

#include "core/data_structures/int64_array.h"
#include "core/types/base.h"

/** @brief Dynamic Move array. */
typedef Int64Array MoveArray;

void MoveArrayInit(MoveArray *array);
void MoveArrayDestroy(MoveArray *array);
bool MoveArrayAppend(MoveArray *array, Move move);
bool MoveArrayPopBack(MoveArray *array);
bool MoveArrayContains(const MoveArray *array, Move move);

#endif  // GAMESMANONE_CORE_TYPES_MOVE_ARRAY_H
