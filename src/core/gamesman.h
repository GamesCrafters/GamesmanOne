#ifndef GAMESMANEXPERIMENT_CORE_GAMESMAN_H_
#define GAMESMANEXPERIMENT_CORE_GAMESMAN_H_

#include <stdbool.h>
#include <stdint.h>

#include "core/analysis.h"
#include "core/gamesman_types.h"

/****************************** Global variables ******************************/

/**
 * @brief The maximum expected hash value of the game.
 *
 * @details The game designer is responsible for setting this value to the
 * maximum expected hash value of their game in the game-specific initialization
 * function. The database will allocate an array of [value, remoteness, etc]
 * records based on this number. If this value is set to a value smaller than
 * the actual maximum hash, the database system will complain about an out-of-
 * bounds access and the solver will fail. If this value is set to a value
 * larger than the actual maximum hash, there will be no error but the
 * memory usage will increase during solve and the size of the database
 * will increase.
 *
 * @note REQUIRED by the regular solver and the tier solver. The system will
 * decide which solver to use based on this value. If this value is set to
 * kTierGamesmanGlobalNumberOfPositions (which is an invalid value for regular
 * games), the tier solver will be selected. If this value is set to a positive
 * integer, the regular solver will be selected. If this value is not set by
 * the game specific initialization function, the system will panic.
 */
extern Position global_num_positions;

/**
 * @brief Initial position of the game.
 *
 * @details The game designer is responsible for setting this value to the
 * initial position of the current game variant being initialized. For a tier
 * game, the other global variable global_initial_tier must also be defined at
 * the same time.
 *
 * @note REQUIRED by the regular solver and the tier solver.
 */
extern Position global_initial_position;

/**
 * @brief Tier to which the initial position of the game belongs.
 *
 * @details The game designer is responsible for setting this value to the tier
 * of the initial position of the current game variant being initialized.
 *
 * @note REQUIRED by the tier solver. Ignored by the regular solver.
 */
extern Tier global_initial_tier;

extern Analysis global_analysis;

/***************************** Regular Solver API *****************************/
// If you are implementing a game without tiers, implement the following
// API functions and ignore the Tier Solver API in the next section.

/**
 * @brief Returns an array of available moves at the given position.
 *
 * @details Assumes the given position is legal. Passing an illegal
 * position to this function is undefined behavior.
 *
 * @note REQUIRED by the regular solver. The regular solver will panic if this
 * function is not implemented.
 */
extern MoveArray (*GenerateMoves)(Position position);

/**
 * @brief Returns the value of the given position if it is a primitive
 * position, or kUndecided otherwise.
 *
 * @details Assumes the given position is legal. Passing an illegal
 * position to this function is undefined behavior.
 *
 * @note REQUIRED by the regular solver. The regular solver will panic if this
 * function is not implemented.
 */
extern Value (*Primitive)(Position position);

/**
 * @brief Returns the resulting position after the given move is
 * performed at the given position.
 *
 * @details Assumes the given move is legal. Passing an illegal position
 * or a move illegal to the given position to this function is undefined
 * behavior.
 *
 * @note REQUIRED by the regular solver. The regular solver will panic if this
 * function is not implemented.
 */
extern Position (*DoMove)(Position position, Move move);

/**
 * @brief Returns true if the given position is legal, or false otherwise.
 *
 * @details A position is legal if and only if it is reachable from the
 * initial game position. Assumes the given position is between 0 and
 * global_num_positions - 1. Passing an out-of-bounds position to this
 * function is undefined behavior.
 *
 * @note Optional for the regular solver, but is required by the Undo-Move
 * Optimization. If not implemented, the Undo-Move Optimization will be
 * disabled and a backward graph will be built and stored in memory using
 * depth-first search from the initial game position.
 *
 * @attention If we simply assume that all positions are legal and don't
 * perform a search from the initial position, the solver statistics may
 * become inaccurate, although the value and remoteness of all legal positions
 * will still be correctly solved. The reason is that some positions that seem
 * to be legal may not be reachable from the initial position. Another warning
 * is that if the game designer implements this function but fails to account
 * for the aforementioned illegal positions, the statistics will also be
 * inaccurate.
 *
 * @todo Decide whether or not to make this function optional for undo-move.
 */
extern bool (*IsLegalPosition)(Position position);

/**
 * @brief Returns the number of unique child positions of the given position.
 *
 *
 * @details The word unique is emphasized here because it is possible in some
 * games that making different moves results in the same child position. Assumes
 * the given position is legal. Passing an illegal position to this function is
 * undefined behavior.
 *
 * @note Optional for the regular solver, but can be implemented as an
 * optimization to first generating moves and then doing moves. If not
 * implemented, the system will use a naive version that calls GenerateMoves()
 * and DoMove().
 */
extern int (*GetNumberOfChildPositions)(Position position);

/**
 * @brief Returns an array of child positions (duplicates allowed) at the given
 * position.
 *
 * @details Assumes the given position is legal. Passing an illegal position
 * to this function is undefined behavior.
 *
 * @note Optional for the regular solver, but can be implemented as an
 * optimization to first generating moves and then doing moves. If not
 * implemented, the system will use a naive version that calls GenerateMoves()
 * and DoMove().
 */
extern PositionArray (*GetChildPositions)(Position position);

/**
 * @brief Returns an array of parent positions (duplicates allowed) at the
 * given position.
 *
 * @details Assumes the given position is legal. Passing an illegal position
 * to this function is undefined behavior.
 *
 * @note Optional for the regular solver, but is required by the Undo-Move
 * Optimization. If not implemented, the Undo-Move Optimization will be
 * disabled and a backward graph will be built and stored in memory using
 * depth-first search from the initial game position.
 */
extern PositionArray (*GetParentPositions)(Position position);

/****************************** Tier Solver API ******************************/
// If you are implementing a tier game, implement the following
// API functions and ignore the Regular Solver API in the previous section.

/* * * * * * * * * * * * * * Tier position related * * * * * * * * * * * * * */

/**
 * @brief Returns the size of the given tier.
 *
 * @details The size of a tier is defined to be the maximum hash value within
 * the tier. The database will allocate an array of [value, remoteness, etc]
 * records for the given tier based on the size of the tier. If this function
 * returned a value smaller than the actual maximum hash, the database system
 * will complain about an out-of- bounds access and the solver will fail. If
 * this function returned a value larger than the actual maximum hash, there
 * will be no error but the memory usage will increase during solve and the size
 * of the database will increase. Assumes the given tier is valid. Passing an
 * invalid tier to this function is undefined behavior.
 *
 * @note REQUIRED by the tier solver. The tier solver will panic if this
 * function is not implemented. Used by tier_solver.c in multiple functions.
 */
extern int64_t (*GetTierSize)(Tier tier);

/**
 * @brief Returns an array of available moves at the given tier position.
 *
 * @details Assumes the given tier position is valid. Passing an invalid
 * tier or illegal position within the given tier is undefined behavior.
 *
 * @note REQUIRED by the tier solver. The tier solver will panic if this
 * function is not implemented.
 */
extern MoveArray (*TierGenerateMoves)(Tier tier, Position position);

/**
 * @brief Returns the value of the given tier position if it is a primitive
 * position. Returns kUndecided otherwise.
 *
 * @details Assumes the given tier position is valid. Passing an invalid tier
 * or an illegal position within the given tier is undefined behavior.
 *
 * @note REQUIRED by the tier solver. The tier solver will panic if this
 * function is not implemented. Used by tier_solver.c::Step3ScanTier.
 */
extern Value (*TierPrimitive)(Tier tier, Position position);

/**
 * @brief Returns the resulting tier position after performing the given move
 * at the given tier position.
 *
 * @details Assumes the given tier position is valid and the given move is a
 * valid move at the tier position. Passing an invalid tier, an illegal position
 * within the tier, or an illegal move is undefined behavior.
 *
 * @note REQUIRED by the tier solver. The tier solver will panic if this
 * function is not implemented.
 */
extern TierPosition (*TierDoMove)(Tier tier, Position position, Move move);

/**
 * @brief Returns true if the given tier position is legal, or false otherwise.
 *
 * @details A tier position is legal if and only if it is reachable from the
 * initial game position. Assumes the given tier position is between 0 and
 * GetTierSize(tier) - 1. Passing an out-of-bounds position to this function
 * is undefined behavior.
 *
 * @attention If we simply assume that all positions are legal, the solver
 * statistics may become inaccurate, although the value and remoteness of all
 * legal positions will still be correctly solved. The reason is that some
 * positions that seem to be legal may not be reachable from the initial
 * position. Another warning is that if the game designer implements this
 * function but fails to account for the aforementioned illegal positions, the
 * statistics will also be inaccurate. Since there is no way to scan through
 * the entire game tree like we do in regular solver, the correctness of
 * this function is critical to accurate statistics.
 *
 * @note REQUIRED by the tier solver. The tier solver will panic if this
 * function is not implemented. Used by tier_solver.c::Step3ScanTier.
 *
 * @todo Decide whether or not to make this function required.
 */
extern bool (*TierIsLegalPosition)(Tier tier, Position position);

/**
 * @brief Returns the number of unique child positions of the given tier
 * position.
 *
 * @details The word unique is emphasized here because it is possible in some
 * games that making different moves results in the same child position. Assumes
 * the given tier position is legal. Passing an invalid tier or an illegal
 * position within the tier is undefined behavior.
 *
 * @note Optional for the tier solver, but can be implemented as an optimization
 * to first generating moves and then doing moves. If not implemented, the
 * system will use a naive version that calls TierGenerateMoves() and
 * TierDoMove(). Used by tier_solver.c::Step3ScanTier.
 */
extern int (*TierGetNumberOfChildPositions)(Tier tier, Position position);

/**
 * @brief Returns an array of positions within the given parent_tier that are
 * parents of the given tier position.
 *
 * @details All positions returned by this function are guaranteed to be part
 * of the given parent_tier. Assumes the given parent_tier and the given tier
 * position are valid. Passing an invalid tier, an illegal position within
 * the tier, an invalid parent_tier, or a "parent_tier" that is not a parent
 * of the given tier is undefined behavior.
 *
 * @note Optional for the tier solver, but is required for the Undo-Move
 * Optimization. If not implemented, the Undo-Move Optimization will be
 * disabled and a backward graph for each tier will be built and stored
 * in memory by calling TierDoMove() on all legal positions in the tier
 * that is being solved. Used by tier_solver.c::ProcessLoseOrTiePosition
 * and tier_solver.c::ProcessWinPosition.
 */
extern PositionArray (*TierGetParentPositions)(Tier tier, Position position,
                                               Tier parent_tier);

/**
 * @brief Returns the position in the given noncanonical_tier that is
 * symmetrical to the given position in the given canonical_tier.
 *
 * @details Assumes the given canonical_tier, position, and noncanonical_tier
 * are all valid. Furthermore, assumes that the given noncanonical_tier is
 * symmetrical to the given canonical_tier. That is, calling
 * GetCanonicalTier(noncanonical_tier) returns canonical_tier.
 *
 * @note Optional for the tier solver, but is required for the Tier Symmetry
 * Removal Optimization. If not implemented, the Optimization will be disabled
 * and all tiers will be treated as canonical. Used by
 * tier_solver.c::Step1_1LoadNonCanonical.
 */
extern Position (*TierGetNonCanonicalPosition)(Tier canonical_tier,
                                               Position position,
                                               Tier noncanonical_tier);

/* * * * * * * * * * * * * * * Tier tree related * * * * * * * * * * * * * * */

/**
 * @brief Returns an array of child tiers of the given tier.
 *
 * @details A child tier of a tier is one that has at least one position that
 * can be reached by performing one move from a position within its parent tier.
 * Assumes the given tier is valid. Passing an invalid tier is undefined
 * behavior.
 *
 * @note REQUIRED by the tier solver. The tier solver will panic if this
 * function is not implemented. Used by solver.c::CreateTierTreeProcessChildren.
 */
extern TierArray (*GetChildTiers)(Tier tier);

/**
 * @brief Returns an array of parent tiers of the given tier.
 *
 * @details A parent tier of a tier is one that has at least one position from
 * which a position within the child tier can be reached by making one move.
 * Assumes the given tier is valid. Passing an invalid tier is undefined
 * behavior.
 *
 * @note Currently REQUIRED by the tier solver. The tier solver will panic if
 * this function is not implemented. Used by solver.c::UpdateTierTree.
 *
 * @todo Decide whether or not to make this function required for tier solver.
 * A backward tier graph can be constructed while we perform topological sort
 * on the tier graph.
 */
extern TierArray (*GetParentTiers)(Tier tier);

/**
 * @brief Returns true if the given tier is canonical, or false otherwise.
 *
 * @details By convention, a canonical tier is one with the smallest tier value
 * in a set of symmetrical tiers. For each tier[i] within the set including
 * the canonical tier itself, calling GetCanonicalTier(tier[i]) returns the
 * canonical tier. Assumes the given tier is valid. Passing an invalid tier
 * to this function is undefined behavior.
 *
 * @note Optional for the tier solver, but is required for the Tier Symmetry
 * Removal Optimization. If not implemented, the Optimization will be disabled
 * and all tiers will be treated as canonical. Used by solver.c::SolveTierTree.
 */
extern bool (*IsCanonicalTier)(Tier tier);

/**
 * @brief Returns the canonical tier that is symmetrical to the given tier.
 * Returns the given tier if itself is canonical.
 *
 * @details By convention, a canonical tier is one with the smallest tier value
 * in a set of symmetrical tiers. For each tier[i] within the set including
 * the canonical tier itself, calling GetCanonicalTier(tier[i]) returns the
 * canonical tier. Assumes the given tier is valid. Passing an invalid tier
 * to this function is undefined behavior.
 *
 * @note Optional for the tier solver, but is required for the Tier Symmetry
 * Removal Optimization. If not implemented, the Optimization will be disabled
 * and all tiers will be treated as canonical. Used by solver.c::UpdateTierTree.
 */
extern Tier (*GetCanonicalTier)(Tier tier);

/******************************* Default Values *******************************/

extern const Position kDefaultGlobalNumberOfPositions;
extern const Position kTierGamesmanGlobalNumberOfPositions;
extern const Position kDefaultInitialPosition;
extern const Tier kDefaultInitialTier;

/***************************** Default Functions *****************************/

int GamesmanGetNumberOfChildPositions(Position position);
PositionArray GamesmanGetChildPositions(Position position);

int64_t GamesmanGetTierSizeConverted(Tier tier);
MoveArray GamesmanTierGenerateMovesConverted(Tier tier, Position position);
Value GamesmanTierPrimitiveConverted(Tier tier, Position position);
TierPosition GamesmanTierDoMoveConverted(Tier tier, Position position,
                                         Move move);
bool GamesmanTierIsLegalPositionConverted(Tier tier, Position position);
int GamesmanTierGetNumberOfChildPositionsConverted(Tier tier,
                                                   Position position);
PositionArray GamesmanTierGetParentPositionsConverted(Tier tier,
                                                      Position position,
                                                      Tier parent_tier);

TierArray GamesmanGetChildTiersConverted(Tier tier);
TierArray GamesmanGetParentTiersConverted(Tier tier);
bool GamesmanIsCanonicalTierConverted(Tier tier);
Tier GamesmanGetCanonicalTierConverted(Tier tier);

#endif  // GAMESMANEXPERIMENT_CORE_GAMESMAN_H_
