/**
 * @file database.h
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Generic Database type and associated constants. A Database is an
 * abstract type of a database. To implement a new Database, fully implement all
 * member functions and set function pointers. All member functions are required
 * unless otherwise noted.
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

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "core/types/base.h"
#include "core/types/database/db_probe.h"

/**
 * @brief Constants associated with the Database type.
 */
enum DatabaseConstants {
    /** Maximum length of a DB's internal name. */
    kDbNameLengthMax = 31,

    /** Max length of a DB's formal name. */
    kDbFormalNameLengthMax = 63,

    /** Max length of a DB file's name not including the extension. */
    kDbFileNameLengthMax = 63,
};

/**
 * @brief Enumeration of all possible statuses of a tier's database file.
 */
enum DatabaseTierStatus {
    kDbTierStatusSolved,     /**< Solved and correctly stored. */
    kDbTierStatusCorrupted,  /**< DB exists but corrupted. */
    kDbTierStatusMissing,    /**< DB file not found. */
    kDbTierStatusCheckError, /**< Error encountered. */
};

/**
 * @brief Enumeration of all possible statuses of a game's database.
 */
enum DatabaseGameStatus {
    kDbGameStatusSolved,     /**< Solved. */
    kDbGameStatusIncomplete, /**< Incomplete database. */
    kDbGameStatusCheckError, /**< Error encountered. */
};

/**
 * @brief Tier name getter function signature.
 */
typedef int (*GetTierNameFunc)(
    Tier tier, char name[ARRAY_PARAM_STATIC kDbFileNameLengthMax + 1]);

/**
 * @brief Generic Tier Database type.
 *
 * @note To implement a new Database module, properly set the name of the new
 * DB and set each member function pointer to a function specific to the module.
 * @note All fields are required unless otherwise specified.
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
     * @param[in] game_name Internal name of the game.
     * @param[in] variant Index of the selected game variant.
     * @param[in] path Path to the directory to which the Database has full
     * access. The Database may choose to store files directly inside this
     * directory or make sub-directories as the DB designer sees fit.
     * @param[in] GetTierName Function that converts a Tier to its name. If set
     * to NULL, a fallback method will be used instead.
     * @param[in,out] aux Auxiliary parameter.
     *
     * @return 0 on success, non-zero error code otherwise.
     */
    int (*Init)(ReadOnlyString game_name, int variant, ReadOnlyString path,
                GetTierNameFunc GetTierName, void *aux);

    /**
     * @brief Finalizes the Database, freeing all allocated space.
     */
    void (*Finalize)(void);

    // Solving API

    /**
     * @brief Creates an in-memory DB for solving the given `tier` of size
     * `size` positions.
     *
     * @note This function is part of the Solving API.
     * @note In-memory databases created using this function are not thread-safe
     * and the following functions, when called on the same position, must be
     * synchronized externally:
     *
     *     - SetValue
     *
     *     - SetRemoteness
     *
     *     - SetValueRemoteness
     *
     *     - MaximizeValueRemoteness
     *
     *     - DecrementNumUndecidedChildren
     *
     *     - ClearNumUndecidedChildren
     *
     *     - GetValue
     *
     *     - GetRemoteness
     *
     *     - GetNumUndecidedChildren
     *
     * @param[in] tier Tier to be solved and stored in memory.
     * @param[in] size Size of `tier` in number of positions.
     *
     * @return `kSuccess` on success,
     *         non-zero error code otherwise.
     */
    int (*CreateSolvingTier)(Tier tier, int64_t size);

    /**
     * @brief Creates a multi-reader multi-writer concurrent in-memory DB for
     * solving the given `tier` of size `size` positions.
     *
     * @note This function is part of the Solving API.
     *
     * @param[in] tier Tier to be solved and stored in memory.
     * @param[in] size Size of `tier` in number of positions.
     *
     * @return `kSuccess` on success,
     *         non-zero error code otherwise.
     */
    int (*CreateConcurrentSolvingTier)(Tier tier, int64_t size);

    /**
     * @brief Flushes the in-memory DB to disk.
     *
     * @note This function is part of the Solving API.
     *
     * @param[in,out] aux Auxiliary parameter.
     *
     * @return `kSuccess` on success,
     *         non-zero error code otherwise.
     */
    int (*FlushSolvingTier)(void *aux);

    /**
     * @brief Frees the in-memory DB. Does nothing if the solving tier has not
     * been created.
     *
     * @note This function is part of the Solving API.
     *
     * @return `kSuccess` on success,
     *         non-zero error code otherwise.
     */
    int (*FreeSolvingTier)(void);

    /**
     * @brief Sets the current game as solved.
     *
     * @return `kSuccess` on success, or
     *         non-zero error code otherwise.
     */
    int (*SetGameSolved)(void);

    /**
     * @brief Sets the value of `position` to `value`.
     *
     * @note This function is part of the Solving API.
     *
     * @param[in] position Target position.
     * @param[in] value Target value.
     *
     * @return `kSuccess` on success,
     *         non-zero error code otherwise.
     */
    int (*SetValue)(Position position, Value value);

    /**
     * @brief Sets the remoteness of `position` to `remoteness`.
     *
     * @note This function is part of the Solving API.
     *
     * @param[in] position Target position.
     * @param[in] remoteness Target remoteness.
     *
     * @return `kSuccess` on success,
     *         non-zero error code otherwise.
     */
    int (*SetRemoteness)(Position position, int remoteness);

    /**
     * @brief Sets the `value` and `remoteness` of `position`.
     *
     * @note This function is part of the Solving API.
     *
     * @param[in] position Target position.
     * @param[in] value Target value.
     * @param[in] remoteness Target remoteness.
     *
     * @return `kSuccess` on success,
     *         non-zero error code otherwise.
     */
    int (*SetValueRemoteness)(Position position, Value value, int remoteness);

    /**
     * @brief Replaces the value and remoteness of `position` with the maximum
     * of its original value-remoteness pair and the one provided by `value`
     * and `remoteness`. The order of value-remoteness pairs is
     * determined by the `compare` function.
     *
     * @param[in] position Position to update.
     * @param[in] value Candidate value.
     * @param[in] remoteness Candidate remoteness.
     * @param[in] compare Pointer to a value-remoteness pair comparison function
     * that takes in two value-remoteness pairs (v1, r1) and (v2, r2) and
     * returns a negative integer if (v1, r1) < (v2, r2), a positive integer if
     * (v1, r1) > (v2, r2), or zero if they are equal.
     *
     * @retval `true` if the provided `value` - `remoteness` pair is greater
     * than the original value-remoteness pair and the old pair is replaced;
     * @retval `false` otherwise.
     */
    bool (*MaximizeValueRemoteness)(Position position, Value value,
                                    int remoteness,
                                    int (*compare)(Value v1, int r1, Value v2,
                                                   int r2));

    /**
     * @brief Subtracts one from the number of undecided children of `position`
     * and returns the value immediately preceding the operation if the value of
     * the position is `kUndecided` and its number of undecided children is at
     * least one. Does nothing otherwise.
     *
     * @note By convention, we overload the remoteness field of a record as the
     * counter for the number of undecided children of that position when its
     * value is `kUndecided`.
     *
     * @param[in] position Target position.
     *
     * @return The number of undecided children immediately preceding the
     * subtraction, or
     *         0 if the operation is not performed.
     */
    int (*DecrementNumUndecidedChildren)(Position position);

    /**
     * @brief Sets the number of undecided children of `position` to zero and
     * returns the value immediately preceding the operation if the value of the
     * position is `kUndecided`. Does nothing otherwise.
     *
     * @note By convention, we overload the remoteness field of a record as the
     * counter for the number of undecided children of that position when its
     * value is `kUndecided`.
     *
     * @param[in] position Target position.
     *
     * @return The number of undecided children immediately preceding the
     * operation, or
     *         0 if the operation is not performed.
     */
    int (*ClearNumUndecidedChildren)(Position position);

    /**
     * @brief Returns the value of the given `position` from in-memory DB.
     *
     * @note This function is part of the Solving API.
     *
     * @param[in] position Target position.
     *
     * @return Value of the given `position` on success, or
     *         `kErrorValue` otherwise.
     */
    Value (*GetValue)(Position position);

    /**
     * @brief Returns the remoteness of the given `position` from in-memory DB.
     *
     * @note This function is part of the Solving API.
     *
     * @param[in] position Target position.
     *
     * @return Remoteness of the given `position` on success, or
     *         `kErrorRemoteness` otherwise.
     */
    int (*GetRemoteness)(Position position);

    /**
     * @brief Returns the number of undecided children of `position` if its
     * value is `kUndecided`. Returns 0 otherwise.
     *
     * @note By convention, we overload the remoteness field of a record as the
     * counter for the number of undecided children of that position when its
     * value is `kUndecided`.
     *
     * @param[in] position Target position.
     *
     * @return The number of undecided children of `position` if its value is
     * `kUndecided`, or
     *         0 otherwise.
     */
    int (*GetNumUndecidedChildren)(Position position);

    /**
     * @brief Segmentation API supports creating, saving, and loading of
     * database segments in memory, which can be used if the solving algorithm
     * supports streaming, greatly reducing the amount of memory required to
     * load the entire database at once.
     */
    struct {
        /**
         * @brief Returns the maximum number of segment buffers supported by the
         * Database.
         *
         * @return Maximum number of segment buffers supported.
         */
        int (*MaxNumBuffers)(void);

        /**
         * @brief Makes `num_segments` segment buffers available for solving the
         * given `tier`, where each segment is of `size` positions.
         *
         * @param[in] tier Tier to be solved.
         * @param[in] num_segments Number of segment buffers to create.
         * @param[in] size Number of positions in each segment.
         *
         * @return `kSuccess` on success,
         *         `kMallocFailureError` on memory allocation failure, or
         *         other non-zero error code otherwise.
         */
        int (*CreateBuffers)(Tier tier, int num_segments, int64_t size);

        /**
         * @brief Loads the `seg_idx`-th segment from disk into the `buf_idx`-th
         * segment buffer.
         *
         * @param[in] buf_idx Index of the destination segment buffer.
         * @param[in] seg_idx Index of the segment to load.
         *
         * @return `kSuccess` on success,
         *         `kIllegalArgumentError` if `buf_idx` is not active,
         *         `kFileSystemError` if `seg_idx` does not exist on disk or
         * failed to read the segment from disk, or
         *         other non-zero error code otherwise.
         */
        int (*Load)(int buf_idx, int seg_idx);

        /**
         * @brief Flushes the contents of the `buf_idx`-th segment buffer to
         * disk as the `seg_idx`-th segment of the current tier.
         *
         * @param[in] buf_idx Index of the source segment buffer.
         * @param[in] seg_idx Index of the segment.
         *
         * @return `kSuccess` on success,
         *         `kIllegalArgument` if `buf_idx` is not active,
         *         `kFileSystemError` if failed to write to disk, or
         *         other non-zero error code otherwise.
         */
        int (*Flush)(int buf_idx, int seg_idx);

        /**
         * @brief Deallocates all active segment buffers.
         *
         * @return `kSuccess` on success, or
         *         other non-zero error code otherwise.
         */
        int (*FreeBuffers)(void);

        /**
         * @brief Returns the value of the position at the given `offset`
         * relative to the first position in the segment currently loaded in the
         * `buf_idx` -th segment buffer.
         *
         * @param[in] buf_idx Index of the segment buffer, which is assumed to
         * be active.
         * @param[in] offset Position offset relative to the first position in
         * the segment loaded in the target segment buffer. The offset is
         * assumed to be valid.
         *
         * @return Value of the position at the given `offset` in the target
         * segment buffer.
         */
        Value (*GetValue)(int buf_idx, int64_t offset);

        /**
         * @brief Returns the remoteness of the position at the given `offset`
         * relative to the first position in the segment currently loaded in the
         * `buf_idx` -th segment buffer.
         *
         * @param[in] buf_idx Index of the segment buffer, which is assumed to
         * be active.
         * @param[in] offset Position offset relative to the first position in
         * the segment loaded in the target segment buffer. The offset is
         * assumed to be valid.
         *
         * @return Remoteness of the position at the given `offset` in the
         * target segment buffer.
         */
        int (*GetRemoteness)(int buf_idx, int64_t offset);

        /**
         * @brief Sets the value and remoteness of the position at the given
         * `offset` relative to the first position in the segment currently
         * loaded in the `buf_idx` -th segment buffer.
         *
         * @param[in] buf_idx Index of the segment buffer, which is assumed to
         * be active.
         * @param[in] offset Position offset relative to the first position in
         * the segment loaded in the target segment buffer. The offset is
         * assumed to be valid.
         * @param[in] value New value.
         * @param[in] remoteness New remoteness.
         */
        void (*SetValueRemoteness)(int buf_idx, int64_t offset, Value value,
                                   int remoteness);

        /**
         * @brief Merges all `num_segments` segments on disk into the normal
         * Database archive. The output is equivalent to the output of
         * `Database`::FlushSolvingTier in the non-segmented methods.
         *
         * @param[in] tier_size Number of positions in the solving tier.
         * @param[in] num_segments Total number of segments, where all segments
         * are assumed to have been solved and flushed to disk.
         *
         * @return `kSuccess` on success,
         *         `kFileSystemError` if any file operation such as reading a
         * segment or saving the output failed, or
         *         other non-zero error code otherwise.
         */
        int (*Consolidate)(int64_t tier_size, int num_segments);
    } segmentation;

    /**
     * @brief Returns whether there exists a checkpoint for `tier`. A
     * checkpoint can be used to restore the solving progress of a tier.
     *
     * @param[in] tier Target tier.
     */
    bool (*CheckpointExists)(Tier tier);

    /**
     * @brief Saves a checkpoint for the current solving tier, including the
     * current solving `status`, overwriting any existing checkpoint.
     *
     * @param[out] status Pointer to data that stores the current solving
     * status.
     * @param[in] status_size Size of `status` in bytes.
     *
     * @return `kSuccess` on success, or
     *         non-zero error code otherwise.
     */
    int (*CheckpointSave)(const void *status, size_t status_size);

    /**
     * @brief Creates an in-memory DB for solving of the given `tier` of size
     * `size` by loading its checkpoint and previous solving status. Does
     * nothing and returns an error if a checkpoint cannot be found for `tier`.
     *
     * @param[in] tier Tier to be initialized and loaded.
     * @param[in] size Size of `tier` in number of positions.
     * @param[out] status (Output parameter) Pointer to a buffer of size at
     * least \p status_size bytes which will be used to load the solving status
     * from the checkpoint. Must be of the same format and size as used when the
     * checkpoint was saved with `Database`::CheckpointSave.
     * @param[in] status_size Size of `status` in bytes.
     *
     * @return `kSuccess` on success, or
     *         non-zero error code otherwise.
     */
    int (*CheckpointLoad)(Tier tier, int64_t size, void *status,
                          size_t status_size);

    /**
     * @brief Removes the checkpoint for `tier` if exists.
     *
     * @param[in] tier Remove the checkpoint for this tier.
     *
     * @return `kSuccess` on success,
     *         `kFileSystemError` if no checkpoint is found for `tier`, or
     *         any other non-zero error code on failure.
     */
    int (*CheckpointRemove)(Tier tier);

    // Loading API

    /**
     * @brief Returns an upper bound, in bytes, on the amount of memory that
     * will be used to load `tier` of `size` positions.
     *
     * @param[in] tier Tier to be loaded.
     * @param[in] size Size of `tier` in number of positions.
     *
     * @return An upper bound on memory usage in bytes.
     */
    size_t (*TierMemUsage)(Tier tier, int64_t size);

    /**
     * @brief Returns an upper bound, in bytes, on the amount of memory that
     * will be used to store a concurrent solving tier `tier` of `size`
     * positions.
     *
     * @param[in] tier Concurrent solving tier to create in memory.
     * @param[in] size Size of `tier` in number of positions.
     *
     * @return An upper bound on memory usage in bytes.
     */
    size_t (*ConcurrentTierMemUsage)(Tier tier, int64_t size);

    /**
     * @brief Loads the given `tier` of `size` positions into memory.
     *
     * @param[in] tier Tier to be loaded.
     * @param[in] size Size of `tier` in number of positions.
     *
     * @return `kSuccess` on success, or
     *         non-zero error code otherwise.
     */
    int (*LoadTier)(Tier tier, int64_t size);

    /**
     * @brief Unloads the given `tier` from memory if it was previously loaded
     * via Database::LoadTier.
     *
     * @note `kIllegalArgumentError` will be returned if this function is used
     * to unload the solving tier.
     *
     * @param[in] tier Target tier.
     *
     * @return `kSuccess` on success,
     *         `kIllegalArgumentError` if `tier` is the solving tier, which
     * can only be unloaded using Database::FreeSolvingTier, or
     *         any other non-zero error code otherwise.
     */
    int (*UnloadTier)(Tier tier);

    /**
     * @brief Returns `true` if `tier` has been loaded via Database::LoadTier
     * or if `tier` is the solving tier and has been created via
     * Database::CreateSolvingTier, Database::CreateConcurrentSolvingTier, or
     * Database::CheckpointLoad. Returns `false` otherwise.
     *
     * @param[in] tier Target tier.
     *
     * @retval `true` if `tier` has been loaded,
     * @retval `false` otherwise.
     */
    bool (*IsTierLoaded)(Tier tier);

    /**
     * @brief Returns the value of position `position` in tier `tier` if
     * `tier` has been created as the solving tier or if `tier` has been
     * loaded via Database::LoadTier. Returns `kErrorValue` otherwise.
     *
     * @param[in] tier A loaded tier.
     * @param[in] position Query the value of this position.
     *
     * @return The value of `position` in `tier` on success, or
     *         `kErrorValue` otherwise.
     */
    Value (*GetValueFromLoaded)(Tier tier, Position position);

    /**
     * @brief Returns the remoteness of position `position` in tier `tier` if
     * `tier` has been created as the solving tier or if `tier` has been
     * loaded via Database::LoadTier. Returns `kErrorRemoteness` otherwise.
     *
     * @param[in] tier A loaded tier.
     * @param[in] position Query the remoteness of this position.
     *
     * @return The remoteness of `position` in `tier` on success, or
     *         `kErrorRemoteness` otherwise.
     */
    int (*GetRemotenessFromLoaded)(Tier tier, Position position);

    // Probing API

    /**
     * @brief Initializes the given Database `probe`.
     *
     * @param[in,out] probe Target probe.
     *
     * @return `kSuccess` on success,
     *         non-zero error code otherwise.
     */
    int (*ProbeInit)(DbProbe *probe);

    /**
     * @brief Frees the given Database `probe`.
     *
     * @param[in,out] probe Target probe.
     *
     * @return `kSuccess` on success,
     *         non-zero error code otherwise.
     */
    int (*ProbeDestroy)(DbProbe *probe);

    /**
     * @brief Probes the value of `tier_position` from permanent storage using
     * `probe` and returns it.
     *
     * @note Assumes `probe` is initialized. Results in undefined behavior if
     * `probe` is NULL or uninitialized.
     *
     * @param[in,out] probe Database probe initialized using the ProbeInit()
     * function.
     * @param[in] tier_position Probe the value of this TierPosition.
     *
     * @return Value of `tier_position` probed from permanent storage, or
     * `kErrorValue` if `tier_position` is not found.
     */
    Value (*ProbeValue)(DbProbe *probe, TierPosition tier_position);

    /**
     * @brief Probes the remoteness of `tier_position` from permanent storage
     * using `probe` and returns it.
     *
     * @note Assumes `probe` is initialized. Results in undefined behavior if
     * `probe` is NULL or uninitialized.
     *
     * @param[in,out] probe Database probe initialized using the ProbeInit()
     * function.
     * @param[in] tier_position Probe the remoteness of this TierPosition.
     *
     * @return Remoteness of `tier_position` probed from permanent storage, or
     * -1 if `tier_position` is not found.
     */
    int (*ProbeRemoteness)(DbProbe *probe, TierPosition tier_position);

    /**
     * @brief Probes the current data path and returns the solving status of the
     * given `tier`.
     *
     * @param[in] tier The tier to check status of.
     *
     * @retval `kDbTierStatusSolved` if solved,
     * @retval `kDbTierStatusCorrupted` if corrupted,
     * @retval `kDbTierStatusMissing` if not solved, or
     * @retval `kDbTierStatusCheckError` if an error occurred when checking the
     * status of `tier`.
     */
    int (*TierStatus)(Tier tier);

    /**
     * @brief Probes the current data path and returns the solving status of the
     * current game.
     *
     * @retval `kDbGameStatusSolved` if solved,
     * @retval `kDbGameStatusIncomplete` if not fully solved, or
     * @retval `kDbGameStatusCheckError` if an error occurred when checking the
     * status of the current game.
     */
    int (*GameStatus)(void);

    /**
     * @brief Returns the path the Database is initialized with.
     *
     * @return The path the Database is initialized with, or
     *         `NULL` if the Database is not initialized.
     */
    const char *(*GetPath)(void);
} Database;

#endif  // GAMESMANONE_CORE_TYPES_DATABASE_DATABASE_H_
