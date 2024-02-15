/**
 * @file regular_solver.h
 * @author Robert Shi (robertyishi@berkeley.edu)
 *         GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief The Regular Solver API.
 * @version 1.3.0
 * @date 2024-02-15
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

#ifndef GAMESMANONE_CORE_SOLVERS_REGULAR_SOLVER_REGULAR_SOLVER_H_
#define GAMESMANONE_CORE_SOLVERS_REGULAR_SOLVER_REGULAR_SOLVER_H_

#include <stdbool.h>  // bool
#include <stdint.h>   // int64_t

#include "core/types/gamesman_types.h"

/** @brief The Regular Solver. */
extern const Solver kRegularSolver;

/**
 * @brief Regular Solver API.
 *
 * @note The functions may behave differently under different game variants,
 * even though the function pointers are constant.
 */
typedef struct RegularSolverApi {
    /**
     * @brief Returns the total number of positions in the current game variant.
     *
     * @details The value returned by this function is typically equal to (the
     * maximum expected hash value + 1). The database will allocate an array of
     * records for each position of this size. If this function returns a
     * value smaller than the actual size, the database system will, at
     * some point, complain about an out-of-bounds array access and the solver
     * will fail. If this function returns a value larger than the actual size,
     * there will be no error but more memory will be used and the size of the
     * database may increase.
     *
     * @note This function is REQUIRED. The solver system will panic if this
     * function is not implemented.
     */
    int64_t (*GetNumPositions)(void);

    /**
     * @brief Returns the initial position of the current game variant.
     *
     * @note This function is REQUIRED. The solver system will panic if this
     * function is not implemented.
     */
    Position (*GetInitialPosition)(void);

    /**
     * @brief Returns an array of available moves at POSITION.
     *
     * @details Assumes POSITION is legal. Results in undefined behavior
     * otherwise.
     *
     * @note This function is REQUIRED. The solver system will panic if this
     * function is not implemented.
     */
    MoveArray (*GenerateMoves)(Position position);

    /**
     * @brief Returns the value of POSITION if POSITION is primitive. Returns
     * kUndecided otherwise.
     *
     * @note Assumes POSITION is valid. Results in undefined behavior otherwise.
     *
     * @note This function is REQUIRED. The solver system will panic if this
     * function is not implemented.
     */
    Value (*Primitive)(Position position);

    /**
     * @brief Returns the resulting position after performing MOVE at POSITION.
     *
     * @note Assumes POSITION is valid and MOVE is a valid move at POSITION.
     * Passing an illegal POSITION or an illegal move at POSITION results in
     * undefined behavior.
     *
     * @note This function is REQUIRED. The solver system will panic if this
     * function is not implemented.
     */
    Position (*DoMove)(Position position, Move move);

    /**
     * @brief Returns false if POSITION is definitely illegal. Returns true
     * if POSITION is legal or if its legality cannot be easily determined
     * by simple observation.
     *
     * @details A position is legal if and only if it is reachable from the
     * initial position. Note that this function is for speed optimization
     * only. It is not intended for statistical purposes. Even if this function
     * reports that POSITION is legal, that position might in fact be
     * unreachable from the initial position. However, if this function reports
     * that POSITION is illegal, then it is definitely not reachable from the
     * inital position.
     *
     * @note Assumes POSITION is between 0 and GetNumPositions(). Passing an
     * out-of-bounds POSITION results in undefined behavior.
     *
     * @note This function is REQUIRED. The solver system will panic if this
     * function is not implemented.
     */
    bool (*IsLegalPosition)(Position position);

    /**
     * @brief Returns the canonical position that is symmetric to POSITION.
     *
     * @details By convention, a canonical position is one with the smallest
     * hash value in a set of symmetrical positions. For each position[i] within
     * the set (including the canonical position itself), calling
     * GetCanonicalPosition() on position[i] returns the canonical position.
     *
     * @note Assumes POSITION is legal. Results in undefined behavior otherwise.
     *
     * @note This function is OPTIONAL, but is required for the Position
     * Symmetry Removal Optimization. If not implemented, the Optimization will
     * be disabled.
     */
    Position (*GetCanonicalPosition)(Position position);

    /**
     * @brief Returns the number of unique canonical child positions of
     * POSITION. For games that do not support the Position Symmetry
     * Removal Optimization, all unique child positions are included.
     *
     * @details The word unique is emphasized here because it is possible, in
     * some games, that making different moves results in the same canonical
     * child position.
     *
     * @note Assumes POSITION is legal. Results in undefined behavior otherwise.
     *
     * @note This function is OPTIONAL, but can be implemented as an
     * optimization to first generating moves and then doing moves. If not
     * implemented, the system will replace calls to this function with calls to
     * GenerateMoves(), DoMove(), and GetCanonicalPosition().
     */
    int (*GetNumberOfCanonicalChildPositions)(Position position);

    /**
     * @brief Returns an array of unique canonical child positions of
     * POSITION. For games that do not support the Position Symmetry
     * Removal Optimization, all unique child positions are included.
     *
     * @details The word unique is emphasized here because it is possible, in
     * some games, that making different moves results in the same canonical
     * child position.
     *
     * @note Assumes POSITION is legal. Results in undefined behavior otherwise.
     *
     * @note This function is OPTIONAL, but can be implemented as an
     * optimization to first generating moves and then doing moves. If not
     * implemented, the system will replace calls to this function with calls to
     * GenerateMoves(), DoMove(), and GetCanonicalPosition().
     */
    PositionArray (*GetCanonicalChildPositions)(Position position);

    /**
     * @brief Returns an array of unique canonical parent positions of POSITION.
     * For games that do not support the Position Symmetry Removal
     * Optimization, all unique parent positions are included.
     *
     * @details The word unique is emphasized here because it is possible in
     * some games that a child position has two parent positions that are
     * symmetric to each other.
     *
     * @note Assumes POSITION is legal. Results in undefined behavior otherwise.
     *
     * @note This function is OPTIONAL, but is required for Retrograde Analysis.
     * If not implemented, Retrograde Analysis will be disabled and a reverse
     * graph for the current game variant will be built and stored in memory by
     * calling DoMove() on all legal positions.
     */
    PositionArray (*GetCanonicalParentPositions)(Position position);
} RegularSolverApi;

/** @brief Solver options of the Regular Solver. */
typedef struct RegularSolverSolveOptions {
    int verbose; /**< Level of details to output. */
    bool force;  /**< Whether to force (re)solve the game. */
} RegularSolverSolveOptions;

/** @brief Analyzer options of the Regular Solver. */
typedef struct RegularSolverAnalyzeOptions {
    int verbose; /**< Level of details to output. */
    bool force;  /**< Whether to force (re)analyze the game. */
} RegularSolverAnalyzeOptions;

#endif  // GAMESMANONE_CORE_SOLVERS_REGULAR_SOLVER_REGULAR_SOLVER_H_
