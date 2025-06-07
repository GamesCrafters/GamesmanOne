/**
 * @file database.h
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Generic Database type and associated constants.
 * @details A Database is an abstract type of a database. To implement a new
 * Database, fully implement all member functions and set function pointers.
 * All member functions are required unless otherwise noted.
 * @version 1.2.0
 * @date 2024-11-11
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

#ifndef GAMESMANONE_CORE_TYPES_DATABASE_DATABASE_H_
#define GAMESMANONE_CORE_TYPES_DATABASE_DATABASE_H_

#include <stdbool.h>  // bool
#include <stddef.h>   // size_t
#include <stdint.h>   // int64_t

#include "core/types/base.h"
#include "core/types/database/db_probe.h"

/** @brief Constants associated with the Database type. */
enum DatabaseConstants {
    kDbNameLengthMax = 31,       /**< Maximum length of a DB's internal name. */
    kDbFormalNameLengthMax = 63, /**< Max length of a DB's formal name. */
    /** Max length of a DB file's name not including the extension. */
    kDbFileNameLengthMax = 63,
};

/** @brief Enumeration of all possible status of a tier's database file. */
enum DatabaseTierStatus {
    kDbTierStatusSolved,     /**< Solved and correctly stored. */
    kDbTierStatusCorrupted,  /**< DB exists but corrupted. */
    kDbTierStatusMissing,    /**< DB file not found. */
    kDbTierStatusCheckError, /**< Error encountered. */
};

/** @brief Enumeration of all possible status of a tier's database file. */
enum DatabaseGameStatus {
    kDbGameStatusSolved,     /**< Solved. */
    kDbGameStatusIncomplete, /**< Incomplete database. */
    kDbGameStatusCheckError, /**< Error encountered. */
};

typedef int (*GetTierNameFunc)(Tier tier,
                               char name[static kDbFileNameLengthMax + 1]);

/**
 * @brief Generic Tier Database type.
 *
 * @note To implement a new Database module, properly set the name of the new
 * DB and set each member function pointer to a function specific to the module.
 *
 * @note All fields are required unless otherwise specified.
 *
 * @note All Databases are Tier Databases because the game designer does not
 * interact with a Database directly.
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
     * @param GetTierName Function that converts a Tier to its name. If set to
     * NULL, a fallback method will be used instead.
     * @param aux Auxiliary parameter.
     *
     * @return 0 on success, non-zero error code otherwise.
     */
    int (*Init)(ReadOnlyString game_name, int variant, ReadOnlyString path,
                GetTierNameFunc GetTierName, void *aux);

    /** @brief Finalizes the Database, freeing all allocated space. */
    void (*Finalize)(void);

    // Solving API

    /**
     * @brief Creates an in-memory DB for solving of the given TIER of SIZE
     * positions.
     * @note This function is part of the Solving API.
     *
     * @param tier Tier to be solved and stored in memory.
     * @param size Size of the TIER in number of Positions.
     *
     * @return \c kNoError on success,
     * @return non-zero error code otherwise.
     */
    int (*CreateSolvingTier)(Tier tier, int64_t size);

    /**
     * @brief Flushes the in-memory DB to disk.
     * @note This function is part of the Solving API.
     *
     * @param aux Auxiliary parameter.
     *
     * @return \c kNoError on success,
     * @return non-zero error code otherwise.
     */
    int (*FlushSolvingTier)(void *aux);

    /**
     * @brief Frees the in-memory DB. Does nothing if the solving tier has not
     * been created.
     * @note This function is part of the Solving API.
     *
     * @return \c kNoError on success,
     * @return non-zero error code otherwise.
     */
    int (*FreeSolvingTier)(void);

    /**
     * @brief Sets the current game as solved.
     *
     * @return \c kNoError on success, or
     * @return non-zero error code otherwise.
     */
    int (*SetGameSolved)(void);

    /**
     * @brief Sets the value of POSITION to VALUE.
     * @note This function is part of the Solving API.
     *
     * @return \c kNoError on success,
     * @return non-zero error code otherwise.
     */
    int (*SetValue)(Position position, Value value);

    /**
     * @brief Sets the remoteness of POSITION to REMOTENESS.
     * @note This function is part of the Solving API.
     *
     * @return \c kNoError on success,
     * @return non-zero error code otherwise.
     */
    int (*SetRemoteness)(Position position, int remoteness);

    /**
     * @brief Sets the \p value and \p remoteness of \p position .
     * @note This function is part of the Solving API.
     *
     * @return \c kNoError on success,
     * @return non-zero error code otherwise.
     */
    int (*SetValueRemoteness)(Position position, Value value, int remoteness);

    /**
     * @brief Returns the value of the given \p position from in-memory DB.
     * @note This function is part of the Solving API.
     *
     * @return Value of the given \p position on success, or
     * @return \c kErrorValue otherwise.
     */
    Value (*GetValue)(Position position);

    /**
     * @brief Returns the remoteness of the given \p position from in-memory DB.
     * @note This function is part of the Solving API.
     *
     * @return Remoteness of the given \p position on success, or
     * @return \c kErrorRemoteness otherwise.
     */
    int (*GetRemoteness)(Position position);

    /**
     * @brief Returns whether there exists a checkpoint for \p tier. A
     * checkpoint can be used to restore the solving progress of a tier.
     */
    bool (*CheckpointExists)(Tier tier);

    /**
     * @brief Saves a checkpoint for the current solving tier, including the
     * current solving \p status, overwriting any existing checkpoint.
     *
     * @param status Pointer to data that stores the current solving status.
     * @param status_size Size of \p status in bytes.
     *
     * @return \c kNoError on success, or
     * @return non-zero error code otherwise.
     */
    int (*CheckpointSave)(const void *status, size_t status_size);

    /**
     * @brief Creates an in-memory DB for solving of the given \p tier of size
     * \p size by loading its checkpoint and previous solving status. Does
     * nothing and returns an error if a checkpoint cannot be found for \p tier.
     *
     * @param tier Tier to be initialized and loaded.
     * @param size Size of \p tier in number of positions.
     * @param status (Output parameter) Pointer to a buffer of size at least \p
     * status_size bytes which will be used to load the solving status from the
     * checkpoint. Must be of the same format and size as used when the
     * checkpoint was saved with \c Database::CheckpointSave.
     * @param status_size Size of \p status in bytes.
     *
     * @return \c kNoError on success, or
     * @return non-zero error code otherwise.
     */
    int (*CheckpointLoad)(Tier tier, int64_t size, void *status,
                          size_t status_size);

    /**
     * @brief Removes the checkpoint for \p tier if exists.
     *
     * @param tier Remove the checkpoint for this tier.
     * @return \c kNoError on success,
     * @return \c kFileSystemError if no checkpoint is found for \p tier, or
     * @return any other non-zero error code on failure.
     */
    int (*CheckpointRemove)(Tier tier);

    // Loading API

    /**
     * @brief Returns an upper bound, in bytes, on the amount of memory that
     * will be used to load \p tier of \p size positions.
     *
     * @param tier Tier to be loaded.
     * @param size Size of \p tier in number of positions.
     * @return An upper bound on memory usage.
     */
    size_t (*TierMemUsage)(Tier tier, int64_t size);

    /**
     * @brief Loads the given \p tier of \p size positions into memory.
     * @param tier Tier to be loaded.
     * @param size Size of \p tier in number of positions.
     *
     * @return \c kNoError on success, or
     * @return non-zero error code otherwise.
     */
    int (*LoadTier)(Tier tier, int64_t size);

    /**
     * @brief Unloads the given \p tier from memory if it was previously loaded.
     *
     * @return \c kNoError on success, or
     * @return non-zero error code otherwise.
     */
    int (*UnloadTier)(Tier tier);

    /**
     * @brief Returns whether the given \p tier has been loaded.
     *
     * @return \c kNoError on success, or
     * @return non-zero error code otherwise.
     */
    bool (*IsTierLoaded)(Tier tier);

    /**
     * @brief Returns the value of position \p position in tier \p tier if
     * \p tier has been loaded. Returns \c kErrorValue otherwise.
     *
     * @param tier A loaded tier.
     * @param position Query the value of this position.
     *
     * @return The value of \p position in \p tier on success, or
     * @return \c kErrorValue otherwise.
     */
    Value (*GetValueFromLoaded)(Tier tier, Position position);

    /**
     * @brief Returns the remoteness of position \p position in tier \p tier if
     * \p tier has been loaded. Returns \c kErrorRemoteness otherwise.
     *
     * @param tier A loaded tier.
     * @param position Query the remoteness of this position.
     *
     * @return The remoteness of \p position in \p tier on success, or
     * @return \c kErrorRemoteness otherwise.
     */
    int (*GetRemotenessFromLoaded)(Tier tier, Position position);

    // Probing API

    /**
     * @brief Initializes the given Database PROBE.
     *
     * @return \c kNoError on success,
     * @return non-zero error code otherwise.
     */
    int (*ProbeInit)(DbProbe *probe);

    /**
     * @brief Frees the given Database PROBE.
     *
     * @return \c kNoError on success,
     * @return non-zero error code otherwise.
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

    /**
     * @brief Probes the current data path and returns the solving status of the
     * current game.
     *
     * @return kDbGameStatusSolved if solved,
     * @return kDbGameStatusIncomplete if not fully solved, or
     * @return kDbGameStatusCheckError if an error occurred when checking the
     * status of the current game.
     */
    int (*GameStatus)(void);
} Database;

#endif  // GAMESMANONE_CORE_TYPES_DATABASE_DATABASE_H_
