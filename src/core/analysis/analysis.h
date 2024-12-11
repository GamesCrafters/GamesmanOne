/**
 * @file analysis.h
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Game analysis helper structure.
 * @version 1.1.0
 * @date 2024-12-10
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

#ifndef GAMESMANONE_CORE_ANALYSIS_ANALYSIS_H_
#define GAMESMANONE_CORE_ANALYSIS_ANALYSIS_H_

#include <stdbool.h>  // bool
#include <stdint.h>   // int64_t
#include <stdio.h>    // FILE

#include "core/constants.h"
#include "core/types/gamesman_types.h"

/**
 * @brief Analysis of a game or a single tier.
 * @note Adding and/or removing fields from this structure will corrupt existing
 * analyses in database. They must be converted or regenerated after making the
 * change.
 * @warning This structure is large. Do NOT store Analysis objects on stack.
 * Instead, store them in static memory or on the heap.
 */
typedef struct Analysis {
    int64_t hash_size;  /**< Number of hash values defined. */
    int64_t win_count;  /**< Number of winning positions in total. */
    int64_t lose_count; /**< Number of losing positions in total. */
    int64_t tie_count;  /**< Number of tying positions in total. */
    int64_t draw_count; /**< Number of drawing positions in total. */
    int64_t move_count; /**< Number of moves in total. */

    /** Number of canonical winning positions in total. */
    int64_t canonical_win_count;
    /** Number of canonical losing positions in total. */
    int64_t canonical_lose_count;
    /** Number of canonical tying positions in total. */
    int64_t canonical_tie_count;
    /** Number of canonical drawing positions in total. */
    int64_t canonical_draw_count;
    /** Number of canonical moves in total. */
    int64_t canonical_move_count;

    /** Number of winning positions of each remoteness as an array. */
    int64_t win_summary[kNumRemotenesses];
    /** Number of losing positions of each remoteness as an array. */
    int64_t lose_summary[kNumRemotenesses];
    /** Number of tying positions of each remoteness as an array. */
    int64_t tie_summary[kNumRemotenesses];
    /** Example winning positions of each remoteness as an array. */
    TierPosition win_examples[kNumRemotenesses];
    /** Example losing positions of each remoteness as an array. */
    TierPosition lose_examples[kNumRemotenesses];
    /** Example tying positions of each remoteness as an array. */
    TierPosition tie_examples[kNumRemotenesses];
    /** An example drawing position. */
    TierPosition draw_example;

    /** Number of canonical winning positions of each remoteness as an array. */
    int64_t canonical_win_summary[kNumRemotenesses];
    /** Number of canonical losing positions of each remoteness as an array. */
    int64_t canonical_lose_summary[kNumRemotenesses];
    /** Number of canonical tying positions of each remoteness as an array. */
    int64_t canonical_tie_summary[kNumRemotenesses];
    /** Example canonical winning positions of each remoteness as an array. */
    TierPosition canonical_win_examples[kNumRemotenesses];
    /** Example canonical losing positions of each remoteness as an array. */
    TierPosition canonical_lose_examples[kNumRemotenesses];
    /** Example canonical tying positions of each remoteness as an array. */
    TierPosition canonical_tie_examples[kNumRemotenesses];
    /** An example canonical drawing position. */
    TierPosition canonical_draw_example;

    /** An example position that has the most moves. */
    TierPosition position_with_most_moves;
    /** An example winning position that has the largest remoteness. */
    TierPosition longest_win_position;
    /** An example losing position that has the largest remoteness. */
    TierPosition longest_lose_position;
    /** An example tying position that has the largest remoteness. */
    TierPosition longest_tie_position;

    int max_num_moves;           /**< Max number of moves of any position. */
    int largest_win_remoteness;  /**< Largest winning remoteness. */
    int largest_lose_remoteness; /**< Largest losing remoteness. */
    int largest_tie_remoteness;  /**< Largest tying remoteness. */
} Analysis;

/**
 * @brief Initializes the given ANALYSIS, setting all counters to 0 and all
 * example tier positions to invalid tier positions.
 */
void AnalysisInit(Analysis *analysis);

/**
 * @brief Writes the given ANALYSIS to the file with file descriptor FD.
 * Assumes FD is correctly opened and writable. FD is not closed after a call to
 * this function.
 *
 * @param analysis Source Analysis object.
 * @param fd Destination file descriptor.
 * @return 0 on success, non-zero error code otherwise.
 */
int AnalysisWrite(const Analysis *analysis, int fd);

/**
 * @brief Reads an Analysis object from file with file descriptor FD into
 * ANALYSIS. Assumes FD is correctly opened and readable. FD is not closed after
 * a call to this function.
 *
 * @param analysis Destination.
 * @param fd Source file descriptor.
 * @return 0 on success, non-zero error code otherwise.
 */
int AnalysisRead(Analysis *analysis, int fd);

// Discovering (reachable positions)

/**
 * @brief Sets the size of the hash space of ANALYSIS to HASH_SIZE.
 *
 * @param analysis Destination.
 * @param hash_size Number of hash values defined for the given game or tier.
 */
void AnalysisSetHashSize(Analysis *analysis, int64_t hash_size);

/**
 * @brief Reports to ANALYSIS that TIER_POSITION has NUM_MOVES moves and
 * NUM_CANONICAL_MOVES canonical moves. A move is canonical if and only if both
 * the parent and child positions are canonical. Assumes TIER_POSITION is valid.
 * @details Typical usage is to call this function once on each position during
 * the discovery phase of analysis to count the total number of moves.
 *
 * @param analysis Destination.
 * @param tier_position A newly discovered tier position, which is assumed to be
 * valid for the given ANALYSIS.
 * @param num_moves Number of moves available at TIER_POSITION.
 * @param num_canonical_moves Number of canonical moves available at
 * TIER_POSITION.
 */
void AnalysisDiscoverMoves(Analysis *analysis, TierPosition tier_position,
                           int num_moves, int num_canonical_moves);

// Counting (number of positions of each type)

/**
 * @brief Reports to ANALYSIS that TIER_POSITION has value VALUE, remoteness
 * REMOTENESS, and whether it IS_CANONICAL.
 * @details Typical usage is to call this function once on each position during
 * the counting phase of analysis to count the number of positions of each type.
 * For games that only solve for values and remotenesses, a type is a
 * value-remoteness pair (e.g., lose in 0, win in 1, tie in 2, draw, etc.)
 *
 * @param analysis Destination.
 * @param tier_position A newly scanned tier position, which is assumed to be
 * valid for the given ANALYSIS.
 * @param value
 * @param remoteness
 * @param is_canonical
 * @return 0 on success, kIllegalGamePositionValueError if VALUE is invalid.
 */
int AnalysisCount(Analysis *analysis, TierPosition tier_position, Value value,
                  int remoteness, bool is_canonical);

/**
 * @brief Merges all counters and examples except for those that are related to
 * moves of \p part into \p dest:
 *
 * @details This function is for reducing multiple thread-local analyses in a
 * multithreading context. The expected usage is to initialize an exclusive
 * \c Analysis structs for each thread and merge the results at the end of the
 * counting phase of an analysis. The counters of \p dest are simply incremented
 * by the amount of the corresponding fields of \p part, and the examples are
 * only replaced if \p dest does not have those examples available yet.
 *
 * The counters and examples merged include the following fields of the
 * \c Analysis struct:
 *
 * win_count, lose_count, tie_count, draw_count, canonical_win_count,
 * canonical_lose_count, canonical_tie_count, canonical_draw_count, win_summary,
 * lose_summary, tie_summary, win_examples, lose_examples, tie_examples,
 * draw_example, canonical_win_summary, canonical_lose_summary,
 * canonical_tie_summary, canonical_win_examples, canonical_lose_examples,
 * canonical_tie_examples, canonical_draw_example, longest_win_position,
 * longest_lose_position, longest_tie_position, largest_win_remoteness,
 * largest_lose_remoteness, largest_tie_remoteness.
 *
 * @param dest Destination \c Analysis.
 * @param part A partial \c Analysis whose counters and examples should be
 * merged into \p dest.
 */
void AnalysisMergeCounts(Analysis *dest, const Analysis *part);

// Aggregating (analysis of each tier into the analysis of the entire game)

/**
 * @brief Converts the given ANALYSIS to non-canonical by converting all tier
 * positions to those in the non-canonical tier and zeroing all counter fields
 * related to canonical positions and canonical moves.
 * @details Typical usage is to call this function once after loading the
 * analysis of the canonical tier from disk as the analysis of a symmetric
 * non-canonical tier. Since the tier being analyzed is non-canonical, all
 * positions and moves are non-canonical. Therefore, all related statistics must
 * be reset.
 */
void AnalysisConvertToNoncanonical(
    Analysis *analysis, Tier noncanonical,
    Position (*GetPositionInSymmetricTier)(TierPosition tier_position,
                                           Tier symmetric));

/**
 * @brief Aggregates the SRC Analysis into the DEST Analysis.
 * @details Typical usage is to call this function once on each tier analysis to
 * aggregate the result into the global analysis of the game.
 * @param dest Destination Analysis, typically the Analysis of the entire game.
 * @param src Source Analysis, typically the Analysis of a tier.
 */
void AnalysisAggregate(Analysis *dest, const Analysis *src);

// Post-Analysis

/**
 * @brief Returns an example position from ANALYSIS that has the given VALUE
 * and REMOTENESS.
 *
 * @param analysis Analysis to draw the example from.
 * @param value Value of the example position.
 * @param remoteness Remoteness of the example position.
 * @return An example position from ANALYSIS that has the given VALUE
 * and REMOTENESS, or an invalid TierPosition of tier -1 and position -1.
 */
TierPosition AnalysisGetExamplePosition(const Analysis *analysis, Value value,
                                        int remoteness);

/**
 * @brief Returns an example canonical position from ANALYSIS that has the given
 * VALUE and REMOTENESS.
 *
 * @param analysis Analysis to draw the example from.
 * @param value Value of the example position.
 * @param remoteness Remoteness of the example position.
 * @return An example canonical position from ANALYSIS that has the given VALUE
 * and REMOTENESS, or an invalid TierPosition of tier -1 and position -1.
 */
TierPosition AnalysisGetExampleCanonicalPosition(const Analysis *analysis,
                                                 Value value, int remoteness);

/**
 * @brief Returns the total number of reachable positions from ANALYSIS.
 */
int64_t AnalysisGetNumReachablePositions(const Analysis *analysis);

/**
 * @brief Returns the total number of reachable canonical positions from
 * ANALYSIS.
 */
int64_t AnalysisGetNumCanonicalPositions(const Analysis *analysis);

/**
 * @brief Returns the total number of reachable non-canonical positions from
 * ANALYSIS.
 */
int64_t AnalysisGetNumNonCanonicalPositions(const Analysis *analysis);

/**
 * @brief Returns the symmetry factor from ANALYSIS. The symmetry factor is
 * defined as a floating point value between 0 and 1 that equals
 * number_of_canonical_positions / number_of_reachable_positions.
 */
double AnalysisGetSymmetryFactor(const Analysis *analysis);

/**
 * @brief Returns whether average branching factor is valid. Returns false when
 * no positions are discovered.
 */
bool AnalysisAverageBranchingFactorIsValid(const Analysis *analysis);

/**
 * @brief Returns the average branching factor from ANALYSIS. The average
 * branching factor is defined as the average number of moves of all reachable
 * positions.
 */
double AnalysisGetAverageBranchingFactor(const Analysis *analysis);

/**
 * @brief Returns whether the average canonical branching factor is valid.
 * Returns false when no canonical positions are discovered.
 */
bool AnalysisCanonicalBranchingFactorIsValid(const Analysis *analysis);

/**
 * @brief Returns the average canonical branching factor from ANALYSIS. The
 * average canonical branching factor is defined as the average number of
 * canonical moves of all canonical positions.
 */
double AnalysisGetCanonicalBranchingFactor(const Analysis *analysis);

/**
 * @brief Returns the hash efficiency from ANALYSIS. The hash efficiency is
 * defined as a floating point value between 0 and 1 that equals
 * number_of_reachable_positions / number_of_hash_values_defined.
 */
double AnalysisGetHashEfficiency(const Analysis *analysis);

/**
 * @brief Returns the largest remoteness (regardless of value) found in
 * ANALYSIS.
 */
int AnalysisGetLargestRemoteness(const Analysis *analysis);

/**
 * @brief Returns the ratio of winning positions to all reachable positions from
 * ANALYSIS.
 */
double AnalysisGetWinRatio(const Analysis *analysis);

/**
 * @brief Returns the ratio of losing positions to all reachable positions from
 * ANALYSIS.
 */
double AnalysisGetLoseRatio(const Analysis *analysis);

/**
 * @brief Returns the ratio of tying positions to all reachable positions from
 * ANALYSIS.
 */
double AnalysisGetTieRatio(const Analysis *analysis);

/**
 * @brief Returns the ratio of drawing positions to all reachable positions from
 * ANALYSIS.
 */
double AnalysisGetDrawRatio(const Analysis *analysis);

/**
 * @brief Returns the ratio of canonical winning positions to all canonical
 * positions from ANALYSIS.
 */
double AnalysisGetCanonicalWinRatio(const Analysis *analysis);

/**
 * @brief Returns the ratio of canonical losing positions to all canonical
 * positions from ANALYSIS.
 */
double AnalysisGetCanonicalLoseRatio(const Analysis *analysis);

/**
 * @brief Returns the ratio of canonical tying positions to all canonical
 * positions from ANALYSIS.
 */
double AnalysisGetCanonicalTieRatio(const Analysis *analysis);

/**
 * @brief Returns the ratio of canonical drawing positions to all canonical
 * positions from ANALYSIS.
 */
double AnalysisGetCanonicalDrawRatio(const Analysis *analysis);

/**
 * @brief Prints a table that contains the number of reachable positions of each
 * type from ANALYSIS to STREAM. Assumes STREAM is writable.
 */
void AnalysisPrintSummary(FILE *stream, const Analysis *analysis);

/**
 * @brief Prints a table that contains the number of canonical positions of each
 * type from ANALYSIS to STREAM. Assumes STREAM is writable.
 */
void AnalysisPrintCanonicalSummary(FILE *stream, const Analysis *analysis);

/**
 * @brief Prints the high level statistics (hash size, number of positions,
 * number of moves, etc.) from ANALYSIS to STREAM. Assumes STREAM is writable.
 */
void AnalysisPrintStatistics(FILE *stream, const Analysis *analysis);

/**
 * @brief Prints the TierPosition that has the most moves from ANALYSIS to
 * STREAM. Assumes STREAM is writable.
 */
void AnalysisPrintPositionWithMostMoves(FILE *stream, const Analysis *analysis);

/**
 * @brief Prints all information from ANALYSIS to STREAM. Assumes STREAM is
 * writable.
 */
void AnalysisPrintEverything(FILE *stream, const Analysis *analysis);

#endif  // GAMESMANONE_CORE_ANALYSIS_ANALYSIS_H_
