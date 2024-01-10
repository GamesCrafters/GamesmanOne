#ifndef GAMESMANONE_CORE_TYPES_UWAPI_UWAPI_TIER_H
#define GAMESMANONE_CORE_TYPES_UWAPI_UWAPI_TIER_H

#include "core/data_structures/cstring.h"
#include "core/types/base.h"

typedef struct UwapiTier {
    MoveArray (*GenerateMoves)(TierPosition tier_position);
    TierPosition (*DoMove)(TierPosition tier_position, Move move);
    TierPosition (*FormalPositionToTierPosition)(
        ReadOnlyString formal_position);
    CString (*TierPositionToFormalPosition)(TierPosition position);
    CString (*TierPositionToAutoGuiPosition)(TierPosition position);
    CString (*MoveToFormalMove)(TierPosition position, Move move);
    CString (*MoveToAutoGuiMove)(TierPosition position, Move move);
    TierPosition (*GetInitialTierPosition)(void);
    TierPosition (*GetRandomLegalTierPosition)(void);
} UwapiTier;

#endif  // GAMESMANONE_CORE_TYPES_UWAPI_UWAPI_TIER_H
