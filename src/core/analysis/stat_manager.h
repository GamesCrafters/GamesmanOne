/**
 * @file stat_manager.h
 * @author Robert Shi (robertyishi@berkeley.edu)
 *         GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Statistics Manager Module for game analysis.
 * @version 1.1
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

#ifndef GAMESMANONE_CORE_ANALYSIS_STAT_MANAGER_H_
#define GAMESMANONE_CORE_ANALYSIS_STAT_MANAGER_H_

#include "core/analysis/analysis.h"
#include "core/data_structures/bitstream.h"
#include "core/types/gamesman_types.h"

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
 * @return 0 on success, non-zero otherwise.
 */
int StatManagerInit(ReadOnlyString game_name, int variant,
                    ReadOnlyString data_path);

/**
 * @brief Finalizes the Statistics Manager Module, freeing all dynamically
 * allocated space.
 */
void StatManagerFinalize(void);

/**
 * @brief Returns the analysis status of the given TIER.
 * @return kAnalysisTierAnalyzed if TIER has been anaylized and stored,
 * kAnalysisTierUnanalyzed if the analysis of TIER is not found on disk, or
 * kAnalysisTierCheckError if an error occurred during the check process.
 */
int StatManagerGetStatus(Tier tier);

/**
 * @brief Stores the ANALYSIS for TIER to disk.
 * @return 0 on success, non-zero error code otherwise.
 */
int StatManagerSaveAnalysis(Tier tier, const Analysis *analysis);

/**
 * @brief Loads the Analysis for TIER to DEST.
 * @return 0 on success, non-zero error code otherwise.
 */
int StatManagerLoadAnalysis(Analysis *dest, Tier tier);

/**
 * @brief Loads the discovery map for TIER as a BitStream from disk.
 * @details A discovery map is a bit stream of length equal to the size of TIER,
 * with the i-th bit turned on if and only if the position i has been discovered
 * as reachable in TIER.
 *
 * @param tier Tier to load.
 * @return Discovery map of TIER as a BitStream if TIER has been at least
 * partially discovered and store on disk. Returns an invalid BitStream all
 * fields set to 0 if the discovery map is not found on disk.
 *
 */
BitStream StatManagerLoadDiscoveryMap(Tier tier);

/**
 * @brief Saves the discovery map of TIER as a BitStream STREAM to disk.
 * @details A discovery map is a bit stream of length equal to the size of TIER,
 * with the i-th bit turned on if and only if the position i has been discovered
 * as reachable in TIER.
 *
 * @param stream Discovery map of TIER as a BitStream.
 * @param tier Tier being discovered.
 * @return 0 on success, non-zero error code otherwise.
 */
int StatManagerSaveDiscoveryMap(const BitStream *stream, Tier tier);

#endif  // GAMESMANONE_CORE_ANALYSIS_STAT_MANAGER_H_
