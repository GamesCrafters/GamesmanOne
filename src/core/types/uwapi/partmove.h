#ifndef GAMESMANONE_CORE_TYPES_UWAPI_PARTMOVE_H
#define GAMESMANONE_CORE_TYPES_UWAPI_PARTMOVE_H

#include "core/data_structures/cstring.h"

typedef struct Partmove {
    CString autogui_move;
    CString formal_move;
    CString from;
    CString to;
    CString full;
} Partmove;

void PartMoveDestroy(Partmove *p);

#endif  // GAMESMANONE_CORE_TYPES_UWAPI_PARTMOVE_H
