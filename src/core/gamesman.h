#ifndef GAMESMANEXPERIMENT_CORE_GAMESMAN_H_
#define GAMESMANEXPERIMENT_CORE_GAMESMAN_H_

#include <stdbool.h>
#include <stdint.h>

#include "gamesman_types.h"

/* Global variables. */
extern Position global_num_positions;
extern Position global_initial_position;
extern Tier global_initial_tier;

/* Regular, no tier */
extern MoveList (*GenerateMoves)(Position position);
extern Value (*Primitive)(Position position);
extern Position (*DoMove)(Position position, Move move);
extern int (*GetNumberOfChildPositions)(Position position);
extern PositionArray (*GetChildPositions)(Position position);
extern PositionArray (*GetParentPositions)(Position position);

/* Tier tree stuff */
extern TierArray (*GetChildTiers)(Tier tier);
extern TierArray (*GetParentTiers)(Tier tier);
extern bool (*IsCanonicalTier)(Tier tier);
extern Tier (*GetCanonicalTier)(Tier tier);

/* Tier position stuff */
extern int64_t (*GetTierSize)(Tier tier); //
extern MoveList (*TierGenerateMoves)(Tier tier, Position position);
extern Value (*TierPrimitive)(Tier tier, Position position);
extern TierPosition (*TierDoMove)(Tier tier, Position position, Move move);
extern int (*TierGetNumberOfChildPositions)(Tier tier, Position position); //
extern PositionArray (*TierGetParentPositions)(Tier tier, Position position, Tier parent_tier); //

const Position kDefaultGlobalNumberOfPositions = -1;
const Position kTierGamesmanGlobalNumberOfPositions = 0;
const Position kDefaultInitialPosition = -1;
const Tier kDefaultInitialTier = -1;

// ---------------------------------------------------------------
/**
 * Mandatory API
 * System will panic if any of these function pointers are
 * set to NULL after a game has been initialized.
 */

/**
 * Derived API
 * These functions are optional and can be derived from the
 * above mandatory API. If a derived API function is set to
 * NULL after the game has been initialized, a generic
 * version of it will be used.
 */

/* API required to enable undo-moves for solving. */

/**
 * Mandatory Tier API
 * The following functions are required for tier solving.
 */

/* Derived Tier API */

/* Generic Derived API functions. */

// ---------------------------------------------------------------

int GamesmanGetNumberOfChildPositions(Position position);
PositionArray GamesmanGetChildPositions(Position position);
int64_t GamesmanGetTierSize(Tier tier);
MoveList GamesmanTierGenerateMoves(Tier tier, Position position);
Value GamesmanTierPrimitive(Tier tier, Position position);
TierPosition GamesmanTierDoMove(Tier tier, Position position, Move move);
int GamesmanTierGetNumberOfChildPositions(Tier tier, Position position);
PositionArray GamesmanTierGetParentPositions(Tier tier, Position position, Tier parent_tier);

#endif  // GAMESMANEXPERIMENT_CORE_GAMESMAN_H_
