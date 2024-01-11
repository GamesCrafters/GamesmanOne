#ifndef GAMESMANONE_CORE_TYPES_UWAPI_UWAPI_TIER_H
#define GAMESMANONE_CORE_TYPES_UWAPI_UWAPI_TIER_H

#include <stdbool.h>  // bool

#include "core/data_structures/cstring.h"
#include "core/types/base.h"

typedef struct UwapiTier {
    MoveArray (*GenerateMoves)(TierPosition tier_position);
    TierPosition (*DoMove)(TierPosition tier_position, Move move);
    bool (*IsLegalFormalPosition)(ReadOnlyString formal_position);
    TierPosition (*FormalPositionToTierPosition)(
        ReadOnlyString formal_position);
    CString (*TierPositionToFormalPosition)(TierPosition tier_position);
    CString (*TierPositionToAutoGuiPosition)(TierPosition tier_position);
    CString (*MoveToFormalMove)(TierPosition tier_position, Move move);
    CString (*MoveToAutoGuiMove)(TierPosition tier_position, Move move);
    Tier (*GetInitialTier)(void);
    Position (*GetInitialPosition)(void);
    TierPosition (*GetRandomLegalTierPosition)(void);
} UwapiTier;

#endif  // GAMESMANONE_CORE_TYPES_UWAPI_UWAPI_TIER_H
