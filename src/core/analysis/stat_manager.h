/**
 * @file stat_manager.h
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Statistics Manager Module for game analysis.
 * @version 2.0.0
 * @date 2025-03-17
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

#ifndef GAMESMANONE_CORE_ANALYSIS_STAT_MANAGER_H_
#define GAMESMANONE_CORE_ANALYSIS_STAT_MANAGER_H_

#include "core/analysis/analysis.h"
#include "core/data_structures/concurrent_bitset.h"
#include "core/types/gamesman_types.h"

/** @brief Enumeration of all possible statuses of a tier's analysis. */
enum AnalysisTierStatus {
    kAnalysisTierAnalyzed,   /**< Analyzed and correctly stored. */
    kAnalysisTierUnanalyzed, /**< Unanalyzed (stat file not found.) */
    kAnalysisTierCheckError, /**< Error encountered. */
};

/**
 * @brief Initializes the Statistics Manager Module.
 *
 * @note This function must be called before any other StatManager functions are
 * invoked. Otherwise, calling those functions will result in undefined
 * behavior.
 *
 * @param game_name Internal name of the game.
 * @param variant Index of the game variant as an integer.
 * @param data_path Absolute or relative path to the data directory if non-NULL.
 * The default path "data" will be used if set to NULL.
 * @return \c kNoError on success,
 * @return non-zero otherwise.
 */
int StatManagerInit(ReadOnlyString game_name, int variant,
                    ReadOnlyString data_path);

/**
 * @brief Finalizes the Statistics Manager Module, freeing all dynamically
 * allocated space.
 */
void StatManagerFinalize(void);

/**
 * @brief Returns the analysis status of the given \p tier.
 * @return kAnalysisTierAnalyzed if \p tier has been anaylized and stored,
 * @return kAnalysisTierUnanalyzed if the analysis of \p tier is not found on
 * disk, or
 * @return \c kAnalysisTierCheckError if an error occurred during the check
 * process.
 */
int StatManagerGetStatus(Tier tier);

/**
 * @brief Stores the \p analysis for \p tier to disk.
 * @return \c kNoError on success,
 * @return non-zero error code otherwise.
 */
int StatManagerSaveAnalysis(Tier tier, const Analysis *analysis);

/**
 * @brief Loads the Analysis for \p tier to DEST.
 * @return \c kNoError on success
 * @return non-zero error code otherwise.
 */
int StatManagerLoadAnalysis(Analysis *dest, Tier tier);

/**
 * @brief Loads the discovery map for \p tier as a ConcurrentBitset from disk.
 * @details A discovery map is a bitset of length equal to the size of
 * \p tier with the i-th bit turned on if and only if the position i has been
 * discovered as reachable in \p tier.
 *
 * @param tier Tier to load.
 * @param size Size of \p tier.
 * @param allocator Memory allocator to use for \p dest.
 * @param dest Pointer to the pointer that will be modified to point to the
 * destination bitset on success. Not modified on failure.
 * @return \c kNoError on success, or
 * @return non-zero error code otherwise.
 */
int StatManagerLoadDiscoveryMap(Tier tier, int64_t size,
                                GamesmanAllocator *allocator,
                                ConcurrentBitset **dest);

/**
 * @brief Compresses and saves the discovery map of \p tier as a
 * ConcurrentBitset to disk.
 * @details A discovery map is a bit stream of length equal to the size of
 * \p tier, with the i-th bit turned on if and only if the position i has been
 * discovered as reachable in \p tier.
 *
 * @param stream Discovery map of \p tier as a ConcurrentBitset.
 * @param tier Tier being discovered.
 * @return \c kNoError on success,
 * @return non-zero error code otherwise.
 */
int StatManagerSaveDiscoveryMap(const ConcurrentBitset *s, Tier tier);

/**
 * @brief Removes the discovery map of \p tier from disk.
 *
 * @param tier Tier whose discovery map should be removed.
 * @return \c kNoError on success,
 * @return non-zero error code otherwise.
 */
int StatManagerRemoveDiscoveryMap(Tier tier);

#endif  // GAMESMANONE_CORE_ANALYSIS_STAT_MANAGER_H_
