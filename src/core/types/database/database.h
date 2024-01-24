/**
 * @file database.h
 * @author Robert Shi (robertyishi@berkeley.edu)
 *         GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Declaration of the Database type and associated constants.
 * @details A Database is an abstract type of a database. To implement a new
 * Database, fully implement all member functions and set function pointers.
 * All member functions are required unless otherwise noted.
 * @version 1.0.0
 * @date 2024-01-19
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

#ifndef GAMESMANONE_CORE_TYPES_DATABASE_DATABASE_H
#define GAMESMANONE_CORE_TYPES_DATABASE_DATABASE_H

#include <stdint.h>  // int64_t

#include "core/types/base.h"
#include "core/types/database/db_probe.h"

/** @brief Constants associated with the Database type. */
enum DatabaseConstants {
    kDbNameLengthMax = 31,       /**< Maximum length of a DB's internal name. */
    kDbFormalNameLengthMax = 63, /**< Max length of a DB's formal name. */
};

/** @brief Enumeration of all possible statuses of a tier's database file. */
enum DatabaseTierStatus {
    kDbTierStatusSolved,     /**< Solved and correctly stored. */
    kDbTierStatusCorrupted,  /**< DB exists but corrupted. */
    kDbTierStatusMissing,    /**< DB file not found. */
    kDbTierStatusCheckError, /**< Error encountered. */
};

/**
 * @brief Generic Tier Database type.
 *
 * @note To implement a new Database module, properly set the name of the new
 * DB and set each member function pointer to a function specific to the module.
 *
 * @note ALL Databases are Tier Databases because the game designer does not
 * have to interact with a Database directly.
 */
typedef struct Database {
    /** Internal name of the Database. Must contain no white spaces. */
    char name[kDbNameLengthMax + 1];

    /** Human-readable name of the Database. */
    char formal_name[kDbFormalNameLengthMax + 1];

    /**
     * @brief Initializes the Database.
     *
     * @param game_name Internal name of the game.
     * @param variant Index of the selected game variant.
     * @param path Path to the directory to which the Database have full access.
     * The Database may choose to store files directly inside this directory or
     * make sub-directories as the DB designer sees fit.
     * @param aux Auxiliary parameter.
     *
     * @return 0 on success, non-zero error code otherwise.
     */
    int (*Init)(ReadOnlyString game_name, int variant, ReadOnlyString path,
                void *aux);

    /** @brief Finalizes the Database, freeing all allocated space. */
    void (*Finalize)(void);

    // Solving API

    /**
     * @brief Creates a in-memory DB for solving of the given TIER of SIZE
     * positions.
     * @note This function is part of the Solving API.
     *
     * @param tier Tier to be solved and stored in memory.
     * @param size Size of the TIER in number of Positions.
     *
     * @return 0 on success, non-zero error code otherwise.
     */
    int (*CreateSolvingTier)(Tier tier, int64_t size);

    /**
     * @brief Flushes the in-memory DB to disk.
     * @note This function is part of the Solving API.
     *
     * @param aux Auxiliary parameter.
     *
     * @return 0 on success, non-zero error code otherwise.
     */
    int (*FlushSolvingTier)(void *aux);

    /**
     * @brief Frees the in-memory DB.
     * @note This function is part of the Solving API.
     *
     * @return 0 on success, non-zero error code otherwise.
     */
    int (*FreeSolvingTier)(void);

    /**
     * @brief Sets the value of POSITION to VALUE.
     * @note This function is part of the Solving API.
     *
     * @return 0 on success, non-zero error code otherwise.
     */
    int (*SetValue)(Position position, Value value);

    /**
     * @brief Sets the remoteness of POSITION to REMOTENESS.
     * @note This function is part of the Solving API.
     *
     * @return 0 on success, non-zero error code otherwise.
     */
    int (*SetRemoteness)(Position position, int remoteness);

    /**
     * @brief Returns the value of the given POSITION from in-memory DB.
     * @note This function is part of the Solving API.
     *
     * @return 0 on success, non-zero error code otherwise.
     */
    Value (*GetValue)(Position position);

    /**
     * @brief Returns the remoteness of the given POSITION from in-memory DB.
     * @note This function is part of the Solving API.
     *
     * @return 0 on success, non-zero error code otherwise.
     */
    int (*GetRemoteness)(Position position);

    // Probing API

    /**
     * @brief Initializes the given Database PROBE.
     *
     * @return 0 on success, non-zero error code otherwise.
     */
    int (*ProbeInit)(DbProbe *probe);

    /**
     * @brief Frees the given Database PROBE.
     *
     * @return 0 on success, non-zero error code otherwise.
     */
    int (*ProbeDestroy)(DbProbe *probe);

    /**
     * @brief Probes the value of TIER_POSITION from permanent storage using
     * PROBE and returns it.
     *
     * @note Assumes PROBE is initialized. Results in undefined behavior if
     * PROBE is NULL or uninitialized.
     *
     * @param probe Database probe initialized using the ProbeInit() function.
     * @param tier_position Probe the value of this TierPosition.
     *
     * @return Value of TIER_POSITION probed from pernament storage, or
     * kErrorValue if TIER_POSITION is not found.
     */
    Value (*ProbeValue)(DbProbe *probe, TierPosition tier_position);

    /**
     * @brief Probes the remoteness of TIER_POSITION from permanent storage
     * using PROBE and returns it.
     *
     * @note Assumes PROBE is initialized. Results in undefined behavior if
     * PROBE is NULL or uninitialized.
     *
     * @param probe Database probe initialized using the ProbeInit() function.
     * @param tier_position Probe the remoteness of this TierPosition.
     *
     * @return Remoteness of TIER_POSITION probed from pernament storage, or
     * -1 if TIER_POSITION is not found.
     */
    int (*ProbeRemoteness)(DbProbe *probe, TierPosition tier_position);

    /**
     * @brief Probes the current data path and returns the solving status of the
     * given TIER.
     *
     * @param tier The tier to check status of.
     *
     * @return kDbTierStatusSolved if solved,
     * @return kDbTierStatusCorrupted if corrupted,
     * @return kDbTierStatusMissing if not solved, or
     * @return kDbTierStatusCheckError if an error occurred when checking the
     * status of TIER.
     */
    int (*TierStatus)(Tier tier);
} Database;

#endif  // GAMESMANONE_CORE_TYPES_DATABASE_DATABASE_H
