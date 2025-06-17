/**
 * @file db_manager.h
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Database manager module.
 * @version 2.0.1
 * @date 2024-12-22
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

#include <stdbool.h>  // bool
#include <stddef.h>   // size_t
#include <stdint.h>   // int64_t

#include "core/types/gamesman_types.h"

/**
 * @brief Initializes the database system and load the chosen DB module.
 *
 * @note This function must be called before any of the other DbManager
 * functions are used. Otherwise, calling those functions will result in
 * undefined behavior.
 *
 * @param db DB to use.
 * @param read_only True if initializing in read-only mode, false otherwise.
 * If initialized in read-only mode, the database manager will assume that the
 * current game has been solved and will not create a directory for the chosen
 * database under the current data path.
 * @param game_name Internal name of the game.
 * @param variant Index of the game variant as an integer.
 * @param data_path Absolute or relative path to the data directory if non-NULL.
 * The default path "data" will be used if set to NULL.
 * @param GetTierName Function that converts a tier to its name. If set to
 * NULL, a fallback method will be used instead.
 * @param aux Auxiliary parameter.
 * @return 0 on success, non-zero otherwise.
 */
int DbManagerInitDb(const Database *db, bool read_only,
                    ReadOnlyString game_name, int variant,
                    ReadOnlyString data_path, GetTierNameFunc GetTierName,
                    void *aux);

// (EXPERIMENTAL) Initializes the reference database for db comparison.
int DbManagerInitRefDb(const Database *db, ReadOnlyString game_name,
                       int variant, ReadOnlyString data_path,
                       GetTierNameFunc GetTierName, void *aux);

/**
 * @brief Finalizes the database system, freeing all dynamically allocated
 * space.
 */
void DbManagerFinalizeDb(void);

// (EXPERIMENTAL) Finalizes the reference database.
void DbManagerFinalizeRefDb(void);

// ----------------------------- Solving Interface -----------------------------

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
 * @brief Creates a new solving \p tier of size \p size positions that allows
 * concurrent read and write access to records.
 *
 * @param tier Tier to create.
 * @param size Number of positions in \p tier.
 * @return kNoError on success,
 * @return non-zero error code otherwise.
 */
int DbManagerCreateConcurrentSolvingTier(Tier tier, int64_t size);

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
 * @brief Frees the solving tier in memory. Does nothing if the solving tier has
 * not been initialized.
 *
 * @return int 0 on success, non-zero otherwise.
 */
int DbManagerFreeSolvingTier(void);

/**
 * @brief Sets the current game as solved.
 *
 * @return \c kNoError on success, or
 * @return non-zero error code otherwise.
 */
int DbManagerSetGameSolved(void);

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
 * @brief Sets the \p value and \p remoteness of \p position in the solving
 * tier.
 *
 * @return \c kNoError on success, or
 * @return non-zero error code otherwise.
 */
int DbManagerSetValueRemoteness(Position position, Value value, int remoteness);

/**
 * @brief Replaces the value and remoteness of \p position in the solving tier
 * with the maximum of its original value-remoteness pair and the one provided
 * by \p val and \p remoteness . The order of value-remoteness pairs are
 * determined by the \p compare function.
 *
 * @param position Target position.
 * @param val Candidate value.
 * @param remoteness Candidate remoteness.
 * @param compare Pointer to a value-remoteness pair comparison function that
 * takes in two value-remoteness pairs (v1, r1) and (v2, r2) and returns a
 * negative integer if (v1, r1) < (v2, r2), a positive integer if (v1, r1) >
 * (v2, r2), or zero if they are equal.
 * @return \c true if the provided \p value - \p remoteness pair is greater than
 * the original value-remoteness pair and the old pair is replaced;
 * @return \c false otherwise.
 */
bool DbManagerMaximizeValueRemoteness(Position position, Value value,
                                      int remoteness,
                                      int (*compare)(Value v1, int r1, Value v2,
                                                     int r2));

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

/**
 * @brief Returns whether there exists a checkpoint for \p tier. A
 * checkpoint can be used to restore the solving progress of a tier.
 *
 * @param tier Check for any existing checkpoints for this tier.
 * @return \c true if there exists a checkpoint for \p tier, or
 * @return \c false otherwise.
 */
bool DbManagerCheckpointExists(Tier tier);

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
int DbManagerCheckpointSave(const void *status, size_t status_size);

/**
 * @brief Creates an in-memory DB for solving of the given \p tier of size
 * \p size by loading its checkpoint and previous solving status. Does nothing
 * and returns an error if a checkpoint cannot be found for \p tier.
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
int DbManagerCheckpointLoad(Tier tier, int64_t size, void *status,
                            size_t status_size);

/**
 * @brief Removes the checkpoint for \p tier if exists.
 *
 * @param tier Remove the checkpoint for this tier.
 * @return \c kNoError on success,
 * @return \c kFileSystemError if no checkpoint is found for \p tier, or
 * @return any other non-zero error code on failure.
 */
int DbManagerCheckpointRemove(Tier tier);

// ----------------------------- Loading Interface -----------------------------

/**
 * @brief Returns an upper bound, in bytes, on the amount of memory that
 * will be used to load \p tier of \p size positions.
 *
 * @param tier Tier to be loaded.
 * @param size Size of \p tier in number of positions.
 * @return An upper bound on memory usage.
 */
size_t DbManagerTierMemUsage(Tier tier, int64_t size);

/**
 * @brief Loads the given \p tier of \p size positions into memory.
 * @param tier Tier to be loaded.
 * @param size Size of \p tier in number of positions.
 *
 * @return \c kNoError on success, or
 * @return non-zero error code otherwise.
 */
int DbManagerLoadTier(Tier tier, int64_t size);

/**
 * @brief Unloads the given \p tier from memory if it was previously loaded.
 *
 * @return \c kNoError on success, or
 * @return non-zero error code otherwise.
 */
int DbManagerUnloadTier(Tier tier);

/**
 * @brief Returns whether the given \p tier has been loaded.
 *
 * @return \c kNoError on success, or
 * @return non-zero error code otherwise.
 */
bool DbManagerIsTierLoaded(Tier tier);

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
Value DbManagerGetValueFromLoaded(Tier tier, Position position);

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
int DbManagerGetRemotenessFromLoaded(Tier tier, Position position);

// ----------------------------- Probing Interface -----------------------------

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

/**
 * @brief Returns the solving status of the current game.
 *
 * @return kDbGameStatusSolved if solved,
 * @return kDbGameStatusIncomplete if not fully solved, or
 * @return kDbGameStatusCheckError if an error occurred when checking the status
 * of the current game.
 */
int DbManagerGameStatus(void);

// --------------------- (EXPERIMENTAL) Testing Interface ---------------------

int DbManagerRefProbeInit(DbProbe *probe);
int DbManagerRefProbeDestroy(DbProbe *probe);
Value DbManagerRefProbeValue(DbProbe *probe, TierPosition tier_position);
int DbManagerRefProbeRemoteness(DbProbe *probe, TierPosition tier_position);

#endif  // GAMESMANONE_CORE_DB_DB_MANAGER_H_
