/**
 * @file db_manager.h
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Database manager module.
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
 * @brief Initializes the database system and loads the chosen DB module.
 *
 * @note This function must be called before any of the other `DbManager`
 * functions are used. Otherwise, calling those functions will result in
 * undefined behavior.
 *
 * @param[in] db DB to use.
 * @param[in] read_only `true` if initializing in read-only mode, `false`
 * otherwise. If initialized in read-only mode, the database manager will assume
 * that the current game has been solved and will not create a directory for the
 * chosen database under the current data path.
 * @param[in] game_name Internal name of the game.
 * @param[in] variant Index of the game variant as an integer.
 * @param[in] data_path Absolute or relative path to the data directory if
 * non-`NULL`. The default path "data" will be used if set to `NULL`.
 * @param[in] GetTierName Function that converts a tier to its name. If set to
 * `NULL`, a fallback method will be used instead.
 * @param[in,out] aux Auxiliary parameter.
 *
 * @note If the provided `db` has an uninitialized or empty `formal_name`, this
 * function will fatally exit the program.
 *
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
 * @brief Creates a new `tier` of size `size` (measured in positions) for
 * solving in memory.
 *
 * @note This function is not thread-safe.
 *
 * @param[in] tier Tier to create.
 * @param[in] size Number of positions in `tier`.
 *
 * @return 0 on success, non-zero otherwise.
 */
static inline int DbManagerCreateSolvingTier(Tier tier, int64_t size) {
    return db_manager_current_db->CreateSolvingTier(tier, size);
}

/**
 * @brief Creates a new solving `tier` of size `size` positions that allows
 * concurrent read and write access to records.
 *
 * @note This function is not thread-safe.
 *
 * @param[in] tier Tier to create.
 * @param[in] size Number of positions in `tier`.
 *
 * @return `kSuccess` on success, or a non-zero error code otherwise.
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
 * @note This function is not thread-safe.
 *
 * @param[in,out] aux Auxiliary parameter.
 *
 * @return 0 on success, non-zero otherwise.
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
 * @return 0 on success, non-zero otherwise.
 */
static inline int DbManagerFreeSolvingTier(void) {
    return db_manager_current_db->FreeSolvingTier();
}

/**
 * @brief Sets the current game as solved.
 *
 * @note This function is not thread-safe.
 *
 * @return `kSuccess` on success, or a non-zero error code otherwise.
 */
static inline int DbManagerSetGameSolved(void) {
    return db_manager_current_db->SetGameSolved();
}

/**
 * @brief Sets the value of `position` in the solving tier to `value`.
 *
 * @note Assumes the solving tier has been created. Results in undefined
 * behavior if not.
 * @note This function is thread-safe if the solving tier is created via
 * `DbManagerCreateConcurrentSolvingTier`.
 *
 * @param[in] position Position to modify.
 * @param[in] value Value to set.
 *
 * @return 0 on success, non-zero otherwise.
 */
static inline int DbManagerSetValue(Position position, Value value) {
    return db_manager_current_db->SetValue(position, value);
}

/**
 * @brief Sets the remoteness of `position` in the solving tier to `remoteness`.
 *
 * @note Assumes the solving tier has been created. Results in undefined
 * behavior if not.
 * @note This function is thread-safe if the solving tier is created via
 * `DbManagerCreateConcurrentSolvingTier`.
 *
 * @param[in] position Position to modify.
 * @param[in] remoteness Remoteness to set.
 *
 * @return 0 on success, non-zero otherwise.
 */
static inline int DbManagerSetRemoteness(Position position, int remoteness) {
    return db_manager_current_db->SetRemoteness(position, remoteness);
}

/**
 * @brief Sets the `value` and `remoteness` of `position` in the solving tier.
 *
 * @note This function is thread-safe if the solving tier is created via
 * `DbManagerCreateConcurrentSolvingTier`.
 *
 * @param[in] position Position to modify.
 * @param[in] value Value to set.
 * @param[in] remoteness Remoteness to set.
 *
 * @return `kSuccess` on success, or a non-zero error code otherwise.
 */
static inline int DbManagerSetValueRemoteness(Position position, Value value,
                                              int remoteness) {
    return db_manager_current_db->SetValueRemoteness(position, value,
                                                     remoteness);
}

/**
 * @brief Replaces the value and remoteness of `position` in the solving tier
 * with the maximum of its original value-remoteness pair and the one provided
 * by `value` and `remoteness`. The order of value-remoteness pairs is
 * determined by the `compare` function.
 *
 * @note This function is thread-safe if the solving tier is created via
 * `DbManagerCreateConcurrentSolvingTier`.
 *
 * @param[in] position Target position.
 * @param[in] value Candidate value.
 * @param[in] remoteness Candidate remoteness.
 * @param[in] compare Pointer to a value-remoteness pair comparison function
 * that takes in two value-remoteness pairs (`v1`, `r1`) and (`v2`, `r2`) and
 * returns a negative integer if (`v1`, `r1`) < (`v2`, `r2`), a positive integer
 * if (`v1`, `r1`) > (`v2`, `r2`), or zero if they are equal.
 *
 * @retval true If the provided `value`-`remoteness` pair is greater than
 * the original value-remoteness pair and the old pair is replaced.
 * @retval false Otherwise.
 */
static inline bool DbManagerMaximizeValueRemoteness(
    Position position, Value value, int remoteness,
    int (*compare)(Value v1, int r1, Value v2, int r2)) {
    return db_manager_current_db->MaximizeValueRemoteness(position, value,
                                                          remoteness, compare);
}

/**
 * @brief Subtracts one from the number of undecided children of `position` and
 * returns the count immediately preceding the operation if the value of the
 * position is `kUndecided` and its number of undecided children is at least
 * one. Does nothing otherwise.
 *
 * @note By convention, we overload the remoteness field of a record as the
 * counter for the number of undecided children of that position when its value
 * is `kUndecided`.
 * @note This function is thread-safe if the solving tier is created via
 * `DbManagerCreateConcurrentSolvingTier`.
 *
 * @param[in] position Target position.
 *
 * @return The number of undecided children immediately preceding the
 * subtraction, or 0 if the operation is not performed.
 */
static inline int DbManagerDecrementNumUndecidedChildren(Position position) {
    return db_manager_current_db->DecrementNumUndecidedChildren(position);
}

/**
 * @brief Sets the number of undecided children of `position` to zero and
 * returns the count immediately preceding the operation if the value of the
 * position is `kUndecided`. Does nothing otherwise.
 *
 * @note By convention, we overload the remoteness field of a record as the
 * counter for the number of undecided children of that position when its value
 * is `kUndecided`.
 * @note This function is thread-safe if the solving tier is created via
 * `DbManagerCreateConcurrentSolvingTier`.
 *
 * @param[in] position Target position.
 *
 * @return The number of undecided children immediately preceding the operation,
 * or 0 if the operation is not performed.
 */
static inline int DbManagerClearNumUndecidedChildren(Position position) {
    return db_manager_current_db->ClearNumUndecidedChildren(position);
}

/**
 * @brief Returns the value of `position` in the solving tier.
 *
 * @note Assumes the solving tier has been created. Results in undefined
 * behavior if not.
 * @note This function is thread-safe if the solving tier is created via
 * `DbManagerCreateConcurrentSolvingTier`.
 *
 * @param[in] position Position to query.
 *
 * @return The value of `position` in the solving tier.
 */
static inline Value DbManagerGetValue(Position position) {
    return db_manager_current_db->GetValue(position);
}

/**
 * @brief Returns the remoteness of `position` in the solving tier.
 *
 * @note Assumes the solving tier has been created. Results in undefined
 * behavior if not.
 * @note This function is thread-safe if the solving tier is created via
 * `DbManagerCreateConcurrentSolvingTier`.
 *
 * @param[in] position Position to query.
 *
 * @return The remoteness of `position` in the solving tier.
 */
static inline int DbManagerGetRemoteness(Position position) {
    return db_manager_current_db->GetRemoteness(position);
}

/**
 * @brief Returns the number of undecided children of `position` if its value
 * is `kUndecided`. Returns 0 otherwise.
 *
 * @note By convention, we overload the remoteness field of a record as the
 * counter for the number of undecided children of that position when its value
 * is `kUndecided`.
 * @note This function is thread-safe if the solving tier is created via
 * `DbManagerCreateConcurrentSolvingTier`.
 *
 * @param[in] position Target position.
 *
 * @return The number of undecided children of `position` if its value is
 * `kUndecided`, or 0 otherwise.
 */
static inline int DbManagerGetNumUndecidedChildren(Position position) {
    return db_manager_current_db->GetNumUndecidedChildren(position);
}

/**
 * @brief Returns the maximum number of segment buffers supported by the
 * database.
 *
 * @return Maximum number of segment buffers supported.
 */
static inline int DbManagerSegmentationMaxNumBuffers(void) {
    return db_manager_current_db->segmentation.MaxNumBuffers();
}

/**
 * @brief Makes `num_segments` segment buffers available for solving the
 * given `tier`, where each segment is of `size` positions.
 *
 * @param[in] tier Tier to be solved.
 * @param[in] num_segments Number of segment buffers to create.
 * @param[in] size Number of positions in each segment.
 *
 * @return `kSuccess` on success, `kMallocFailureError` on memory allocation
 * failure, or another non-zero error code otherwise.
 */
static inline int DbManagerSegmentationCreateBuffers(Tier tier,
                                                     int num_segments,
                                                     int64_t size) {
    return db_manager_current_db->segmentation.CreateBuffers(tier, num_segments,
                                                             size);
}

/**
 * @brief Loads the `seg_idx`-th segment from disk into the `buf_idx`-th
 * segment buffer.
 *
 * @param[in] buf_idx Index of the destination segment buffer.
 * @param[in] seg_idx Index of the segment to load.
 *
 * @return `kSuccess` on success, `kIllegalArgumentError` if `buf_idx` is not
 * active, `kFileSystemError` if `seg_idx` does not exist on disk or failed to
 * read the segment from disk, or another non-zero error code otherwise.
 */
static inline int DbManagerSegmentationLoad(int buf_idx, int seg_idx) {
    return db_manager_current_db->segmentation.Load(buf_idx, seg_idx);
}

/**
 * @brief Flushes the contents of the `buf_idx`-th segment buffer to
 * disk as the `seg_idx`-th segment of the current tier.
 *
 * @param[in] buf_idx Index of the source segment buffer.
 * @param[in] seg_idx Index of the segment.
 *
 * @return `kSuccess` on success, `kIllegalArgument` if `buf_idx` is not
 * active, `kFileSystemError` if failed to write to disk, or another non-zero
 * error code otherwise.
 */
static inline int DbManagerSegmentationFlush(int buf_idx, int seg_idx) {
    return db_manager_current_db->segmentation.Flush(buf_idx, seg_idx);
}

/**
 * @brief Deallocates all active segment buffers.
 *
 * @return `kSuccess` on success, or another non-zero error code otherwise.
 */
static inline int DbManagerSegmentationFreeBuffers(void) {
    return db_manager_current_db->segmentation.FreeBuffers();
}

/**
 * @brief Returns the value of the position at the given `offset`
 * relative to the first position in the segment currently loaded in the
 * `buf_idx`-th segment buffer.
 *
 * @param[in] buf_idx Index of the segment buffer, which is assumed to be
 * active.
 * @param[in] offset Position offset relative to the first position in the
 * segment loaded in the target segment buffer. The offset is assumed to
 * be valid.
 *
 * @return Value of the position at the given `offset` in the target
 * segment buffer.
 */
static inline Value DbManagerSegmentationGetValue(int buf_idx, int64_t offset) {
    return db_manager_current_db->segmentation.GetValue(buf_idx, offset);
}

/**
 * @brief Returns the remoteness of the position at the given `offset`
 * relative to the first position in the segment currently loaded in the
 * `buf_idx`-th segment buffer.
 *
 * @param[in] buf_idx Index of the segment buffer, which is assumed to be
 * active.
 * @param[in] offset Position offset relative to the first position in the
 * segment loaded in the target segment buffer. The offset is assumed to
 * be valid.
 *
 * @return Remoteness of the position at the given `offset` in the
 * target segment buffer.
 */
static inline int DbManagerSegmentationGetRemoteness(int buf_idx,
                                                     int64_t offset) {
    return db_manager_current_db->segmentation.GetRemoteness(buf_idx, offset);
}

/**
 * @brief Sets the value and remoteness of the position at the given
 * `offset` relative to the first position in the segment currently
 * loaded in the `buf_idx`-th segment buffer.
 *
 * @param[in] buf_idx Index of the segment buffer, which is assumed to be
 * active.
 * @param[in] offset Position offset relative to the first position in the
 * segment loaded in the target segment buffer. The offset is assumed to
 * be valid.
 * @param[in] value New value.
 * @param[in] remoteness New remoteness.
 */
static inline void DbManagerSegmentationSetValueRemoteness(int buf_idx,
                                                           int64_t offset,
                                                           Value value,
                                                           int remoteness) {
    db_manager_current_db->segmentation.SetValueRemoteness(buf_idx, offset,
                                                           value, remoteness);
}

/**
 * @brief Merges all `num_segments` segments on disk into the normal
 * database archive. The output is equivalent to the output of
 * `Database::FlushSolvingTier` in the non-segmented methods.
 *
 * @param[in] tier_size Number of positions in the solving tier.
 * @param[in] num_segments Total number of segments, where all segments are
 * assumed to have been solved and flushed to disk.
 *
 * @return `kSuccess` on success, `kFileSystemError` if any file operation such
 * as reading a segment or saving the output failed, or another non-zero error
 * code otherwise.
 */
static inline int DbManagerSegmentationConsolidate(int64_t tier_size,
                                                   int num_segments) {
    return db_manager_current_db->segmentation.Consolidate(tier_size,
                                                           num_segments);
}

/**
 * @brief Returns whether there exists a checkpoint for `tier`. A
 * checkpoint can be used to restore the solving progress of a tier.
 *
 * @note This function is not thread-safe.
 *
 * @param[in] tier Check for any existing checkpoints for this tier.
 *
 * @retval true If there exists a checkpoint for `tier`.
 * @retval false Otherwise.
 */
static inline bool DbManagerCheckpointExists(Tier tier) {
    return db_manager_current_db->CheckpointExists(tier);
}

/**
 * @brief Saves a checkpoint for the current solving tier, including the
 * current solving `status`, overwriting any existing checkpoint.
 *
 * @note This function is not thread-safe.
 *
 * @param[in] status Pointer to data that stores the current solving status.
 * @param[in] status_size Size of `status` in bytes.
 *
 * @return `kSuccess` on success, or a non-zero error code otherwise.
 */
static inline int DbManagerCheckpointSave(const void *status,
                                          size_t status_size) {
    return db_manager_current_db->CheckpointSave(status, status_size);
}

/**
 * @brief Creates an in-memory DB for solving the given `tier` of size
 * `size` by loading its checkpoint and previous solving status. Does nothing
 * and returns an error if a checkpoint cannot be found for `tier`.
 *
 * @note This function is not thread-safe.
 *
 * @param[in] tier Tier to be initialized and loaded.
 * @param[in] size Size of `tier` in number of positions.
 * @param[out] status Pointer to a buffer of size at least `status_size` bytes
 * which will be used to load the solving status from the checkpoint. Must be
 * of the same format and size as used when the checkpoint was saved with
 * `Database::CheckpointSave`.
 * @param[in] status_size Size of `status` in bytes.
 *
 * @return `kSuccess` on success, or a non-zero error code otherwise.
 */
static inline int DbManagerCheckpointLoad(Tier tier, int64_t size, void *status,
                                          size_t status_size) {
    return db_manager_current_db->CheckpointLoad(tier, size, status,
                                                 status_size);
}

/**
 * @brief Removes the checkpoint for `tier` if it exists.
 *
 * @note This function is not thread-safe.
 *
 * @param[in] tier Remove the checkpoint for this tier.
 *
 * @return `kSuccess` on success, `kFileSystemError` if no checkpoint is found
 * for `tier`, or any other non-zero error code on failure.
 */
static inline int DbManagerCheckpointRemove(Tier tier) {
    return db_manager_current_db->CheckpointRemove(tier);
}

// ----------------------------- Loading Interface -----------------------------

/**
 * @brief Returns an upper bound, in bytes, on the amount of memory that
 * will be used to load `tier` of `size` positions.
 *
 * @note This function is thread-safe.
 *
 * @param[in] tier Tier to be loaded.
 * @param[in] size Size of `tier` in number of positions.
 *
 * @return An upper bound on memory usage in bytes.
 */
static inline size_t DbManagerTierMemUsage(Tier tier, int64_t size) {
    return db_manager_current_db->TierMemUsage(tier, size);
}

/**
 * @brief Returns an upper bound, in bytes, on the amount of memory that
 * will be used to store a concurrent solving tier `tier` of `size` positions.
 *
 * @note This function is thread-safe.
 *
 * @param[in] tier Concurrent solving tier to be created in memory.
 * @param[in] size Size of `tier` in number of positions.
 *
 * @return An upper bound on memory usage in bytes.
 */
static inline size_t DbManagerConcurrentTierMemUsage(Tier tier, int64_t size) {
    return db_manager_current_db->ConcurrentTierMemUsage(tier, size);
}

/**
 * @brief Loads the given `tier` of `size` positions into memory.
 *
 * @note This function is not thread-safe.
 *
 * @param[in] tier Tier to be loaded.
 * @param[in] size Size of `tier` in number of positions.
 *
 * @return `kSuccess` on success, or a non-zero error code otherwise.
 */
static inline int DbManagerLoadTier(Tier tier, int64_t size) {
    return db_manager_current_db->LoadTier(tier, size);
}

/**
 * @brief Unloads the given `tier` from memory if it was previously loaded.
 *
 * @note This function is not thread-safe.
 *
 * @param[in] tier Tier to unload.
 *
 * @return `kSuccess` on success, or a non-zero error code otherwise.
 */
static inline int DbManagerUnloadTier(Tier tier) {
    return db_manager_current_db->UnloadTier(tier);
}

/**
 * @brief Returns whether the given `tier` has been loaded.
 *
 * @note This function is thread-safe.
 *
 * @param[in] tier Tier to check.
 *
 * @retval true If `tier` has been loaded.
 * @retval false Otherwise.
 */
static inline bool DbManagerIsTierLoaded(Tier tier) {
    return db_manager_current_db->IsTierLoaded(tier);
}

/**
 * @brief Returns the value of position `position` in tier `tier` if
 * `tier` has been loaded. Returns `kErrorValue` otherwise.
 *
 * @note This function is thread-safe.
 *
 * @param[in] tier A loaded tier.
 * @param[in] position Query the value of this position.
 *
 * @return The value of `position` in `tier` on success, or
 * `kErrorValue` otherwise.
 */
static inline Value DbManagerGetValueFromLoaded(Tier tier, Position position) {
    return db_manager_current_db->GetValueFromLoaded(tier, position);
}

/**
 * @brief Returns the remoteness of position `position` in tier `tier` if
 * `tier` has been loaded. Returns `kErrorRemoteness` otherwise.
 *
 * @note This function is thread-safe.
 *
 * @param[in] tier A loaded tier.
 * @param[in] position Query the remoteness of this position.
 *
 * @return The remoteness of `position` in `tier` on success, or
 * `kErrorRemoteness` otherwise.
 */
static inline int DbManagerGetRemotenessFromLoaded(Tier tier,
                                                   Position position) {
    return db_manager_current_db->GetRemotenessFromLoaded(tier, position);
}

// ----------------------------- Probing Interface -----------------------------

/**
 * @brief Initializes `probe` using the method provided by the current database.
 *
 * @note This function is not thread-safe.
 *
 * @param[in,out] probe Probe to initialize.
 *
 * @return 0 on success, non-zero otherwise.
 */
static inline int DbManagerProbeInit(DbProbe *probe) {
    return db_manager_current_db->ProbeInit(probe);
}

/**
 * @brief Destroys `probe` using the method provided by the current database.
 *
 * @note This function is not thread-safe.
 *
 * @param[in,out] probe Probe to destroy.
 *
 * @return 0 on success, non-zero otherwise.
 */
static inline int DbManagerProbeDestroy(DbProbe *probe) {
    return db_manager_current_db->ProbeDestroy(probe);
}

/**
 * @brief Reads the value of `tier_position` in the current database from disk
 * using the given initialized `probe` and returns it.
 *
 * @note Results in undefined behavior if `probe` has not been initialized.
 * @note This function is not thread-safe.
 *
 * @param[in,out] probe Initialized database probe.
 * @param[in] tier_position Tier position to read.
 *
 * @return Value of the given `tier_position` in the database, or `kErrorValue`
 * if the given tier has not been solved, the given position is out of bounds,
 * or any other error occurred.
 */
static inline Value DbManagerProbeValue(DbProbe *probe,
                                        TierPosition tier_position) {
    return db_manager_current_db->ProbeValue(probe, tier_position);
}

/**
 * @brief Reads the remoteness of `tier_position` in the current database from
 * disk using the given initialized `probe` and returns it.
 *
 * @note Results in undefined behavior if `probe` has not been initialized.
 * @note This function is not thread-safe.
 *
 * @param[in,out] probe Initialized database probe.
 * @param[in] tier_position Tier position to read.
 *
 * @return Remoteness of the given `tier_position` in the database, or a
 * negative value if the given tier has not been solved, the given position is
 * out of bounds, or any other error occurred.
 */
static inline int DbManagerProbeRemoteness(DbProbe *probe,
                                           TierPosition tier_position) {
    return db_manager_current_db->ProbeRemoteness(probe, tier_position);
}

/**
 * @brief Returns the status of `tier`.
 *
 * @note This function is not thread-safe.
 *
 * @param[in] tier Tier to check.
 *
 * @retval kDbTierStatusSolved If solved.
 * @retval kDbTierStatusCorrupted If corrupted.
 * @retval kDbTierStatusMissing If not solved.
 * @retval kDbTierStatusCheckError If an error occurred when checking the
 * status of `tier`.
 */
static inline int DbManagerTierStatus(Tier tier) {
    return db_manager_current_db->TierStatus(tier);
}

/**
 * @brief Returns the solving status of the current game.
 *
 * @note This function is not thread-safe.
 *
 * @retval kDbGameStatusSolved If solved.
 * @retval kDbGameStatusIncomplete If not fully solved.
 * @retval kDbGameStatusCheckError If an error occurred when checking the
 * status of the current game.
 */
static inline int DbManagerGameStatus(void) {
    return db_manager_current_db->GameStatus();
}

// ------------------------- External Access Interface -------------------------

/**
 * @brief Returns the path the database is initialized with.
 *
 * @return The path the database is initialized with, or `NULL` if the database
 * is not initialized.
 */
static inline const char *DbManagerGetPath(void) {
    return db_manager_current_db->GetPath();
}

#endif  // GAMESMANONE_CORE_DB_DB_MANAGER_H_
