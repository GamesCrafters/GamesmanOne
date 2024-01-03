/**
 * @file tier_solver.h
 * @author Max Delgadillo: designed and implemented the original version
 * of tier solver (solveretrograde.c in GamesmanClassic.)
 * @author Robert Shi (robertyishi@berkeley.edu): Modularized, multithreaded
 * tier solver with various optimizations.
 *         GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Declaration of the Tier Solver API.
 *
 * @version 1.11
 * @date 2023-10-22
 *
 * @copyright This file is part of GAMESMAN, The Finite, Two-person
 * Perfect-Information Game Generator released under the GPL:
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef GAMESMANONE_CORE_SOLVERS_TIER_SOLVER_TIER_SOLVER_H_
#define GAMESMANONE_CORE_SOLVERS_TIER_SOLVER_TIER_SOLVER_H_

#include <stdbool.h>  // bool
#include <stdint.h>   // int64_t

#include "core/gamesman_types.h"

/** @brief The Tier Solver. */
extern const Solver kTierSolver;

/**
 * @brief Tier Solver API.
 *
 * @note The functions may behave differently under different game variants,
 * even though the function pointers are constant.
 */
typedef struct TierSolverApi {
    /**
     * @brief Returns the initial tier of the current game variant.
     *
     * @note This function is REQUIRED. The solver system will panic if this
     * function is not implemented.
     */
    Tier (*GetInitialTier)(void);

    /**
     * @brief Returns the initial position (within the initial tier) of the
     * current game variant.
     *
     * @note This function is REQUIRED. The solver system will panic if this
     * function is not implemented.
     */
    Position (*GetInitialPosition)(void);

    /**
     * @brief Returns the number of positions in TIER.
     *
     * @details The size of a tier is defined as (the maximum hash value + 1)
     * within the tier. The database will allocate an array of records for each
     * position within the given tier of this size. If this function returns a
     * value smaller than the actual size, the database system will, at some
     * point, complain about an out-of-bounds array access and the solver will
     * fail. If this function returns a value larger than the actual size, there
     * will be no error but more memory will be used and the size of the
     * database may increase.
     *
     * @note Assumes TIER is a valid tier reachable from the initial tier.
     * Passing an illegal tier results in undefined behavior.
     *
     * @note This function is REQUIRED. The solver system will panic if this
     * function is not implemented.
     */
    int64_t (*GetTierSize)(Tier tier);

    /**
     * @brief Returns an array of moves available at TIER_POSITION.
     *
     * @note Assumes TIER_POSITION is valid. Passing an invalid tier or illegal
     * position within the tier results in undefined behavior.
     *
     * @note This function is REQUIRED. The solver system will panic if this
     * function is not implemented.
     */
    MoveArray (*GenerateMoves)(TierPosition tier_position);

    /**
     * @brief Returns the value of TIER_POSITION if TIER_POSITION is primitive.
     * Returns kUndecided otherwise.
     *
     * @note Assumes TIER_POSITION is valid. Passing an invalid tier or an
     * illegal position within the tier results in undefined behavior.
     *
     * @note This function is REQUIRED. The solver system will panic if this
     * function is not implemented.
     */
    Value (*Primitive)(TierPosition tier_position);

    /**
     * @brief Returns the resulting tier position after performing MOVE at
     * TIER_POSITION.
     *
     * @note Assumes TIER_POSITION is valid and MOVE is a valid move at
     * TIER_POSITION. Passing an invalid tier, an illegal position within the
     * tier, or an illegal move results in undefined behavior.
     *
     * @note This function is REQUIRED. The solver system will panic if this
     * function is not implemented.
     */
    TierPosition (*DoMove)(TierPosition tier_position, Move move);

    /**
     * @brief Returns false if TIER_POSITION is definitely illegal. Returns true
     * if TIER_POSITION is considered legal by all other API functions.
     *
     * @details This function is for speed optimization only. It is not intended
     * for statistical purposes. Even if this function reports that
     * TIER_POSITION is legal, that position might in fact be unreachable from
     * the initial tier position. However, if this function reports that
     * TIER_POSITION is illegal, then TIER_POSITION is definitely not reachable
     * from the inital tier position. Furthermore, it is guaranteed that calling
     * all other API functions such as GenerateMoves() and DoMove() on a legal
     * TIER_POSITION results in well-defined behavior.
     *
     * @note Assumes TIER_POSITION.position is between 0 and
     * GetTierSize(TIER_POSITION.tier) - 1. Passing an out-of-bounds position
     * results in undefined behavior.
     *
     * @note This function is REQUIRED. The solver system will panic if this
     * function is not implemented.
     */
    bool (*IsLegalPosition)(TierPosition tier_position);

    /**
     * @brief Returns the canonical position that is symmetric to TIER_POSITION
     * within the same tier.
     *
     * @details GAMESMAN currently does not support position symmetry removal
     * across tiers. By convention, a canonical position is one with the
     * smallest hash value in a set of symmetrical positions that all belong to
     * the same tier. For each position[i] within the set (including the
     * canonical position itself), calling GetCanonicalPosition() on position[i]
     * returns the canonical position.
     *
     * @note Assumes TIER_POSITION is legal. Passing an invalid tier, or an
     * illegal position within the tier results in undefined behavior.
     *
     * @note This function is OPTIONAL, but is required for the Position
     * Symmetry Removal Optimization. If not implemented, the Optimization will
     * be disabled.
     */
    Position (*GetCanonicalPosition)(TierPosition tier_position);

    /**
     * @brief Returns the number of unique canonical child positions of
     * TIER_POSITION. For games that do not support the Position Symmetry
     * Removal Optimization, all unique child positions are included.
     *
     * @details The word unique is emphasized here because it is possible, in
     * some games, that making different moves results in the same canonical
     * child position.
     *
     * @note Assumes TIER_POSITION is legal. Passing an invalid tier or an
     * illegal position within the tier results in undefined behavior.
     *
     * @note This function is OPTIONAL, but can be implemented as an
     * optimization to first generating moves and then doing moves. If not
     * implemented, the system will replace calls to this function with calls to
     * GenerateMoves(), DoMove(), and GetCanonicalPosition().
     */
    int (*GetNumberOfCanonicalChildPositions)(TierPosition tier_position);

    /**
     * @brief Returns an array of unique canonical child positions of
     * TIER_POSITION. For games that do not support the Position Symmetry
     * Removal Optimization, all unique child positions are included.
     *
     * @details The word unique is emphasized here because it is possible, in
     * some games, that making different moves results in the same canonical
     * child position.
     *
     * @note Assumes TIER_POSITION is legal. Passing an invalid tier or an
     * illegal position within the tier results in undefined behavior.
     *
     * @note This function is OPTIONAL, but can be implemented as an
     * optimization to first generating moves and then doing moves. If not
     * implemented, the system will replace calls to this function with calls to
     * GenerateMoves(), DoMove(), and GetCanonicalPosition().
     */
    TierPositionArray (*GetCanonicalChildPositions)(TierPosition tier_position);

    /**
     * @brief Returns an array of unique canonical parent positions of CHILD.
     * Furthermore, these parent positions are restricted to be within
     * PARENT_TIER. For games that do not support the Position Symmetry Removal
     * Optimization, all unique parent positions within PARENT_TIER are
     * included.
     *
     * @details The word unique is emphasized here because it is possible in
     * some games that a child position has two parent positions that are
     * symmetric to each other.
     *
     * @note Assumes PARENT_TIER is a valid tier and CHILD is a valid tier
     * position. Passing an invalid CHILD, an invalid PARENT_TIER, or a
     * PARENT_TIER that is not actually a parent of the given CHILD results in
     * undefined behavior.
     *
     * @note This function is OPTIONAL, but is required for Retrograde Analysis.
     * If not implemented, Retrograde Analysis will be disabled and a reverse
     * graph for the current solving tier and its child tiers will be built and
     * stored in memory by calling DoMove() on all legal positions.
     */
    PositionArray (*GetCanonicalParentPositions)(TierPosition child,
                                                 Tier parent_tier);

    /**
     * @brief Returns the position symmetric to TIER_POSITION within the given
     * SYMMETRIC tier.
     *
     * @note Assumes TIER_POSITION is a valid tier position and SYMMETRIC is a
     * valid tier. Also assumes the SYMMETRIC tier is symmetric to
     * TIER_POSITION.tier. That is, calling GetCanonicalTier(TIER_POSITION.tier)
     * and calling GetCanonicalTier(SYMMETRIC) returns the same canonical tier.
     * Results in undefined behavior if either assumption is false.
     *
     * @note This function is OPTIONAL, but is required for the Tier Symmetry
     * Removal Optimization. If not implemented, the Optimization will be
     * disabled and all tiers will be treated as canonical.
     */
    Position (*GetPositionInSymmetricTier)(TierPosition tier_position,
                                           Tier symmetric);

    /**
     * @brief Returns an array of child tiers of the given TIER.
     *
     * @details A child tier is a tier that has at least one position that can
     * be reached by performing a single move from a position within its parent
     * tier.
     *
     * @note Assumes TIER is valid. Results in undefined behavior otherwise.
     *
     * @note This function is REQUIRED. The solver system will panic if this
     * function is not implemented.
     */
    TierArray (*GetChildTiers)(Tier tier);

    /**
     * @brief Returns an array of parent tiers of the given TIER.
     *
     * @details A parent tier is a tier that has at least one position from
     * which a position within the child tier can be reached by making a single
     * move.
     *
     * @note Assumes TIER is valid. Results in undefined behavior otherwise.
     *
     * @note This function is REQUIRED. The solver system will panic if this
     * function is not implemented.
     *
     * @todo Decide whether to make this function optional.. A reverse tier
     * graph can be constructed while we perform topological sort on the tier
     * graph, but will increase memory usage.
     */
    TierArray (*GetParentTiers)(Tier tier);

    /**
     * @brief Returns the canonical tier symmetric to the given TIER. Returns
     * TIER if itself is canonical.
     *
     * @details By convention, a canonical tier is one with the smallest tier
     * value in a set of symmetrical tiers. For each tier[i] within the set
     * including the canonical tier itself, calling GetCanonicalTier(tier[i])
     * returns the canonical tier.
     *
     * @note Assumes TIER is valid. Results in undefined behavior otherwise.
     *
     * @note This function is OPTIONAL, but is required for the Tier Symmetry
     * Removal Optimization. If not implemented, the Optimization will be
     * disabled and all tiers will be treated as canonical.
     */
    Tier (*GetCanonicalTier)(Tier tier);
} TierSolverApi;

typedef struct TierSolverSolveOptions {
    int verbose;
    bool force;
} TierSolverSolveOptions;

typedef struct TierSolverAnalyzeOptions {
    int verbose;
    bool force;
} TierSolverAnalyzeOptions;

#endif  // GAMESMANONE_CORE_SOLVERS_TIER_SOLVER_TIER_SOLVER_H_
