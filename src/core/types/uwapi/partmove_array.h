#ifndef GAMESMANONE_CORE_TYPES_UWAPI_PARTMOVE_ARRAY_H
#define GAMESMANONE_CORE_TYPES_UWAPI_PARTMOVE_ARRAY_H

#include <stdint.h>  // int64_t

#include "core/types/uwapi/partmove.h"

typedef struct PartmoveArray {
    Partmove *array;
    int64_t size;
    int64_t capacity;
} PartmoveArray;

void PartmoveArrayInit(PartmoveArray *pa);
void PartmoveArrayDestroy(PartmoveArray *pa);

int PartmoveArrayEmplaceBack(PartmoveArray *pa, const char *autogui_move,
                             const char *formal_move, const char *from,
                             const char *to, const char *full);

#endif  // GAMESMANONE_CORE_TYPES_UWAPI_PARTMOVE_ARRAY_H