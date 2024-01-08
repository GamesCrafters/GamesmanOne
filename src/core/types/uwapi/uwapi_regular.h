#ifndef GAMESMANONE_CORE_TYPES_UWAPI_UWAPI_REGULAR_H
#define GAMESMANONE_CORE_TYPES_UWAPI_UWAPI_REGULAR_H

#include "core/data_structures/cstring.h"
#include "core/types/base.h"

typedef struct UwapiRegular {
    Position (*FormalPositionToPosition)(ReadOnlyString formal_position);
    CString (*PositionToFormalPosition)(Position position);
    CString (*PositionToUwapiPosition)(Position position);
    CString (*MoveToUwapiMove)(Position position, Move move);
    Position (*GetInitialPosition)(void);
    Position (*GetRandomLegalPosition)(void);
} UwapiRegular;

#endif  // GAMESMANONE_CORE_TYPES_UWAPI_UWAPI_REGULAR_H
