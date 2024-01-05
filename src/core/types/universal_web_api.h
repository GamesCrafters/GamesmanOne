#ifndef GAMESMANONE_CORE_TYPES_UNIVERSAL_WEB_API_H
#define GAMESMANONE_CORE_TYPES_UNIVERSAL_WEB_API_H

#include "core/data_structures/cstring.h"
#include "core/types/base.h"

typedef struct UniversalWebApi {
    Position (*FormalPositionToPosition)(ReadOnlyString formal_position);
    TierPosition (*FormalPositionToTierPosition)(
        ReadOnlyString formal_position);

    CString (*PositionToFormalPosition)(Position position);
    CString (*TierPositionToFormalPosition)(TierPosition position);

    CString (*PositionToUwapiPosition)(Position position);
    CString (*TierPositionToUwapiPosition)(TierPosition position);

    CString (*MoveToUwapiMove)(Position position, Move move);
    CString (*TierMoveToUwapiMove)(TierPosition position, Move move);

    Position (*GetRandomLegalPosition)(void);
    TierPosition (*GetRandomLegalTierPosition)(void);
} UniversalWebApi;

#endif  // GAMESMANONE_CORE_TYPES_UNIVERSAL_WEB_API_H
