#ifndef GAMESMANEXPERIMENT_CORE_INTERACTIVE_GAMES_PRESOLVE_MATCH_H_
#define GAMESMANEXPERIMENT_CORE_INTERACTIVE_GAMES_PRESOLVE_MATCH_H_

#include <stdbool.h>

#include "core/gamesman_types.h"

const Game *InteractiveMatchGetCurrentGame(void);
void InteractiveMatchRestart(const Game *game);
void InteractiveMatchTogglePlayerType(int player);
bool InteractiveMatchPlayerIsComputer(int player);

TierPosition InteractiveMatchGetCurrentPosition(void);
int InteractiveMatchGetTurn(void);
bool InteractiveMatchDoMove(Move move);
bool InteractiveMatchUndo(void);

#endif  // GAMESMANEXPERIMENT_CORE_INTERACTIVE_GAMES_PRESOLVE_MATCH_H_
