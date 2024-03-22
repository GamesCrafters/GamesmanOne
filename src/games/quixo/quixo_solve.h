#ifndef GAMESMANONE_GAMES_QUIXO_QUIXO_SOLVE_H_
#define GAMESMANONE_GAMES_QUIXO_QUIXO_SOLVE_H_

#include "core/types/gamesman_types.h"

Tier QuixoGetInitialTier(void);
Position QuixoGetInitialPosition(void);

int64_t QuixoGetTierSize(Tier tier);
MoveArray QuixoGenerateMoves(TierPosition tier_position);
Value QuixoPrimitive(TierPosition tier_position);
TierPosition QuixoDoMove(TierPosition tier_position, Move move);
bool QuixoIsLegalPosition(TierPosition tier_position);
Position QuixoGetCanonicalPosition(TierPosition tier_position);
PositionArray QuixoGetCanonicalParentPositions(TierPosition tier_position,
                                               Tier parent_tier);
Position QuixoGetPositionInSymmetricTier(TierPosition tier_position,
                                         Tier symmetric);
TierArray QuixoGetChildTiers(Tier tier);
Tier QuixoGetCanonicalTier(Tier tier);
int QuixoGetTierName(char *name, Tier tier);

#endif  // GAMESMANONE_GAMES_QUIXO_QUIXO_SOLVE_H_
