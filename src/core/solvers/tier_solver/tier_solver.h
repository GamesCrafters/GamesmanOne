/**
 * @file tier_solver.h
 * @author Max Delgadillo: designed and implemented the original version
 * of tier solver (solveretrograde.c in GamesmanClassic.)
 * @author Robert Shi (robertyishi@berkeley.edu): Modularized, multithreaded
 * tier solver with various optimizations.
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief The generic tier solver capable of handling loopy and loop-free tiers.
 * @version 2.1.1
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
    /////////////////////////////////////////////////////////
    /// Tier Graph Construction Base Functions (Required) ///
    /////////////////////////////////////////////////////////

    /**
     * @brief Returns the initial tier of the current game variant. The actual
     * initial tier is always be returned even if tier symmetry removal is
     * implemented and the actual initial tier is not canonical.
     *
     * @note This function is REQUIRED. The solver system will panic if this
     * function is not implemented.
     */
    Tier (*GetInitialTier)(void);

    /**
     * @brief Stores all distinct child tiers of the given \p tier in as an
     * array in \p child_tiers and returns the size of the array. All child
     * tiers, including ones that are not canonical if tier symmetry removal is
     * implemented, will be returned.
     *
     * @details A child tier is a tier that has at least one position that can
     * be reached by performing a single move from a position within its parent
     * tier.
     *
     * @note The current version of the Tier Solver does not support generating
     * more than \c kTierSolverNumChildTiersMax child tiers.
     *
     * @note The behavior of this function is undefined if \p tier is not valid.
     *
     * @note This function is REQUIRED. The solver system will panic if this
     * function is not implemented.
     */
    int (*GetChildTiers)(Tier tier,
                         Tier child_tiers[static kTierSolverNumChildTiersMax]);

    /////////////////////////////////////////////////////////////
    /// Position Graph Construction Base Functions (Required) ///
    /////////////////////////////////////////////////////////////

    /**
     * @brief Returns the number of positions in the given \p tier . If tier
     * symmetry removal is implemented, calling this function on two different
     * tiers that are symmetric to each other returns the same size.
     *
     * @details The size of a tier is defined as the maximum hash value within
     * the tier + 1. The database will allocate an array of records for each
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
     * @brief Returns the initial position within the initial tier of the
     * current game variant. The actual initial position is always returned even
     * if position symmetry removal is implemented and the actual initial
     * position is not canonical.
     *
     * @note This function is REQUIRED. The solver system will panic if this
     * function is not implemented.
     */
    Position (*GetInitialPosition)(void);

    /**
     * @brief Returns the value of \p tier_position if \p tier_position is
     * primitive. Returns \c kUndecided otherwise. If position symmetry removal
     * is implemented, calling this function on two different positions that are
     * symmetric to each other within the same tier returns the same value.
     *
     * @note Assumes \p tier_position is valid. Passing an invalid tier or an
     * illegal position within the tier results in undefined behavior.
     *
     * @note This function is REQUIRED. The solver system will panic if this
     * function is not implemented.
     */
    Value (*Primitive)(TierPosition tier_position);

    /**
     * @brief Stores the array of available moves at \p tier_position in
     * \p moves and returns the size of the array. If position symmetry removal
     * is implemented, calling this function on two different positions that are
     * symmetric to each other within the same tier returns the same number of
     * moves and the moves should also be symmetrical but no necessarily in the
     * same order.
     *
     * @note The current version of Tier Solver does not support generating more
     * than \c kTierSolverNumMovesMax moves.
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
     * @brief Returns the resulting tier position after performing the given
     * \p move at \p tier_position . This function accepts non-canonical
     * \p tier_position inputs and always returns the actual resulting tier
     * position without applying tier/position symmetries.
     *
     * @note Assumes \p tier_position is valid and \p move is a valid move at
     * \p tier_position . Passing an invalid tier, an illegal position within
     * the tier, or an illegal move results in undefined behavior.
     *
     * @note This function is REQUIRED. The solver system will panic if this
     * function is not implemented.
     */
    TierPosition (*DoMove)(TierPosition tier_position, Move move);

    /**
     * @brief Performs a weak test on the legality of \p tier_position . The
     * function returns false if \p tier_position is definitely illegal, or
     * true if \p tier_position is considered legal and safe to be passed to all
     * other Tier Solver API functions. If tier/position symmetry is
     * implemented, passing two tier positions that are symmetric to each other
     * returns the same result.
     *
     * @details This function serves two purposes: eliminating inputs that may
     * cause errors when passed to other API functions, and for speed
     * optimizations. It is NOT intended for statistical purposes. Even if this
     * function reports that \p tier_position is legal, that position might in
     * fact be unreachable from the initial tier position. However, if this
     * function reports that \p tier_position is illegal, then \p tier_position
     * is definitely not reachable from the inital tier position and the solver
     * will not use them to, for example, generate moves or parent positions
     * while solving the game. Therefore, the game developer is responsible for
     * making sure that all tier positions deemed legal by this function are
     * actually safe to be passed to other API functions such as
     * \c TierSolverApi::GenerateMoves and \c TierSolverApi::DoMove .
     *
     * @note Assumes \p tier_position.position is between 0 and
     * GetTierSize(tier_position.tier) - 1. Passing an out-of-bounds position
     * results in undefined behavior.
     *
     * @note This function is REQUIRED. The solver system will panic if this
     * function is not implemented.
     */
    bool (*IsLegalPosition)(TierPosition tier_position);

    ////////////////////////////////////////////
    /// Tier Symmetry Removal API (Optional) ///
    ////////////////////////////////////////////

    /**
     * @brief Returns the canonical tier symmetric to the given \p tier. Returns
     * \p tier if itself is canonical.
     *
     * @details By convention, a canonical tier is one with the smallest hash
     * value in a group of symmetrical tiers. For each tier T within the group
     * including the canonical tier itself, calling
     * \c TierSolverApi::GetCanonicalTier(T) returns the same canonical tier of
     * the group.
     *
     * @note Assumes \p tier is valid. Results in undefined behavior otherwise.
     *
     * @note This function is OPTIONAL, but is required for the Tier Symmetry
     * Removal Optimization. If not implemented, the Optimization will be
     * disabled and all tiers will be treated as canonical.
     */
    Tier (*GetCanonicalTier)(Tier tier);

    /**
     * @brief Returns the position symmetric to \p tier_position within the
     * given \p symmetric tier. Returns \c tier_position.position if
     * \p symmetric is the same as \c tier_position.tier . The position returned
     * is not guaranteed to be canonical even if position symmetry is also
     * implemented.
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

    ////////////////////////////////////////////////
    /// Position Symmetry Removal API (Optional) ///
    ////////////////////////////////////////////////

    /**
     * @brief Returns the canonical position that is symmetric to
     * \p tier_position within the same tier. This function accepts tier
     * positions that belong to non-canonical tiers if tier symmetry removal is
     * also implemented.
     *
     * @details The Tier Solver currently does not support position symmetry
     * removal across tiers. By convention, a canonical position is one with the
     * smallest hash value in a group of symmetric positions that all belong to
     * the same tier. For each position P within the group (including the
     * canonical position itself), calling
     * \c TierSolverApi::GetCanonicalPosition on P returns the same canonical
     * position.
     *
     * @note Assumes \p tier_position is legal. Passing an invalid tier, or an
     * illegal position within the tier results in undefined behavior.
     *
     * @note This function is OPTIONAL, but is required for the Position
     * Symmetry Removal Optimization. If not implemented, the Optimization will
     * be disabled.
     */
    Position (*GetCanonicalPosition)(TierPosition tier_position);

    ///////////////////////////////////////////////////
    /// Performance Optimizing Functions (Optional) ///
    ///////////////////////////////////////////////////

    /**
     * @brief Returns the number of unique canonical child positions of
     * \p tier_position . If position symmetry is implemented, the actual child
     * positions are first converted into canonical positions within the same
     * tier, deduplicated, and then counted. Tier symmetry, however, is not
     * applied to the positions returned.
     *
     * @note The word unique is emphasized here because it is possible, in
     * some games, that making different moves at the same position results in
     * the same canonical child position. Even if no symmetry removal is
     * implemented, deduplication may still be necessary.
     *
     * @note Assumes \p tier_position is legal. Passing an invalid tier or an
     * illegal or primitive position within the tier results in undefined
     * behavior.
     *
     * @note This function is OPTIONAL, but can be implemented as an
     * optimization to first generating moves and then doing moves. If not
     * implemented, the system will replace calls to this function with calls to
     * \c TierSolverApi::GenerateMoves , \c TierSolverApi::DoMove ,
     * \c TierSolverApi::GetCanonicalPosition , and a \c TierPositionHashSet
     * will be used for deduplication regardless of whether duplicated child
     * positions are possible.
     */
    int (*GetNumberOfCanonicalChildPositions)(TierPosition tier_position);

    /**
     * @brief Stores all unique canonical child positions of \p tier_position
     * as an array in \p children and returns the size of the array. If position
     * symmetry is implemented, the actual child positions are first converted
     * into canonical positions within the same tier, deduplicated, and then
     * stored. Tier symmetry, however, is not applied to the positions
     * returned.
     *
     * @note The word unique is emphasized here because it is possible, in
     * some games, that making different moves results in the same canonical
     * child position. Even if no symmetry removal is implemented, deduplication
     * may still be necessary.
     *
     * @note The Tier Solver currently does not support generating more than
     * \c kTierSolverNumChildPositionsMax child positions.
     *
     * @note Assumes \p tier_position is legal and non-primitive. Passing an
     * invalid tier or an illegal or primitive position within the tier results
     * in undefined behavior.
     *
     * @note This function is OPTIONAL, but can be implemented as an
     * optimization to first generating moves and then doing moves. If not
     * implemented, the system will replace calls to this function with calls to
     * \c TierSolverApi::GenerateMoves , \c TierSolverApi::DoMove ,
     * \c TierSolverApi::GetCanonicalPosition , and a \c TierPositionHashSet
     * will be used for deduplication regardless of whether duplicated child
     * positions are possible.
     */
    int (*GetCanonicalChildPositions)(
        TierPosition tier_position,
        TierPosition children[static kTierSolverNumChildPositionsMax]);

    /**
     * @brief Stores all unique canonical parent positions of \p child that
     * belong to tier \p parent_tier as an array in \p parents and returns the
     * size of the array. If position symmetry is implemented, the actual parent
     * positions are first converted into canonical positions within the same
     * tier, deduplicated, and then stored. Tier symmetry, however, is not
     * applied to the positions returned.
     *
     * @note The word unique is emphasized here because it is possible in some
     * games that a child position has two parent positions that are symmetric
     * to each other. Even if no symmetry removal is implemented, deduplication
     * may still be necessary.
     *
     * @note The Tier Solver currently does not support generating more than
     * \c kTierSolverNumParentPositionsMax parent positions.
     *
     * @note Assumes \p parent_tier is a valid tier and \p child is a valid tier
     * position. Passing an invalid \p child , an invalid \p parent_tier , or a
     * \p parent_tier that is not actually a parent of the given \p child
     * results in undefined behavior.
     *
     * @note To simplify game implementation, this function is allowed to
     * generate illegal/primitive parent positions.
     *
     * @note This function is OPTIONAL, but is required for Retrograde Analysis.
     * If not implemented, Retrograde Analysis will be disabled and a reverse
     * position graph for all positions in the current solving tier and its
     * child tiers will be built and stored in memory by calling
     * \c TierSolverApi::GenerateMove and \c TierSolverApi::DoMove on all
     * legal positions. This operation is usually extremely memory-intensive
     * for large games and is slow on multithreaded builds due to the use of
     * mutex locks.
     */
    int (*GetCanonicalParentPositions)(
        TierPosition child, Tier parent_tier,
        Position parents[static kTierSolverNumParentPositionsMax]);

    /**
     * @brief Returns the type of \p tier . If tier symmetry is implemented,
     * passing two different tiers that are symmetric to each other returns the
     * same \c TierType.
     *
     * @note Refer to the documentation of \c TierType for the definition of
     * each tier type in src/core/types/base.h.
     *
     * @note This function is OPTIONAL. If not implemented, all tiers will be
     * treated as loopy.
     */
    TierType (*GetTierType)(Tier tier);

    //////////////////////////////////////////
    /// Visualization/Debugging (Optional) ///
    //////////////////////////////////////////

    /**
     * @brief Converts \p tier to its name, which is then used as the file name
     * for the tier database. Writes the result to \p name, assuming it has
     * enough space.
     *
     * @note It is the game developer's responsibility to make sure that the
     * name of any tier is not longer than \c kDbFileNameLengthMax bytes (not
     * including the file extension.)
     *
     * @note This function is OPTIONAL. If set to \c NULL, the tier database
     * files will use the \p tier value as their file names.
     *
     * @param tier Get name of this tier.
     * @param name Tier name output buffer.
     * @return \c kNoError on success, or
     * @return non-zero error code on failure.
     */
    int (*GetTierName)(Tier tier, char name[static kDbFileNameLengthMax + 1]);

    /**
     * @brief Maximum length of a position string not including the
     * NULL-terminating character ( \c '\0' ). This value is tied to
     * \c TierSolverApi::TierPositionToString and is used as the buffer size.
     *
     * @details The gameplay system will try to allocate this amount of space
     * plus one additional byte for the NULL-terminating character ( \c '\0' )
     * as buffer for position strings. It is the game developer's responsibility
     * to precalculate this value and make sure that enough space is provided.
     */
    int position_string_length_max;

    /**
     * @brief Converts \p tier_position into a position string and stores it in
     * \p buffer .
     *
     * @note Assumes that \p tier_position is valid and \p buffer has enough
     * space to hold the position string. Results in undefined behavior
     * otherwise.
     */
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

typedef struct TierSolverTestOptions {
    long seed;         /**< Seed for PRNG for random testing. */
    int64_t test_size; /**< Number of random positions to test in each tier. */
    int verbose;       /**< Level of details to output. */
} TierSolverTestOptions;

typedef enum TierSolverSolveStatus {
    kTierSolverSolveStatusNotSolved, /**< Not fully solved. */
    kTierSolverSolveStatusSolved,    /**< Fully solved. */
} TierSolverSolveStatus;

#endif  // GAMESMANONE_CORE_SOLVERS_TIER_SOLVER_TIER_SOLVER_H_
