/**
 * @file db_manager.h
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Database manager module.
 * @version 2.1.0
 * @date 2025-06-23
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

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "core/types/base.h"
#include "core/types/database/database.h"
#include "core/types/database/db_probe.h"

extern const Database *db_manager_current_db;

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

/**
 * @brief Finalizes the database system, freeing all dynamically allocated
 * space.
 */
static inline void DbManagerFinalizeDb(void) {
    if (db_manager_current_db) {
        db_manager_current_db->Finalize();
    }

    db_manager_current_db = NULL;
}

// ----------------------------- Solving Interface -----------------------------

/**
 * @brief Creates a new TIER of size SIZE (measured in positions) for solving in
 * memory.
 *
 * @note This function is not thread-safe.
 *
 * @param tier Tier to create.
 * @param size Number of positions in TIER.
 * @return int 0 on success, non-zero otherwise.
 */
static inline int DbManagerCreateSolvingTier(Tier tier, int64_t size) {
    return db_manager_current_db->CreateSolvingTier(tier, size);
}

/**
 * @brief Creates a new solving \p tier of size \p size positions that allows
 * concurrent read and write access to records.
 *
 * @note This function is not thread-safe.
 *
 * @param tier Tier to create.
 * @param size Number of positions in \p tier.
 * @return kSuccess on success,
 * @return non-zero error code otherwise.
 */
static inline int DbManagerCreateConcurrentSolvingTier(Tier tier,
                                                       int64_t size) {
    return db_manager_current_db->CreateConcurrentSolvingTier(tier, size);
}

/**
 * @brief Flushes the solving tier in memory to disk.
 *
 * @note Assumes the solving tier has been created. Results in undefined
 * behavior if not.
 *
 * @note This function is not thread-safe.
 *
 * @param aux Auxiliary parameter.
 * @return int 0 on success, non-zero otherwise.
 */
static inline int DbManagerFlushSolvingTier(void *aux) {
    return db_manager_current_db->FlushSolvingTier(aux);
}

/**
 * @brief Frees the solving tier in memory. Does nothing if the solving tier has
 * not been initialized.
 *
 * @note This function is not thread-safe.
 *
 * @return int 0 on success, non-zero otherwise.
 */
static inline int DbManagerFreeSolvingTier(void) {
    return db_manager_current_db->FreeSolvingTier();
}

/**
 * @brief Sets the current game as solved.
 *
 * @note This function is not thread-safe.
 *
 * @return \c kSuccess on success, or
 * @return non-zero error code otherwise.
 */
static inline int DbManagerSetGameSolved(void) {
    return db_manager_current_db->SetGameSolved();
}

/**
 * @brief Sets the value of POSITION in the solving tier to VALUE.
 *
 * @note Assumes the solving tier has been created. Results in undefined
 * behavior if not.
 *
 * @note This function is thread-safe if the solving tier is created via
 * DbManagerCreateConcurrentSolvingTier.
 *
 * @return int 0 on success, non-zero otherwise.
 */
static inline int DbManagerSetValue(Position position, Value value) {
    return db_manager_current_db->SetValue(position, value);
}

/**
 * @brief Sets the remoteness of POSITION in the solving tier to REMOTENESS.
 *
 * @note Assumes the solving tier has been created. Results in undefined
 * behavior if not.
 *
 * @note This function is thread-safe if the solving tier is created via
 * DbManagerCreateConcurrentSolvingTier.
 *
 * @return int 0 on success, non-zero otherwise.
 */
static inline int DbManagerSetRemoteness(Position position, int remoteness) {
    return db_manager_current_db->SetRemoteness(position, remoteness);
}

/**
 * @brief Sets the \p value and \p remoteness of \p position in the solving
 * tier.
 *
 * @note This function is thread-safe if the solving tier is created via
 * DbManagerCreateConcurrentSolvingTier.
 *
 * @return \c kSuccess on success, or
 * @return non-zero error code otherwise.
 */
static inline int DbManagerSetValueRemoteness(Position position, Value value,
                                              int remoteness) {
    return db_manager_current_db->SetValueRemoteness(position, value,
                                                     remoteness);
}

/**
 * @brief Replaces the value and remoteness of \p position in the solving tier
 * with the maximum of its original value-remoteness pair and the one provided
 * by \p val and \p remoteness . The order of value-remoteness pairs are
 * determined by the \p compare function.
 *
 * @note This function is thread-safe if the solving tier is created via
 * DbManagerCreateConcurrentSolvingTier.
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
static inline bool DbManagerMaximizeValueRemoteness(
    Position position, Value value, int remoteness,
    int (*compare)(Value v1, int r1, Value v2, int r2)) {
    return db_manager_current_db->MaximizeValueRemoteness(position, value,
                                                          remoteness, compare);
}

/**
 * @brief Subtracts one from the number of undecided children of \p position and
 * returns the value immediately preceding the operation if the value of the
 * position is \c kUndecided and its number of undecided children is at least
 * one. Does nothing otherwise.
 *
 * @note By convention, we overload the remoteness field of a record as the
 * counter for the number of undecided children of that position when its value
 * is \c kUndecided .
 *
 * @note This function is thread-safe if the solving tier is created via
 * DbManagerCreateConcurrentSolvingTier.
 *
 * @param position Target position.
 * @return The number of undecided children immediately preceding the
 * subtraction, or
 * @return 0 if the operation is not performed.
 */
static inline int DbManagerDecrementNumUndecidedChildren(Position position) {
    return db_manager_current_db->DecrementNumUndecidedChildren(position);
}

/**
 * @brief Sets the number of undecided children of \p position to zero and
 * returns the value immediately preceding the operation if the value of the
 * position is \c kUndecided . Does nothing otherwise.
 *
 * @note By convention, we overload the remoteness field of a record as the
 * counter for the number of undecided children of that position when its value
 * is \c kUndecided .
 *
 * @note This function is thread-safe if the solving tier is created via
 * DbManagerCreateConcurrentSolvingTier.
 *
 * @param position Target position.
 * @return The number of undecided children immediately preceding the operation,
 * or
 * @return 0 if the operation is not performed.
 */
static inline int DbManagerClearNumUndecidedChildren(Position position) {
    return db_manager_current_db->ClearNumUndecidedChildren(position);
}

/**
 * @brief Returns the value of POSITION in the solving tier.
 *
 * @note Assumes the solving tier has been created. Results in undefined
 * behavior if not.
 *
 * @note This function is thread-safe if the solving tier is created via
 * DbManagerCreateConcurrentSolvingTier.
 */
static inline Value DbManagerGetValue(Position position) {
    return db_manager_current_db->GetValue(position);
}

/**
 * @brief Returns the remoteness of POSITION in the solving tier.
 *
 * @note Assumes the solving tier has been created. Results in undefined
 * behavior if not.
 *
 * @note This function is thread-safe if the solving tier is created via
 * DbManagerCreateConcurrentSolvingTier.
 */
static inline int DbManagerGetRemoteness(Position position) {
    return db_manager_current_db->GetRemoteness(position);
}

/**
 * @brief Returns the number of undecided children of \p position if its value
 * is \c kUndecided . Returns 0 otherwise.
 *
 * @note By convention, we overload the remoteness field of a record as the
 * counter for the number of undecided children of that position when its value
 * is \c kUndecided .
 *
 * @note This function is thread-safe if the solving tier is created via
 * DbManagerCreateConcurrentSolvingTier.
 *
 * @param position Target position.
 * @return The number of undecided children of \p position if its value is
 * \c kUndecided , or
 * @return 0 otherwise.
 */
static inline int DbManagerGetNumUndecidedChildren(Position position) {
    return db_manager_current_db->GetNumUndecidedChildren(position);
}

/**
 * @brief Returns the maximum number of segment buffers supported by the
 * Database.
 *
 * @return Maximum number of segment buffers supported.
 */
static inline int DbManagerSegmentationMaxNumBuffers(void) {
    return db_manager_current_db->segmentation.MaxNumBuffers();
}

/**
 * @brief Make \p num_segments segment buffers available for solving the
 * given \p tier , where each segment is of \p size positions
 *
 * @param tier Tier to be solved.
 * @param num_segments Number of segment buffers to create.
 * @param size Number of positions in each segment.
 * @return \c kSuccess on success,
 * @return \c kMallocFailureError on memory allocation failure, or
 * @return other non-zero error code otherwise.
 */
static inline int DbManagerSegmentationCreateBuffers(Tier tier,
                                                     int num_segments,
                                                     int64_t size) {
    return db_manager_current_db->segmentation.CreateBuffers(tier, num_segments,
                                                             size);
}

/**
 * @brief Loads the \p seg_idx -th segment from disk into the \p buf_idx
 * -th segment buffer.
 *
 * @param buf_idx Index of the destination segment buffer.
 * @param seg_idx Index of the segment to load.
 * @return \c kSuccess on success,
 * @return \c kIllegalArgumentError if \p buf_idx is not active,
 * @return \c kFileSystemError if \p seg_idx does not exist on disk or
 * failed to read the segment from disk, or
 * @return other non-zero error code otherwise.
 */
static inline int DbManagerSegmentationLoad(int buf_idx, int seg_idx) {
    return db_manager_current_db->segmentation.Load(buf_idx, seg_idx);
}

/**
 * @brief Flushes the contents of the \p buf_idx -th segment buffer to
 * disk as the \p seg_idx -th segment of the current tier.
 *
 * @param buf_idx Index of the source segment buffer.
 * @param seg_idx Index of the segment.
 * @return \c kSuccess on success,
 * @return \c kIllegalArgument if \p buf_idx is not active,
 * @return \c kFileSystemError if failed to write to disk, or
 * @return other non-zero error code otherwise.
 */
static inline int DbManagerSegmentationFlush(int buf_idx, int seg_idx) {
    return db_manager_current_db->segmentation.Flush(buf_idx, seg_idx);
}

/**
 * @brief Deallocates all active segment buffers.
 *
 * @return \c kSuccess on success, or
 * @return other non-zero error code otherwise.
 */
static inline int DbManagerSegmentationFreeBuffers(void) {
    return db_manager_current_db->segmentation.FreeBuffers();
}

/**
 * @brief Returns the value of the position at the given \p offset
 * relative to the first position in the segment currently loaded in the
 * \p buf_idx -th segment buffer.
 *
 * @param buf_idx Index of the segment buffer, which is assumed to be
 * active.
 * @param offset Position offset relative to the first position in the
 * segment loaded in the target segment buffer. The offset is assumed to
 * be valid.
 * @return Value of the position at the given \p offset in the target
 * segment buffer.
 */
static inline Value DbManagerSegmentationGetValue(int buf_idx, int64_t offset) {
    return db_manager_current_db->segmentation.GetValue(buf_idx, offset);
}

/**
 * @brief Returns the remoteness of the position at the given \p offset
 * relative to the first position in the segment currently loaded in the
 * \p buf_idx -th segment buffer.
 *
 * @param buf_idx Index of the segment buffer, which is assumed to be
 * active.
 * @param offset Position offset relative to the first position in the
 * segment loaded in the target segment buffer. The offset is assumed to
 * be valid.
 * @return Remoteness of the position at the given \p offset in the
 * target segment buffer.
 */
static inline int DbManagerSegmentationGetRemoteness(int buf_idx,
                                                     int64_t offset) {
    return db_manager_current_db->segmentation.GetRemoteness(buf_idx, offset);
}

/**
 * @brief Sets the value and remoteness of the position at the given
 * \p offset relative to the first position in the segment currently
 * loaded in the \p buf_idx -th segment buffer.
 *
 * @param buf_idx Index of the segment buffer, which is assumed to be
 * active.
 * @param offset Position offset relative to the first position in the
 * segment loaded in the target segment buffer. The offset is assumed to
 * be valid.
 * @param value New value.
 * @param remoteness New remoteness.
 */
static inline void DbManagerSegmentationSetValueRemoteness(int buf_idx,
                                                           int64_t offset,
                                                           Value value,
                                                           int remoteness) {
    db_manager_current_db->segmentation.SetValueRemoteness(buf_idx, offset,
                                                           value, remoteness);
}

/**
 * @brief Merges all \p num_segments segments on disk into the normal
 * Database archive. The output is equivalent to the output of
 * \c Database::FlushSolvingTier in the non-segmented methods.
 *
 * @param tier_size Number of positions in the solving tier.
 * @param num_segments Total number of segments, where all segments are
 * assumed to have been solved and flushed to disk.
 * @return \c kSuccess on success,
 * @return \c kFileSystemError if any file operation such as reading a
 * segment or saving the output failed, or
 * @return other non-zero error code otherwise.
 */
static inline int DbManagerSegmentationConsolidate(int64_t tier_size,
                                                   int num_segments) {
    return db_manager_current_db->segmentation.Consolidate(tier_size,
                                                           num_segments);
}

/**
 * @brief Returns whether there exists a checkpoint for \p tier. A
 * checkpoint can be used to restore the solving progress of a tier.
 *
 * @note This function is not thread-safe.
 *
 * @param tier Check for any existing checkpoints for this tier.
 * @return \c true if there exists a checkpoint for \p tier, or
 * @return \c false otherwise.
 */
static inline bool DbManagerCheckpointExists(Tier tier) {
    return db_manager_current_db->CheckpointExists(tier);
}

/**
 * @brief Saves a checkpoint for the current solving tier, including the
 * current solving \p status, overwriting any existing checkpoint.
 *
 * @note This function is not thread-safe.
 *
 * @param status Pointer to data that stores the current solving status.
 * @param status_size Size of \p status in bytes.
 *
 * @return \c kSuccess on success, or
 * @return non-zero error code otherwise.
 */
static inline int DbManagerCheckpointSave(const void *status,
                                          size_t status_size) {
    return db_manager_current_db->CheckpointSave(status, status_size);
}

/**
 * @brief Creates an in-memory DB for solving of the given \p tier of size
 * \p size by loading its checkpoint and previous solving status. Does nothing
 * and returns an error if a checkpoint cannot be found for \p tier.
 *
 * @note This function is not thread-safe.
 *
 * @param tier Tier to be initialized and loaded.
 * @param size Size of \p tier in number of positions.
 * @param status (Output parameter) Pointer to a buffer of size at least \p
 * status_size bytes which will be used to load the solving status from the
 * checkpoint. Must be of the same format and size as used when the
 * checkpoint was saved with \c Database::CheckpointSave.
 * @param status_size Size of \p status in bytes.
 *
 * @return \c kSuccess on success, or
 * @return non-zero error code otherwise.
 */
static inline int DbManagerCheckpointLoad(Tier tier, int64_t size, void *status,
                                          size_t status_size) {
    return db_manager_current_db->CheckpointLoad(tier, size, status,
                                                 status_size);
}

/**
 * @brief Removes the checkpoint for \p tier if exists.
 *
 * @note This function is not thread-safe.
 *
 * @param tier Remove the checkpoint for this tier.
 * @return \c kSuccess on success,
 * @return \c kFileSystemError if no checkpoint is found for \p tier, or
 * @return any other non-zero error code on failure.
 */
static inline int DbManagerCheckpointRemove(Tier tier) {
    return db_manager_current_db->CheckpointRemove(tier);
}

// ----------------------------- Loading Interface -----------------------------

/**
 * @brief Returns an upper bound, in bytes, on the amount of memory that
 * will be used to load \p tier of \p size positions.
 *
 * @note This function is thread-safe.
 *
 * @param tier Tier to be loaded.
 * @param size Size of \p tier in number of positions.
 * @return An upper bound on memory usage in bytes.
 */
static inline size_t DbManagerTierMemUsage(Tier tier, int64_t size) {
    return db_manager_current_db->TierMemUsage(tier, size);
}

/**
 * @brief Returns an upper bound, in bytes, on the amount of memory that
 * will be used to store a concurrent solving tier \p tier of \p size positions.
 *
 * @note This function is thread-safe.
 *
 * @param tier Concurrent solving tier to be created in memory.
 * @param size Size of \p tier in number of positions.
 * @return An upper bound on memory usage in bytes.
 */
static inline size_t DbManagerConcurrentTierMemUsage(Tier tier, int64_t size) {
    return db_manager_current_db->ConcurrentTierMemUsage(tier, size);
}

/**
 * @brief Loads the given \p tier of \p size positions into memory.
 *
 * @note This function is not thread-safe.
 *
 * @param tier Tier to be loaded.
 * @param size Size of \p tier in number of positions.
 *
 * @return \c kSuccess on success, or
 * @return non-zero error code otherwise.
 */
static inline int DbManagerLoadTier(Tier tier, int64_t size) {
    return db_manager_current_db->LoadTier(tier, size);
}

/**
 * @brief Unloads the given \p tier from memory if it was previously loaded.
 *
 * @note This function is not thread-safe.
 *
 * @return \c kSuccess on success, or
 * @return non-zero error code otherwise.
 */
static inline int DbManagerUnloadTier(Tier tier) {
    return db_manager_current_db->UnloadTier(tier);
}

/**
 * @brief Returns whether the given \p tier has been loaded.
 *
 * @note This function is thread-safe.
 *
 * @return \c kSuccess on success, or
 * @return non-zero error code otherwise.
 */
static inline bool DbManagerIsTierLoaded(Tier tier) {
    return db_manager_current_db->IsTierLoaded(tier);
}

/**
 * @brief Returns the value of position \p position in tier \p tier if
 * \p tier has been loaded. Returns \c kErrorValue otherwise.
 *
 * @note This function is thread-safe.
 *
 * @param tier A loaded tier.
 * @param position Query the value of this position.
 *
 * @return The value of \p position in \p tier on success, or
 * @return \c kErrorValue otherwise.
 */
static inline Value DbManagerGetValueFromLoaded(Tier tier, Position position) {
    return db_manager_current_db->GetValueFromLoaded(tier, position);
}

/**
 * @brief Returns the remoteness of position \p position in tier \p tier if
 * \p tier has been loaded. Returns \c kErrorRemoteness otherwise.
 *
 * @note This function is thread-safe.
 *
 * @param tier A loaded tier.
 * @param position Query the remoteness of this position.
 *
 * @return The remoteness of \p position in \p tier on success, or
 * @return \c kErrorRemoteness otherwise.
 */
static inline int DbManagerGetRemotenessFromLoaded(Tier tier,
                                                   Position position) {
    return db_manager_current_db->GetRemotenessFromLoaded(tier, position);
}

// ----------------------------- Probing Interface -----------------------------

/**
 * @brief Initializes PROBE using the method provided by the current database.
 *
 * @note This function is not thread-safe.
 */
static inline int DbManagerProbeInit(DbProbe *probe) {
    return db_manager_current_db->ProbeInit(probe);
}

/**
 * @brief Destroys PROBE using the method provided by the current database.
 *
 * @note This function is not thread-safe.
 */
static inline int DbManagerProbeDestroy(DbProbe *probe) {
    return db_manager_current_db->ProbeDestroy(probe);
}

/**
 * @brief Reads the value of TIER_POSITION in the current database from disk
 * using the given initialized PROBE and returns it.
 *
 * @note Results in undefined behavior if PROBE has not been initialized.
 *
 * @note This function is not thread-safe.
 *
 * @param probe Initialized database probe.
 * @param tier_position TierPosition to read.
 * @return Value of the given TIER_POSITION in database; kErrorValue if the
 * given TIER has not been solved, the given POSITION is out of bounds, or any
 * other error occurred.
 */
static inline Value DbManagerProbeValue(DbProbe *probe,
                                        TierPosition tier_position) {
    return db_manager_current_db->ProbeValue(probe, tier_position);
}

/**
 * @brief Reads the remoteness of TIER_POSITION in the current database from
 * disk using the given initialized PROBE and returns it.
 *
 * @note Results in undefined behavior if PROBE has not been initialized.
 *
 * @note This function is not thread-safe.
 *
 * @param probe Initialized database probe.
 * @param tier_position TierPosition to read.
 * @return Remoteness of the given TIER_POSITION in database; a negative value
 * if the given TIER has not been solved, the given POSITION is out of bounds,
 * or any other error occurred.
 */
static inline int DbManagerProbeRemoteness(DbProbe *probe,
                                           TierPosition tier_position) {
    return db_manager_current_db->ProbeRemoteness(probe, tier_position);
}

/**
 * @brief Returns the status of TIER.
 *
 * @note This function is not thread-safe.
 *
 * @param tier Tier to check.
 * @return \c kDbTierStatusSolved if solved,
 * @return \c kDbTierStatusCorrupted if corrupted,
 * @return \c kDbTierStatusMissing if not solved, or
 * @return \c kDbTierStatusCheckError if an error occurred when checking the
 * status of TIER.
 */
static inline int DbManagerTierStatus(Tier tier) {
    return db_manager_current_db->TierStatus(tier);
}

/**
 * @brief Returns the solving status of the current game.
 *
 * @note This function is not thread-safe.
 *
 * @return \c kDbGameStatusSolved if solved,
 * @return \c kDbGameStatusIncomplete if not fully solved, or
 * @return \c kDbGameStatusCheckError if an error occurred when checking the
 * status of the current game.
 */
static inline int DbManagerGameStatus(void) {
    return db_manager_current_db->GameStatus();
}

// ------------------------- External Access Interface -------------------------

/**
 * @brief Returns the path the Database is initialized with.
 *
 * @return The path the Database is initialized with, or
 * @return \c NULL if the Database is not initialized.
 */
static inline const char *DbManagerGetPath(void) {
    return db_manager_current_db->GetPath();
}

#endif  // GAMESMANONE_CORE_DB_DB_MANAGER_H_
