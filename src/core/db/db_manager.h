/**
 * @file db_manager.h
 * @author Robert Shi (robertyishi@berkeley.edu)
 *         GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Database manager module.
 * @version 1.2.1
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

#ifndef GAMESMANONE_CORE_DB_DB_MANAGER_H_
#define GAMESMANONE_CORE_DB_DB_MANAGER_H_

#include <stdint.h>  // int64_t

#include "core/types/gamesman_types.h"

/**
 * @brief Initializes the database system and load the chosen DB module.
 *
 * @note This function must be called before any of the other DbManager
 * functions are used. Otherwise, calling those functions will result in
 * undefined behavior.
 *
 * @param db DB to use.
 * @param game_name Internal name of the game.
 * @param variant Index of the game variant as an integer.
 * @param data_path Absolute or relative path to the data directory if non-NULL.
 * The default path "data" will be used if set to NULL.
 * @param GetTierName Function that converts a tier to its name. If set to
 * NULL, a fallback method will be used instead.
 * @param aux Auxiliary parameter.
 * @return 0 on success, non-zero otherwise.
 */
int DbManagerInitDb(const Database *db, ReadOnlyString game_name, int variant,
                    ReadOnlyString data_path, GetTierNameFunc GetTierName,
                    void *aux);

/**
 * @brief Finalizes the database system, freeing all dynamically allocated
 * space.
 */
void DbManagerFinalizeDb(void);

// ---------------------------- Solving Interface. ----------------------------

/**
 * @brief Creates a new TIER of size SIZE (measured in positions) for solving in
 * memory.
 *
 * @param tier Tier to create.
 * @param size Number of positions in TIER.
 * @return int 0 on success, non-zero otherwise.
 */
int DbManagerCreateSolvingTier(Tier tier, int64_t size);

/**
 * @brief Flushes the solving tier in memory to disk.
 *
 * @note Assumes the solving tier has been created. Results in undefined
 * behavior if not.
 *
 * @param aux Auxiliary parameter.
 * @return int 0 on success, non-zero otherwise.
 */
int DbManagerFlushSolvingTier(void *aux);

/**
 * @brief Frees the solving tier in memory.
 *
 * @return int 0 on success, non-zero otherwise.
 */
int DbManagerFreeSolvingTier(void);

/**
 * @brief Sets the value of POSITION in the solving tier to VALUE.
 *
 * @note Assumes the solving tier has been created. Results in undefined
 * behavior if not.
 *
 * @return int 0 on success, non-zero otherwise.
 */
int DbManagerSetValue(Position position, Value value);

/**
 * @brief Sets the remoteness of POSITION in the solving tier to REMOTENESS.
 *
 * @note Assumes the solving tier has been created. Results in undefined
 * behavior if not.
 *
 * @return int 0 on success, non-zero otherwise.
 */
int DbManagerSetRemoteness(Position position, int remoteness);

/**
 * @brief Returns the value of POSITION in the solving tier.
 *
 * @note Assumes the solving tier has been created. Results in undefined
 * behavior if not.
 */
Value DbManagerGetValue(Position position);

/**
 * @brief Returns the remoteness of POSITION in the solving tier.
 *
 * @note Assumes the solving tier has been created. Results in undefined
 * behavior if not.
 */
int DbManagerGetRemoteness(Position position);

// ---------------------------- Probing Interface. ----------------------------

/**
 * @brief Initializes PROBE using the method provided by the current database.
 */
int DbManagerProbeInit(DbProbe *probe);

/**
 * @brief Destroys PROBE using the method provided by the current database.
 */
int DbManagerProbeDestroy(DbProbe *probe);

/**
 * @brief Reads the value of TIER_POSITION in the current database from disk
 * using the given initialized PROBE and returns it.
 *
 * @note Results in undefined behavior if PROBE has not been initialized.
 *
 * @param probe Initialized database probe.
 * @param tier_position TierPosition to read.
 * @return Value of the given TIER_POSITION in database; kErrorValue if the
 * given TIER has not been solved, the given POSITION is out of bounds, or any
 * other error occurred.
 */
Value DbManagerProbeValue(DbProbe *probe, TierPosition tier_position);

/**
 * @brief Reads the remoteness of TIER_POSITION in the current database from
 * disk using the given initialized PROBE and returns it.
 *
 * @note Results in undefined behavior if PROBE has not been initialized.
 *
 * @param probe Initialized database probe.
 * @param tier_position TierPosition to read.
 * @return Remoteness of the given TIER_POSITION in database; a negative value
 * if the given TIER has not been solved, the given POSITION is out of bounds,
 * or any other error occurred.
 */
int DbManagerProbeRemoteness(DbProbe *probe, TierPosition tier_position);

/**
 * @brief Returns the status of TIER.
 *
 * @param tier Tier to check.
 * @return kDbTierStatusSolved if solved,
 * @return kDbTierStatusCorrupted if corrupted,
 * @return kDbTierStatusMissing if not solved, or
 * @return kDbTierStatusCheckError if an error occurred when checking the status
 * of TIER.
 */
int DbManagerTierStatus(Tier tier);

#endif  // GAMESMANONE_CORE_DB_DB_MANAGER_H_
