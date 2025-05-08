/**
 * @file tier_solver.h
 * @author Max Delgadillo: designed and implemented the original version
 * of tier solver (solveretrograde.c in GamesmanClassic.)
 * @author Robert Shi (robertyishi@berkeley.edu): Modularized, multithreaded
 * tier solver with various optimizations.
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief The generic tier solver capable of handling loopy and loop-free tiers.
 * @version 2.1.0
 * @date 2025-03-31
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

#include "core/types/gamesman_types.h"

/** @brief Tier Solver. */
extern const Solver kTierSolver;

typedef enum {
    kTierSolverNumMovesMax = 4096,
    kTierSolverNumChildPositionsMax = kTierSolverNumMovesMax,
    kTierSolverNumParentPositionsMax = kTierSolverNumMovesMax,
    kTierSolverNumChildTiersMax = 128,
} TierSolverConstants;

/**
 * @brief API for Tier Solver.
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
     * @brief Returns the number of positions in \p tier.
     *
     * @details The size of a tier is defined as (the maximum hash value within
     * the tier + 1). The database will allocate an array of records for each
     * position within the given tier of this size. If this function returns a
     * value smaller than the actual size, the database system will, at some
     * point, complain about an out-of-bounds array access and the solver will
     * fail. If this function returns a value larger than the actual size, there
     * will be no error but memory usage and the size of the database may
     * increase.
     *
     * @note Assumes \p tier is a valid tier reachable from the initial tier.
     * Passing an illegal tier results in undefined behavior.
     *
     * @note This function is REQUIRED. The solver system will panic if this
     * function is not implemented.
     */
    int64_t (*GetTierSize)(Tier tier);

    /**
     * @brief Stores the array of moves available at \p tier_position to
     * \p moves and returns the size of the array.
     *
     * @note To game developers: the current version of Tier Solver does not
     * support more than \c kTierSolverNumMovesMax moves to be generated. If
     * you think your game may exceed this limit, please contact the author of
     * this API.
     *
     * @note Assumes \p tier_position is valid and non-primitive. Passing an
     * invalid tier, or an illegal/primitive position within the tier results in
     * undefined behavior.
     *
     * @note This function is REQUIRED. The solver system will panic if this
     * function is not implemented.
     */
    int (*GenerateMoves)(TierPosition tier_position,
                         Move moves[static kTierSolverNumMovesMax]);

    /**
     * @brief Returns the value of \p tier_position if \p tier_position is
     * primitive. Returns kUndecided otherwise.
     *
     * @note Assumes \p tier_position is valid. Passing an invalid tier or an
     * illegal position within the tier results in undefined behavior.
     *
     * @note This function is REQUIRED. The solver system will panic if this
     * function is not implemented.
     */
    Value (*Primitive)(TierPosition tier_position);

    /**
     * @brief Returns the resulting tier position after performing \p move at
     * \p tier_position.
     *
     * @note Assumes \p tier_position is valid and \p move is a valid move at
     * \p tier_position. Passing an invalid tier, an illegal position within the
     * tier, or an illegal move results in undefined behavior.
     *
     * @note This function is REQUIRED. The solver system will panic if this
     * function is not implemented.
     */
    TierPosition (*DoMove)(TierPosition tier_position, Move move);

    /**
     * @brief Performs a weak test on the legality of \p tier_position. The
     * function returns false if \p tier_position is definitely illegal, or
     * true if \p tier_position is considered legal and safe to be passed to all
     * other Tier Solver API functions.
     *
     * @details This function is for speed optimization only. It is not intended
     * for statistical purposes. Even if this function reports that
     * \p tier_position is legal, that position might in fact be unreachable
     * from the initial tier position. However, if this function reports that
     * \p tier_position is illegal, then \p tier_position is definitely not
     * reachable from the inital tier position. Furthermore, it must be
     * guaranteed by the game developer that calling all other API functions
     * such as \c TierSolverApi::GenerateMoves and \c TierSolverApi::DoMove on
     * tier positions that are deemed legal by this function results in
     * well-defined behavior.
     *
     * @note Assumes \p tier_position.position is between 0 and
     * GetTierSize(tier_position.tier) - 1. Passing an out-of-bounds position
     * results in undefined behavior.
     *
     * @note This function is REQUIRED. The solver system will panic if this
     * function is not implemented.
     */
    bool (*IsLegalPosition)(TierPosition tier_position);

    /**
     * @brief Returns the canonical position that is symmetric to
     * \p tier_position within the same tier.
     *
     * @details GAMESMAN currently does not support position symmetry removal
     * across tiers. By convention, a canonical position is one with the
     * smallest hash value in a set of symmetrical positions that all belong to
     * the same tier. For each position[i] within the set (including the
     * canonical position itself), calling
     * \c TierSolverApi::GetCanonicalPosition on position[i] returns the
     * canonical position.
     *
     * @note Assumes \p tier_position is legal. Passing an invalid tier, or an
     * illegal position within the tier results in undefined behavior.
     *
     * @note This function is OPTIONAL, but is required for the Position
     * Symmetry Removal Optimization. If not implemented, the Optimization will
     * be disabled.
     */
    Position (*GetCanonicalPosition)(TierPosition tier_position);

    /**
     * @brief Returns the number of unique canonical child positions of
     * \p tier_position. For games that do not support the Position Symmetry
     * Removal Optimization, all unique child positions are included.
     *
     * @details The word unique is emphasized here because it is possible, in
     * some games, that making different moves at the same position results in
     * the same canonical child position.
     *
     * @note Assumes \p tier_position is legal. Passing an invalid tier or an
     * illegal position within the tier results in undefined behavior.
     *
     * @note This function is OPTIONAL, but can be implemented as an
     * optimization to first generating moves and then doing moves. If not
     * implemented, the system will replace calls to this function with calls to
     * \c TierSolverApi::GenerateMoves, \c TierSolverApi::DoMove, and
     * \c TierSolverApi::GetCanonicalPosition.
     */
    int (*GetNumberOfCanonicalChildPositions)(TierPosition tier_position);

    /**
     * @brief Stores an array of unique canonical child positions of
     * \p tier_position into \p children and returns the size of the array. For
     * games that do not support the Position Symmetry Removal Optimization, all
     * unique child positions are included.
     *
     * @details The word unique is emphasized here because it is possible, in
     * some games, that making different moves results in the same canonical
     * child position.
     *
     * @note To game developers: the current version of Tier Solver does not
     * support more than \c kTierSolverNumChildPositionsMax child positions to
     * be generated. If you think your game may exceed this limit, please
     * contact the author of this API.
     *
     * @note Assumes \p tier_position is legal and non-primitive. Passing an
     * invalid tier or an illegal/primitive position within the tier results in
     * undefined behavior.
     *
     * @note This function is OPTIONAL, but can be implemented as an
     * optimization to first generating moves and then doing moves. If not
     * implemented, the system will replace calls to this function with calls to
     * \c TierSolverApi::GenerateMoves, \c TierSolverApi::DoMove, and
     * \c TierSolverApi::GetCanonicalPosition.
     */
    int (*GetCanonicalChildPositions)(
        TierPosition tier_position,
        TierPosition children[static kTierSolverNumChildPositionsMax]);

    /**
     * @brief Stores an array of unique canonical parent positions of \p child
     * that belong to tier \p parent_tier in \p parents. For games that do not
     * support the Position Symmetry Removal Optimization, all unique parent
     * positions within \p parent_tier are included.
     *
     * @details The word unique is emphasized here because it is possible in
     * some games that a child position has two parent positions that are
     * symmetric to each other.
     *
     * @note To game developers: the current version of Tier Solver does not
     * support more than \c kTierSolverNumParentPositionsMax parent positions
     * to be generated. If you think your game may exceed this limit, please
     * contact the author of this API.
     *
     * @note Assumes \p parent_tier is a valid tier and \p child is a valid tier
     * position. Passing an invalid \p child, an invalid \p parent_tier, or a
     * \p parent_tier that is not actually a parent of the given \p child
     * results in undefined behavior.
     *
     * @note This function is OPTIONAL, but is required for Retrograde Analysis.
     * If not implemented, Retrograde Analysis will be disabled and a reverse
     * position graph for all positions in the current solving tier and its
     * child tiers will be built and stored in memory by calling
     * \c TierSolverApi::GenerateMove and \c TierSolverApi::DoMove on all
     * legal positions. This operation is known to be extremely memory-intensive
     * for large games and is slow on multithreaded builds due to the use of
     * mutex locks.
     */
    int (*GetCanonicalParentPositions)(
        TierPosition child, Tier parent_tier,
        Position parents[static kTierSolverNumParentPositionsMax]);

    /**
     * @brief Returns the position symmetric to \p tier_position within the
     * given \p symmetric tier.
     *
     * @note Assumes \p tier_position is a valid tier position and \p symmetric
     * is a valid tier. Also assumes that the \p symmetric tier is symmetric to
     * \c tier_position.tier. That is, calling
     * \c TierSolverApi::GetCanonicalTier(tier_position.tier) and calling
     * \c TierSolverApi::GetCanonicalTier(symmetric) return the same canonical
     * tier. Results in undefined behavior if either assumption is false.
     *
     * @note This function is OPTIONAL, but is required for the Tier Symmetry
     * Removal Optimization. If not implemented, the Optimization will be
     * disabled and all tiers will be treated as canonical.
     */
    Position (*GetPositionInSymmetricTier)(TierPosition tier_position,
                                           Tier symmetric);

    /**
     * @brief Stores an array of child tiers of the given \p tier in
     * \p child_tiers and returns the size of the array.
     *
     * @details A child tier is a tier that has at least one position that can
     * be reached by performing a single move from a position within its parent
     * tier.
     *
     * @note To game developers: the current version of Tier Solver does not
     * support more than \c kTierSolverNumChildTiersMax child tiers to be
     * generated. If you think your game may exceed this limit, please contact
     * the author of this API.
     *
     * @note Assumes \p tier is valid. Results in undefined behavior otherwise.
     *
     * @note This function is REQUIRED. The solver system will panic if this
     * function is not implemented.
     */
    int (*GetChildTiers)(Tier tier,
                         Tier child_tiers[static kTierSolverNumChildTiersMax]);

    /**
     * @brief Returns the type of \p tier.
     *
     * @note Refer to the documentation of \c TierType for the definition of
     * each tier type in tier_solver.h.
     *
     * @note This function is OPTIONAL. If not implemented, all tiers will be
     * treated as loopy.
     */
    TierType (*GetTierType)(Tier tier);

    /**
     * @brief Returns the canonical tier symmetric to the given \p tier. Returns
     * \p tier if itself is canonical.
     *
     * @details By convention, a canonical tier is one with the smallest tier
     * value in a set of symmetrical tiers. For each tier[i] within the set
     * including the canonical tier itself, calling
     * \c TierSolverApi::GetCanonicalTier(tier[i]) returns the canonical tier.
     *
     * @note Assumes \p tier is valid. Results in undefined behavior otherwise.
     *
     * @note This function is OPTIONAL, but is required for the Tier Symmetry
     * Removal Optimization. If not implemented, the Optimization will be
     * disabled and all tiers will be treated as canonical.
     */
    Tier (*GetCanonicalTier)(Tier tier);

    /**
     * @brief Converts \p tier to its name, which is then used as the file name
     * for the tier database. Writes the result to NAME, assuming it has enough
     * space.
     *
     * @note It is the game developer's responsibility to make sure that the
     * name of any tier is not larger than \c kDbFileNameLengthMax bytes (not
     * including the file extension.)
     *
     * @note This function is OPTIONAL. If set to \c NULL, the tier database
     * files will use the \p tier value as their file names.
     *
     * @param tier Get name of this tier.
     * @param name Tier name output buffer.
     * @return 0 on success, or
     * @return non-zero error code on failure.
     */
    int (*GetTierName)(Tier tier, char name[static kDbFileNameLengthMax + 1]);

    int position_string_length_max;

    int (*TierPositionToString)(TierPosition tier_position, char *buffer);
} TierSolverApi;

/**
 * @brief Types of all detectable error by the test function of Tier Solver.
 */
enum TierSolverTestErrors {
    kTierSolverTestNoError,          /**< No error. */
    kTierSolverTestDependencyError,  /**< Test failed due to a prior error. */
    kTierSolverTestGetTierNameError, /**< Failed to get tier name. */
    /** Illegal child tier detected. */
    kTierSolverTestIllegalChildTierError,
    /** Illegal child position detected. */
    kTierSolverTestIllegalChildPosError,
    /** The positions returned by the game-specific GetCanonicalChildPositions
       did not match those returned by the default function which calls
       GenerateMoves and DoMove. */
    kTierSolverTestGetCanonicalChildPositionsMismatch,
    /** The number of canonical positions returned by the game-specific
       GetNumberOfCanonicalChildPositions did not match the value returned by
       the default function which calls GenerateMoves and DoMove. */
    kTierSolverTestGetNumberOfCanonicalChildPositionsMismatch,
    /** Applying tier symmetry within the same tier returned a different
       position. */
    kTierSolverTestTierSymmetrySelfMappingError,
    /** Applying tier symmetry twice - first using a symmetric tier, then using
       the original tier - returned a different position */
    kTierSolverTestTierSymmetryInconsistentError,
    /** One of the canonical child positions of a legal canonical position was
       found not to have that legal position as its parent */
    kTierSolverTestChildParentMismatchError,
    /** One of the canonical parent positions of a legal canonical position was
       found not to have that legal position as its child. */
    kTierSolverTestParentChildMismatchError,
};

/** @brief Solver options of the Tier Solver. */
typedef struct TierSolverSolveOptions {
    int verbose;       /**< Level of details to output. */
    bool force;        /**< Whether to force (re)solve the game. */
    intptr_t memlimit; /**< Approximate heap memory limit in bytes. */
} TierSolverSolveOptions;

/** @brief Analyzer options of the Tier Solver. */
typedef struct TierSolverAnalyzeOptions {
    int verbose;       /**< Level of details to output. */
    bool force;        /**< Whether to force (re)analyze the game. */
    intptr_t memlimit; /**< Approximate heap memory limit in bytes. */
} TierSolverAnalyzeOptions;

typedef enum TierSolverSolveStatus {
    kTierSolverSolveStatusNotSolved, /**< Not fully solved. */
    kTierSolverSolveStatusSolved,    /**< Fully solved. */
} TierSolverSolveStatus;

#endif  // GAMESMANONE_CORE_SOLVERS_TIER_SOLVER_TIER_SOLVER_H_
