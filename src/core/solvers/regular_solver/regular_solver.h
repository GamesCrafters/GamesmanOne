/**
 * @file regular_solver.h
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief The Regular Solver API.
 * @version 2.2.0
 * @date 2025-05-11
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
#include <stdint.h>   // int64_t, intptr_t

#include "core/types/gamesman_types.h"

/** @brief The Regular Solver. */
extern const Solver kRegularSolver;

typedef enum {
    /**
     * @brief A single-tier game is of this type if it is loop-free (no cycles
     * in the position graph.)
     */
    kSingleTierGameTypeLoopFree,

    /**
     * @brief A single-tier game is of this type if it is loopy, or if its
     * loopiness is unclear.
     *
     * @note The loopy algorithm also works for loop-free games. Hence, this is
     * the default type of a game if its type is not specified.
     */
    kSingleTierGameTypeLoopy,
} SingleTierGameType;

typedef enum {
    kRegularSolverNumMovesMax = 1024,
    kRegularSolverNumChildPositionsMax = kRegularSolverNumMovesMax,
    kRegularSolverNumParentPositionsMax = kRegularSolverNumMovesMax,
} RegularSolverConstants;

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
     * there will be no error but memory usage and the size of the database may
     * increase.
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
     * @brief Stores the array of moves available at \p position to
     * \p moves and returns the size of the array.
     *
     * @details Assumes \p position is legal. Results in undefined behavior
     * otherwise.
     *
     * @note To game developers: the current version of Regular Solver does not
     * support more than \c kRegularSolverNumMovesMax moves to be generated. If
     * you think your game may exceed this limit, please contact the author of
     * this API.
     *
     * @note This function is REQUIRED. The solver system will panic if this
     * function is not implemented.
     */
    int (*GenerateMoves)(Position position,
                         Move moves[static kRegularSolverNumMovesMax]);

    /**
     * @brief Returns the value of \p position if \p position is primitive.
     * Returns kUndecided otherwise.
     *
     * @note Assumes \p position is valid. Results in undefined behavior
     * otherwise.
     *
     * @note This function is REQUIRED. The solver system will panic if this
     * function is not implemented.
     */
    Value (*Primitive)(Position position);

    /**
     * @brief Returns the resulting position after performing MOVE at \p
     * position.
     *
     * @note Assumes \p position is valid and MOVE is a valid move at \p
     * position. Passing an illegal \p position or an illegal move at \p
     * position results in undefined behavior.
     *
     * @note This function is REQUIRED. The solver system will panic if this
     * function is not implemented.
     */
    Position (*DoMove)(Position position, Move move);

    /**
     * @brief Returns false if \p position is definitely illegal. Returns true
     * if \p position is legal or if its legality cannot be easily determined
     * by simple observation.
     *
     * @details A position is legal if and only if it is reachable from the
     * initial position. Note that this function is for speed optimization
     * only. It is not intended for statistical purposes. Even if this function
     * reports that \p position is legal, that position might in fact be
     * unreachable from the initial position. However, if this function reports
     * that \p position is illegal, then it is definitely not reachable from the
     * inital position.
     *
     * @note Assumes \p position is between 0 and GetNumPositions(). Passing an
     * out-of-bounds \p position results in undefined behavior.
     *
     * @note This function is REQUIRED. The solver system will panic if this
     * function is not implemented.
     */
    bool (*IsLegalPosition)(Position position);

    /**
     * @brief Returns the canonical position that is symmetric to \p position.
     *
     * @details By convention, a canonical position is one with the smallest
     * hash value in a set of symmetrical positions. For each position[i] within
     * the set (including the canonical position itself), calling
     * GetCanonicalPosition() on position[i] returns the canonical position.
     *
     * @note Assumes \p position is legal. Results in undefined behavior
     * otherwise.
     *
     * @note This function is OPTIONAL, but is required for the Position
     * Symmetry Removal Optimization. If not implemented, the Optimization will
     * be disabled.
     */
    Position (*GetCanonicalPosition)(Position position);

    /**
     * @brief Returns the number of unique canonical child positions of
     * \p position. For games that do not support the Position Symmetry
     * Removal Optimization, all unique child positions are included.
     *
     * @details The word unique is emphasized here because it is possible, in
     * some games, that making different moves results in the same canonical
     * child position.
     *
     * @note Assumes \p position is legal. Results in undefined behavior
     * otherwise.
     *
     * @note This function is OPTIONAL, but can be implemented as an
     * optimization to first generating moves and then doing moves. If not
     * implemented, the system will replace calls to this function with calls to
     * GenerateMoves(), DoMove(), and GetCanonicalPosition().
     */
    int (*GetNumberOfCanonicalChildPositions)(Position position);

    /**
     * @brief Stores an array of unique canonical child positions of
     * \p position into \p children and returns the size of the array. For
     * games that do not support the Position Symmetry Removal Optimization, all
     * unique child positions are included.
     *
     * @details The word unique is emphasized here because it is possible, in
     * some games, that making different moves results in the same canonical
     * child position.
     *
     * @note To game developers: the current version of Regular Solver does not
     * support more than \c kRegularSolverNumChildPositionsMax child positions
     * to be generated. If you think your game may exceed this limit, please
     * contact the author of this API.
     *
     * @note Assumes \p position is legal. Results in undefined behavior
     * otherwise.
     *
     * @note This function is OPTIONAL, but can be implemented as an
     * optimization to first generating moves and then doing moves. If not
     * implemented, the system will replace calls to this function with calls to
     * GenerateMoves(), DoMove(), and GetCanonicalPosition().
     */
    int (*GetCanonicalChildPositions)(
        Position position,
        Position children[static kRegularSolverNumChildPositionsMax]);

    /**
     * @brief Stores an array of unique canonical parent positions of
     * \p position in \p parents and returns the size of the array. For games
     * that do not support the Position Symmetry Removal Optimization, all
     * unique parent positions are included.
     *
     * @details The word unique is emphasized here because it is possible in
     * some games that a child position has two parent positions that are
     * symmetric to each other.
     *
     * @note To game developers: the current version of Regular Solver does not
     * support more than \c kRegularSolverNumParentPositionsMax parent positions
     * to be generated. If you think your game may exceed this limit, please
     * contact the author of this API.
     *
     * @note Assumes \p position is legal. Results in undefined behavior
     * otherwise.
     *
     * @note This function is OPTIONAL, but is required for Retrograde Analysis.
     * If not implemented, Retrograde Analysis will be disabled and a reverse
     * graph for the current game variant will be built and stored in memory by
     * calling DoMove() on all legal positions.
     */
    int (*GetCanonicalParentPositions)(
        Position position,
        Position parents[static kRegularSolverNumParentPositionsMax]);
} RegularSolverApi;

/** @brief Solver options of the Regular Solver. */
typedef struct RegularSolverSolveOptions {
    int verbose;       /**< Level of details to output. */
    bool force;        /**< Whether to force (re)solve the game. */
    intptr_t memlimit; /**<  Approximate heap memory limit in bytes. */
} RegularSolverSolveOptions;

/** @brief Analyzer options of the Regular Solver. */
typedef struct RegularSolverAnalyzeOptions {
    int verbose;       /**< Level of details to output. */
    bool force;        /**< Whether to force (re)analyze the game. */
    intptr_t memlimit; /**<  Approximate heap memory limit in bytes. */
} RegularSolverAnalyzeOptions;

typedef struct RegularSolverTestOptions {
    long seed;         /**< Seed for PRNG for random testing. */
    int64_t test_size; /**< Number of random positions to test. */
    int verbose;       /**< Level of details to output. */
} RegularSolverTestOptions;

#endif  // GAMESMANONE_CORE_SOLVERS_REGULAR_SOLVER_REGULAR_SOLVER_H_
