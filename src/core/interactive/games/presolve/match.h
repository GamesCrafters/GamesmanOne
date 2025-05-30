#ifndef GAMESMANONE_CORE_INTERACTIVE_GAMES_PRESOLVE_MATCH_H_
#define GAMESMANONE_CORE_INTERACTIVE_GAMES_PRESOLVE_MATCH_H_

#include <stdbool.h>  // bool

#include "core/types/gamesman_types.h"

int InteractiveMatchSetGame(const Game *game);
const Game *InteractiveMatchGetCurrentGame(void);

bool InteractiveMatchRestart(void);
void InteractiveMatchTogglePlayerType(int player);
bool InteractiveMatchPlayerIsComputer(int player);

const GameVariant *InteractiveMatchGetVariant(void);
int InteractiveMatchGetVariantIndex(void);
int InteractiveMatchSetVariantOption(int option, int selection);

TierPosition InteractiveMatchGetCurrentPosition(void);
int InteractiveMatchGetTurn(void);
MoveArray InteractiveMatchGenerateMoves(void);
TierPosition InteractiveMatchDoMove(TierPosition tier_position, Move move);
bool InteractiveMatchCommitMove(Move move);
Value InteractiveMatchPrimitive(void);
int InteractiveMatchUndo(void);
int InteractiveMatchPositionToString(TierPosition tier_position, char *buffer);

void InteractiveMatchSetSolved(bool solved);
bool InteractiveMatchSolved(void);

#endif  // GAMESMANONE_CORE_INTERACTIVE_GAMES_PRESOLVE_MATCH_H_
