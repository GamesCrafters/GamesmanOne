#ifndef GAMESMANEXPERIMENT_CORE_INTERACTIVE_GAMES_PRESOLVE_MATCH_H_
#define GAMESMANEXPERIMENT_CORE_INTERACTIVE_GAMES_PRESOLVE_MATCH_H_

#include <stdbool.h>

#include "core/gamesman_types.h"

typedef enum InteractiveMatchSetGameErrorCode {
    kInteractiveMatchSetGameOk = 0,
    kInteractiveMatchSetGameBasicApiIncomplete,
    kInteractiveMatchSetGameRegularOrTierApiIncomplete,
} InteractiveMatchSetGameErrorCode;

int InteractiveMatchSetGame(const Game *game);
const Game *InteractiveMatchGetCurrentGame(void);

bool InteractiveMatchRestart(void);
void InteractiveMatchTogglePlayerType(int player);
bool InteractiveMatchPlayerIsComputer(int player);

int InteractiveMatchGetCurrentVariant(void);
TierPosition InteractiveMatchGetCurrentPosition(void);
int InteractiveMatchGetTurn(void);
MoveArray InteractiveMatchGenerateMoves(void);
TierPosition InteractiveMatchDoMove(TierPosition tier_position, Move move);
bool InteractiveMatchCommitMove(Move move);
Value InteractiveMatchPrimitive(void);
bool InteractiveMatchUndo(void);
int InteractiveMatchPositionToString(TierPosition tier_position, char *buffer);

#endif  // GAMESMANEXPERIMENT_CORE_INTERACTIVE_GAMES_PRESOLVE_MATCH_H_
