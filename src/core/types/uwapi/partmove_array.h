#ifndef GAMESMANONE_CORE_TYPES_UWAPI_PARTMOVE_ARRAY_H_
#define GAMESMANONE_CORE_TYPES_UWAPI_PARTMOVE_ARRAY_H_

#include <stdint.h>  // int64_t

#include "core/data_structures/cstring.h"
#include "core/types/uwapi/partmove.h"

typedef struct PartmoveArray {
    Partmove *array;
    int64_t size;
    int64_t capacity;
} PartmoveArray;

void PartmoveArrayInit(PartmoveArray *pa);
void PartmoveArrayDestroy(PartmoveArray *pa);

int PartmoveArrayEmplaceBack(PartmoveArray *pa, CString *autogui_move,
                             CString *formal_move, CString *from, CString *to,
                             CString *full);

#endif  // GAMESMANONE_CORE_TYPES_UWAPI_PARTMOVE_ARRAY_H
