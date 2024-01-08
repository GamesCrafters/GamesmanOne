#ifndef GAMESMANONE_CORE_TYPES_UWAPI_UWAPI_TIER_H
#define GAMESMANONE_CORE_TYPES_UWAPI_UWAPI_TIER_H

#include "core/data_structures/cstring.h"
#include "core/types/base.h"

typedef struct UwapiTier {
    TierPosition (*FormalPositionToTierPosition)(
        ReadOnlyString formal_position);
    CString (*TierPositionToFormalPosition)(TierPosition position);
    CString (*TierPositionToUwapiPosition)(TierPosition position);
    CString (*TierMoveToUwapiMove)(TierPosition position, Move move);
    TierPosition (*GetInitialTierPosition)(void);
    TierPosition (*GetRandomLegalTierPosition)(void);
} UwapiTier;

#endif  // GAMESMANONE_CORE_TYPES_UWAPI_UWAPI_TIER_H
