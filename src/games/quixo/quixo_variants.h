#ifndef GAMESMANONE_GAMES_QUIXO_QUIXO_VARIANTS_H_
#define GAMESMANONE_GAMES_QUIXO_QUIXO_VARIANTS_H_

#include "core/types/gamesman_types.h"

enum QuixoConstants {
    kBoardRowsMax = 6,
    kBoardColsMax = 6,
    kBoardSizeMax = kBoardRowsMax * kBoardColsMax,
};

Tier QuixoCurrentVariantGetInitialTier(void);
Position QuixoCurrentVariantGetInitialPosition(void);

int QuixoCurrentVariantGetBoardRows(void);
int QuixoCurrentVariantGetBoardCols(void);
int QuixoCurrentVariantGetKInARow(void);

#endif  // GAMESMANONE_GAMES_QUIXO_QUIXO_VARIANTS_H_
