/**
 * @file database.h
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Generic `Database` type and associated constants. A `Database` is an
 * abstract type of a database. An implementation provides names and function
 * pointers for the operations it supports. Only `Init` and `Finalize` are
 * required; callers must check every other function pointer before calling it.
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
 * @brief Constants associated with the `Database` type.
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
 * @brief Generic Tier `Database` type.
 *
 * @note Both name fields, `Init`, and `Finalize` are required. Every other
 * function pointer may be `NULL`. Callers must check that each operation they
 * use is supported, including setup, observation, and cleanup operations.
 * @note All Databases are Tier Databases because the game designer does not
 * interact with a `Database` directly.
 * @note Only one `Database` implementation may be initialized at a time within
 * a process. Its lifetime starts with `Init` and ends with `Finalize`. Calling
 * `Init` again without first calling `Finalize` results in undefined behavior.
 * @note Newly allocated in-memory database fields are unknown until explicitly
 * written using the provided methods, such as `SetValue`, `SetRemoteness`, or
 * `SetValueRemoteness`. Writing one field does not initialize the other. A
 * successful load restores the fields saved in the corresponding archive,
 * checkpoint, or segment; it does not establish defaults for unwritten fields.
 * @note All five legal Value enumerators are supported. Every implementation
 * must support remotenesses from zero through `kRemotenessMax`, inclusive, as
 * defined in core/constants.h. An implementation may document a larger range.
 * Any legal value-remoteness combination, including `kUndecided` for
 * unreachable positions, may appear in a completed tier archive.
 * @note Valid tier identifiers, position indices, and remoteness data are
 * nonnegative. Position indices must also be within the target tier's range
 * [0, size). Negative values, including `kIllegalTier`, `kIllegalPosition`, and
 * `kErrorRemoteness` from `core/constants.h`, are invalid data but remain
 * available as documented error or sentinel results. Their definitions are
 * unchanged. Callers must provide valid data unless a method explicitly
 * documents a checked error for an invalid argument. Size and count arguments
 * retain their method-specific validation rules.
 * @note A stored tier has a fixed position count. Its creation, checkpoint
 * restoration, and archive loading must use that same tier size. An
 * inconsistent size results in undefined behavior unless a method explicitly
 * documents a checked error.
 * @note The nine top-level Solving API record methods require an active
 * solving tier, a valid position, and legal supplied values and remotenesses.
 * Violating these record-access preconditions results in undefined behavior.
 * Loaded-tier and segmentation methods have their own documented prerequisites.
 * Unwritten fields have unspecified contents; callers must initialize fields
 * before relying on their values. Child counters have an implementation-defined
 * maximum; callers must consult that implementation's specification and stay
 * within its range.
 * @note Only one solving tier or segment-buffer context may be active at a
 * time. Read-only tiers loaded through `LoadTier` may coexist with that
 * context.
 * @note Record operations must not overlap lifecycle operations. Checkpoint
 * operations require writes to be paused. Probes access permanent storage
 * independently of in-memory solving and loaded tiers; this does not guarantee
 * concurrent archive replacement or concurrent access through a shared probe.
 * @note The checkpoint API supports only regular, externally synchronized
 * solving tiers. Using checkpoint operations with a concurrent solving tier
 * results in undefined behavior. `CheckpointLoad` always creates a regular
 * solving tier.
 * @note After an operation reports failure, the solver is expected to stop.
 * Unless explicitly stated, rollback, retry, and continued use of partially
 * modified state are not guaranteed. `Finalize` remains available for cleanup
 * subject to its no-live-probes requirement.
 */
typedef struct Database {
    /**
     * Internal name of the `Database`. Shall not be empty or contain any white
     * spaces.
     */
    char name[kDbNameLengthMax + 1];

    /**
     * Human-readable name of the `Database`. Shall not be empty.
     */
    char formal_name[kDbFormalNameLengthMax + 1];

    /**
     * @brief Initializes the `Database`.
     *
     * @param[in] game_name Internal name of the game.
     * @param[in] variant Index of the selected game variant.
     * @param[in] path Path to the directory to which the `Database` has full
     * access. The `Database` may choose to store files directly inside this
     * directory or make sub-directories as the DB designer sees fit.
     * @param[in] GetTierName Function that converts a Tier to its name. If set
     * to `NULL`, a fallback method will be used instead.
     * @param[in,out] aux Auxiliary parameter. Purpose defined by the `Database`
     * implementation. However, `NULL` shall always be accepted to use the
     * default settings.
     *
     * @pre No `Database` is currently initialized. Repeated initialization
     * without intervening `Finalize` results in undefined behavior.
     *
     * @return 0 on success, non-zero error code otherwise.
     */
    int (*Init)(ReadOnlyString game_name, int variant, ReadOnlyString path,
                GetTierNameFunc GetTierName, void *aux);

    /**
     * @brief Finalizes the `Database`, freeing all allocated space.
     *
     * @note This operation always completes successfully. It is a successful
     * no-op before initialization and after successful finalization, and is
     * also valid after a failed `Init`. It has no return value.
     *
     * @pre No initialized probe remains undestroyed and no other database
     * operation is in progress. Violating this requirement results in undefined
     * behavior.
     */
    void (*Finalize)(void);

    // Solving API

    /**
     * @brief Creates an in-memory DB for solving the given `tier` of size
     * `size` positions.
     *
     * @note A size of zero is allowed. Each record's value and remoteness are
     * initially unknown until the corresponding field is explicitly written.
     * Writing one field does not initialize the other.
     * @note This function is part of the Solving API.
     * @note In-memory databases created using this function are not thread-safe
     * and the following functions, when called on the same position, must be
     * synchronized externally:
     *
     *     - `SetValue`
     *
     *     - `SetRemoteness`
     *
     *     - `SetValueRemoteness`
     *
     *     - `MaximizeValueRemoteness`
     *
     *     - `DecrementNumUndecidedChildren`
     *
     *     - `ClearNumUndecidedChildren`
     *
     *     - `GetValue`
     *
     *     - `GetRemoteness`
     *
     *     - `GetNumUndecidedChildren`
     *
     * @param[in] tier Tier to be solved and stored in memory.
     * @param[in] size Non-negative size of `tier` in number of positions.
     *
     * @pre No solving tier or segment-buffer context is currently active.
     * Creating another solving context, including switching creation modes
     * without first freeing the current context, is undefined behavior.
     *
     * @return `kSuccess` on success,
     *         `kIllegalArgumentError` if `size` is negative,
     *         `kMallocFailureError` if the requested storage cannot be
     *         allocated, or any other non-zero error code otherwise.
     */
    int (*CreateSolvingTier)(Tier tier, int64_t size);

    /**
     * @brief Creates a multi-reader multi-writer concurrent in-memory DB for
     * solving the given `tier` of size `size` positions.
     *
     * @note A size of zero is allowed. Initial field contents are unknown, as
     * described for `Database::CreateSolvingTier`.
     * @note Each call to the nine record operations listed in
     * `Database::CreateSolvingTier` is atomic, including calls on the same
     * position. Counter condition checks and updates form one operation;
     * comparing and replacing a value-remoteness pair also forms one
     * operation. Overlapping calls have no predetermined ordering.
     * @note Separate calls to `GetValue` and `GetRemoteness` remain separate
     * operations and do not provide a value-remoteness pair snapshot while
     * another thread writes the position.
     * @note Creation, release, and other lifecycle operations must not overlap
     * record access.
     * @note This function is part of the Solving API.
     *
     * @param[in] tier Tier to be solved and stored in memory.
     * @param[in] size Non-negative size of `tier` in number of positions.
     *
     * @pre No solving tier or segment-buffer context is currently active.
     * Creating another solving context or switching creation modes before
     * freeing it is undefined behavior.
     *
     * @return `kSuccess` on success,
     *         `kIllegalArgumentError` if `size` is negative,
     *         `kMallocFailureError` if the requested storage cannot be
     *         allocated, or any other non-zero error code otherwise.
     */
    int (*CreateConcurrentSolvingTier)(Tier tier, int64_t size);

    /**
     * @brief Flushes the in-memory DB to disk.
     *
     * @note This operation is idempotent: repeating a successful flush without
     * changing the records preserves the same logical archive. Each call may
     * still fail if an operational error occurs.
     * @note All supported value-remoteness pairs may be stored, including
     * `kUndecided` records for unreachable positions in a solved tier.
     * @note This function is part of the Solving API.
     *
     * @param[in,out] aux Auxiliary parameter. `NULL` is accepted to select the
     * default behavior.
     *
     * @pre A solving tier exists. Calling without one is undefined behavior.
     *
     * @return `kSuccess` on success,
     *         non-zero error code otherwise.
     */
    int (*FlushSolvingTier)(void *aux);

    /**
     * @brief Frees the in-memory solving tier.
     *
     * @note If no solving tier exists, this function does nothing and returns
     * `kSuccess`. This includes repeated calls after a successful release.
     * @note This function is part of the Solving API.
     *
     * @return `kSuccess` on success,
     *         non-zero error code otherwise.
     */
    int (*FreeSolvingTier)(void);

    /**
     * @brief Sets the current game as solved.
     *
     * @note This is a declaration by the caller, who is responsible for
     * determining whether the game is solved. A successful call causes
     * `Database::GameStatus` to report `kDbGameStatusSolved`.
     * @note This operation is idempotent. Repeated declarations preserve the
     * solved status, but a call may still fail if an operational error occurs.
     *
     * @return `kSuccess` on success, or
     *         non-zero error code otherwise.
     */
    int (*SetGameSolved)(void);

    /**
     * @brief Sets the value of `position` to `value`.
     *
     * @note The remoteness and all other positions are unchanged.
     * @note This function is part of the Solving API.
     *
     * @param[in] position Target position.
     * @param[in] value Target value.
     *
     * @pre The record-access preconditions of `Database` apply: a solving tier
     * exists, `position` belongs to it, and `value` is supported. Violating
     * these preconditions is undefined behavior.
     *
     * @return `kSuccess` on success,
     *         non-zero error code otherwise.
     */
    int (*SetValue)(Position position, Value value);

    /**
     * @brief Sets the remoteness of `position` to `remoteness`.
     *
     * @note The value and all other positions are unchanged.
     * @note This function is part of the Solving API.
     *
     * @param[in] position Target position.
     * @param[in] remoteness Target remoteness.
     *
     * @pre The record-access preconditions of `Database` apply: a solving tier
     * exists, `position` belongs to it, and `remoteness` is supported.
     * Violating these preconditions is undefined behavior.
     *
     * @return `kSuccess` on success,
     *         non-zero error code otherwise.
     */
    int (*SetRemoteness)(Position position, int remoteness);

    /**
     * @brief Sets the `value` and `remoteness` of `position`.
     *
     * @note All other positions are unchanged.
     * @note This function is part of the Solving API.
     *
     * @param[in] position Target position.
     * @param[in] value Target value.
     * @param[in] remoteness Target remoteness.
     *
     * @pre The record-access preconditions of `Database` apply: a solving tier
     * exists, `position` belongs to it, and both supplied fields are supported.
     * Violating these preconditions is undefined behavior.
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
     * @note If the comparator returns zero, no change is made, even if the two
     * pairs have different fields. All other positions are unchanged.
     * @note The comparison must define a consistent ordering, including stable
     * equivalence between equally ranked pairs. It must not reenter the
     * `Database`. During concurrent solving, the comparison and any state it
     * accesses must be safe for simultaneous calls from multiple threads.
     *
     * @param[in] position Position to update.
     * @param[in] value Candidate value.
     * @param[in] remoteness Candidate remoteness.
     * @param[in] compare Pointer to a value-remoteness pair comparison function
     * that takes in two value-remoteness pairs (v1, r1) and (v2, r2) and
     * returns a negative integer if (v1, r1) < (v2, r2), a positive integer if
     * (v1, r1) > (v2, r2), or zero if they are equal.
     *
     * @pre The record-access preconditions of `Database` apply, and `compare`
     * is a non-null comparison function. Invalid fields or positions, an
     * absent solving tier, or a null comparator result in undefined behavior.
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
     * @note Every implementation supports counts from zero through
     * `kRemotenessMax`. Its maximum supported counter is implementation
     * defined; callers must consult that implementation's specification before
     * storing larger counts. When the value is `kUndecided`, negative or
     * unsupported counts result in undefined behavior.
     * @note The value and all other positions are unchanged.
     *
     * @param[in] position Target position.
     *
     * @pre A solving tier exists and `position` belongs to it. Violating these
     * record-access preconditions is undefined behavior.
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
     * @note The value and all other positions are unchanged.
     *
     * @param[in] position Target position.
     *
     * @pre A solving tier exists and `position` belongs to it. If its value is
     * `kUndecided`, its counter must be within the implementation's supported
     * range; see `Database::DecrementNumUndecidedChildren`. Violating these
     * preconditions is undefined behavior.
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
     * @pre A solving tier exists and `position` belongs to it. Violating these
     * record-access preconditions is undefined behavior.
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
     * @pre A solving tier exists and `position` belongs to it. Violating these
     * record-access preconditions is undefined behavior.
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
     * @pre A solving tier exists and `position` belongs to it. If its value is
     * `kUndecided`, its counter must be within the implementation's supported
     * range; see `Database::DecrementNumUndecidedChildren`. Violating these
     * preconditions is undefined behavior.
     *
     * @return The number of undecided children of `position` if its value is
     * `kUndecided`, or
     *         0 otherwise.
     */
    int (*GetNumUndecidedChildren)(Position position);

    /**
     * Segmentation API supports creating, saving, and loading of
     * database segments in memory, which can be used if the solving algorithm
     * supports streaming, greatly reducing the amount of memory required to
     * load the entire database at once.
     *
     * `CreateBuffers` establishes the current tier and segment size. This
     * context remains active until `FreeBuffers` is called. `Consolidate` must
     * be called before `FreeBuffers` and within the same initialized
     * `Database` lifetime; consolidation cannot resume after reinitialization.
     *
     * For a tier with `n` segments, segment indices are consecutive integers
     * from 0 through `n - 1`. Buffer indices and segment indices are
     * independent.
     * If the segment size is `S`, offset `offset` in segment `seg_idx`
     * corresponds to tier position `seg_idx * S + offset`. Other segment
     * indices result in undefined behavior, except that `Load` and `Flush`
     * return `kIllegalArgumentError` for negative indices.
     *
     * Newly created buffer contents are unknown until explicitly written or
     * loaded. Offsets corresponding to tier positions at or beyond `tier_size`
     * are outside the logical tier and remain unknown. For an empty tier this
     * includes every offset in every segment. Accessing such offsets through
     * the getters or setter results in undefined behavior; `Consolidate`
     * ignores them.
     */
    struct {
        /**
         * @brief Returns the maximum number of segment buffers supported by the
         * `Database`.
         *
         * @note The maximum may be zero.
         *
         * @return Maximum number of segment buffers supported.
         */
        int (*MaxNumBuffers)(void);

        /**
         * @brief Makes `num_buffers` segment buffers available for solving the
         * given `tier`, where each segment is of `size` positions.
         *
         * @note Zero buffers and zero-sized segments are valid. Newly created
         * buffer contents are unknown until explicitly written or loaded.
         *
         * @param[in] tier Tier to be solved.
         * @param[in] num_buffers Number of segment buffers to create.
         * @param[in] size Number of positions in each segment.
         *
         * @pre No solving tier or segment-buffer context is active. Calling
         * this function before freeing an existing context results in
         * undefined behavior.
         *
         * @return `kSuccess` on success,
         *         `kIllegalArgumentError` if `num_buffers` or `size` is
         *         negative, or `num_buffers` exceeds `MaxNumBuffers()`,
         *         `kMallocFailureError` on memory allocation failure, or
         *         other non-zero error code otherwise.
         */
        int (*CreateBuffers)(Tier tier, int num_buffers, int64_t size);

        /**
         * @brief Loads the `seg_idx`-th segment from disk into the `buf_idx`-th
         * segment buffer.
         *
         * @note Loading into an active buffer overwrites its previous contents.
         * @note An inactive buffer returns `kIllegalArgumentError` even before
         * buffer creation or after `FreeBuffers`, without using tier metadata.
         *
         * @param[in] buf_idx Index of the destination segment buffer.
         * @param[in] seg_idx Index of the segment to load.
         *
         * @return `kSuccess` on success,
         *         `kIllegalArgumentError` if `buf_idx` or `seg_idx` is negative
         *         or if `buf_idx` is not active,
         *         `kFileSystemError` if `seg_idx` does not exist on disk or
         * failed to read the segment from disk, or
         *         other non-zero error code otherwise.
         */
        int (*Load)(int buf_idx, int seg_idx);

        /**
         * @brief Flushes the contents of the `buf_idx`-th segment buffer to
         * disk as the `seg_idx`-th segment of the current tier.
         *
         * @note A successful flush overwrites any existing saved segment at
         * `seg_idx`. Later loading and consolidation use the new contents;
         * replacing a segment does not add another segment index.
         * @note An inactive buffer returns `kIllegalArgumentError` even before
         * buffer creation or after `FreeBuffers`, without using tier metadata.
         *
         * @param[in] buf_idx Index of the source segment buffer.
         * @param[in] seg_idx Index of the segment.
         *
         * @return `kSuccess` on success,
         *         `kIllegalArgumentError` if `buf_idx` or `seg_idx` is negative
         *         or if `buf_idx` is not active,
         *         `kFileSystemError` if failed to write to disk, or
         *         other non-zero error code otherwise.
         */
        int (*Flush)(int buf_idx, int seg_idx);

        /**
         * @brief Deallocates all active segment buffers and clears the current
         * tier and segment-size context.
         *
         * @note Does nothing and returns `kSuccess` if no segment-buffer
         * context exists. This includes calls before creation and repeated
         * calls after a successful release. `Consolidate` must be called before
         * freeing the context.
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
         * @note Contents not yet explicitly written or loaded are unknown.
         *
         * @param[in] buf_idx Index of the segment buffer, which is assumed to
         * be active.
         * @param[in] offset Position offset relative to the first position in
         * the segment loaded in the target segment buffer. The offset is
         * assumed to be valid and within the logical tier. Invalid or
         * outside-tier indices result in undefined behavior.
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
         * @note Contents not yet explicitly written or loaded are unknown.
         *
         * @param[in] buf_idx Index of the segment buffer, which is assumed to
         * be active.
         * @param[in] offset Position offset relative to the first position in
         * the segment loaded in the target segment buffer. The offset is
         * assumed to be valid and within the logical tier. Invalid or
         * outside-tier indices result in undefined behavior.
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
         * assumed to be valid and within the logical tier. Invalid or
         * outside-tier indices result in undefined behavior.
         * @param[in] value New value.
         * @param[in] remoteness New remoteness.
         */
        void (*SetValueRemoteness)(int buf_idx, int64_t offset, Value value,
                                   int remoteness);

        /**
         * @brief Merges all `num_segments` segments on disk into the normal
         * `Database` archive. The output is equivalent to the output of
         * `Database::FlushSolvingTier` in the non-segmented methods.
         *
         * @note Segment indices must be 0 through `num_segments - 1`. Their
         * contents are concatenated in index order, and records at or beyond
         * `tier_size` are ignored. Any legal value-remoteness pair, including
         * `kUndecided`, may be stored.
         * @note For an empty tier, every nonnegative segment count and segment
         * capacity is a valid shape, including zero. No prior segment flushes
         * or materialized segment files are required. Success produces a
         * normal archive representing zero positions and causes `TierStatus`
         * to report `kDbTierStatusSolved`. Internal file representation is
         * implementation-defined.
         * @note For a nonempty tier, the segment size must be positive and
         * `num_segments` must equal the tier size divided by the segment size,
         * rounded up. Other nonnegative nonempty-tier shapes result in
         * undefined behavior.
         *
         * @param[in] tier_size Number of positions in the solving tier.
         * @param[in] num_segments Total number of segments. For a nonempty
         * tier, all logical segments must have been solved and saved through
         * `Flush`. This prerequisite does not apply to an empty tier.
         *
         * @pre The context established by `CreateBuffers` in the current
         * initialized `Database` lifetime remains active. Calling after
         * `FreeBuffers` or resuming consolidation after reinitialization
         * results in undefined behavior.
         *
         * @return `kSuccess` on success,
         *         `kIllegalArgumentError` if `tier_size` or `num_segments` is
         *         negative,
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
     * @note All writes to the solving tier must be paused during checkpoint
     * operations. Overlapping a checkpoint operation with such writes results
     * in undefined behavior.
     *
     * @param[in] tier Target tier.
     */
    bool (*CheckpointExists)(Tier tier);

    /**
     * @brief Saves a checkpoint for the current solving tier, including the
     * current solving `status`, overwriting any existing checkpoint.
     *
     * @note Saving a checkpoint does not affect the tier's solved status.
     * @note All writes to the solving tier must be paused during this
     * operation; overlapping writes result in undefined behavior.
     * @note This function may be called when no solving tier exists, but
     * success is not guaranteed in that case. Either `kSuccess` or a non-zero
     * error code may be returned normally.
     *
     * @param[in] status Pointer to the current solving status, which is not
     * modified. When `status_size` is zero, either a `NULL` or non-`NULL`
     * pointer is permitted, but success is not guaranteed.
     * @param[in] status_size Size of `status` in bytes.
     *
     * @pre If a solving tier exists, it is a regular tier created through
     * `Database::CreateSolvingTier` or restored through
     * `Database::CheckpointLoad`. Saving a concurrent solving tier results in
     * undefined behavior.
     *
     * @return `kSuccess` on success,
     *         `kIllegalArgumentError` if `status` is `NULL` and `status_size`
     *         is positive, or
     *         non-zero error code otherwise.
     */
    int (*CheckpointSave)(const void *status, size_t status_size);

    /**
     * @brief Creates an in-memory DB for solving of the given `tier` of size
     * `size` by loading its checkpoint and previous solving status. Does
     * nothing and returns an error if a checkpoint cannot be found for `tier`.
     *
     * @note The restored solving tier uses the externally synchronized mode
     * of `Database::CreateSolvingTier`. Concurrent solving tiers are not
     * supported by the checkpoint API.
     * @note Calling this function while a solving tier or segment-buffer
     * context exists results in undefined behavior.
     * @note All writes to the solving tier must be paused during checkpoint
     * operations; overlapping writes result in undefined behavior.
     *
     * @param[in] tier Tier to be initialized and loaded.
     * @param[in] size Size of `tier` in number of positions. Must match the
     * size used to create the checkpointed solving tier.
     * @param[out] status (Output parameter) Pointer to a buffer of size at
     * least \p status_size bytes which will be used to load the solving status
     * from the checkpoint. For a supplied nonempty status buffer, the format
     * and size must match those used with `Database::CheckpointSave`. May be
     * `NULL` when `status_size` is zero; this combination is permitted, but
     * success is not guaranteed.
     * @param[in] status_size Size of `status` in bytes.
     *
     * @pre The tier size matches the size originally supplied to
     * `Database::CreateSolvingTier`. A mismatch results in undefined behavior.
     *
     * @return `kSuccess` on success,
     *         `kIllegalArgumentError` if `status` is `NULL` and `status_size`
     *         is positive, or if `status` is non-`NULL` and `status_size` is
     *         zero, or
     *         non-zero error code otherwise.
     */
    int (*CheckpointLoad)(Tier tier, int64_t size, void *status,
                          size_t status_size);

    /**
     * @brief Removes the checkpoint for `tier` if exists.
     *
     * @note All writes to the solving tier must be paused during this
     * operation; overlapping writes result in undefined behavior.
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
     * @brief Returns an upper bound, in bytes, on peak memory usage for loading
     * `tier` of `size` positions using `Database::LoadTier` or creating it
     * using `Database::CreateSolvingTier`.
     *
     * @note Zero-sized tiers are allowed. The estimate bounds peak usage,
     * including temporary memory needed by the operation, rather than only
     * the retained tier data. No particular allocation formula is required.
     *
     * @param[in] tier Tier to be loaded or created.
     * @param[in] size Non-negative size of `tier` in number of positions.
     *
     * @return An upper bound on peak memory usage in bytes, or `SIZE_MAX` if
     * `tier` or `size` is negative, the estimate overflows, or any
     * other error occurs.
     */
    size_t (*TierMemUsage)(Tier tier, int64_t size);

    /**
     * @brief Returns an upper bound, in bytes, on peak memory usage for
     * creating concurrent solving tier `tier` of `size` positions using
     * `Database::CreateConcurrentSolvingTier`.
     *
     * @note Zero-sized tiers are allowed. The estimate includes temporary
     * memory needed by the operation, rather than only retained tier data.
     * No particular allocation formula is required.
     *
     * @param[in] tier Concurrent solving tier to create in memory.
     * @param[in] size Non-negative size of `tier` in number of positions.
     *
     * @return An upper bound on peak memory usage in bytes, or `SIZE_MAX` if
     * `tier` or `size` is negative, the estimate overflows, or any
     * other error occurs.
     */
    size_t (*ConcurrentTierMemUsage)(Tier tier, int64_t size);

    /**
     * @brief Loads the given `tier` of `size` positions into memory.
     *
     * @note Calling this function for a tier already loaded via
     * `Database::LoadTier` results in undefined behavior.
     * @note `size` must match the size of the stored tier. A mismatch results
     * in undefined behavior.
     *
     * @param[in] tier Tier to be loaded.
     * @param[in] size Size of `tier` in number of positions.
     *
     * @return `kSuccess` on success,
     *         `kRuntimeError` if the tier's database file is missing or
     *         corrupted,
     *         `kIllegalArgumentError` if `tier` is the current solving tier, or
     *         non-zero error code otherwise.
     */
    int (*LoadTier)(Tier tier, int64_t size);

    /**
     * @brief Unloads the given `tier` from memory if it was previously loaded
     * via `Database::LoadTier`.
     *
     * @note `kIllegalArgumentError` will be returned if this function is used
     * to unload the solving tier or a tier that is not resident in memory.
     *
     * @param[in] tier Target tier.
     *
     * @return `kSuccess` on success,
     *         `kIllegalArgumentError` if `tier` is the solving tier, which
     *         can only be unloaded using `Database::FreeSolvingTier`, or if
     *         `tier` is not resident in memory, or
     *         any other non-zero error code otherwise.
     */
    int (*UnloadTier)(Tier tier);

    /**
     * @brief Returns `true` if `tier` has been loaded via `Database::LoadTier`
     * or if `tier` is the solving tier and has been created via
     * `Database::CreateSolvingTier`, `Database::CreateConcurrentSolvingTier`,
     * or
     * `Database::CheckpointLoad`. Returns `false` otherwise.
     *
     * @param[in] tier Target tier.
     *
     * @retval `true` if `tier` has been loaded,
     * @retval `false` otherwise.
     */
    bool (*IsTierLoaded)(Tier tier);

    /**
     * @brief Returns the value of position `position` in tier `tier` if
     * `tier` has been created as the solving tier via
     * `Database::CreateSolvingTier`, `Database::CreateConcurrentSolvingTier`,
     * or
     * `Database::CheckpointLoad`, or loaded via `Database::LoadTier`. Returns
     * `kErrorValue` otherwise.
     *
     * @note For the solving tier, this function has the same concurrency
     * guarantees as `Database::GetValue`. It is safe alongside concurrent
     * solving-tier readers and writers when the tier was created via
     * `Database::CreateConcurrentSolvingTier`.
     * @note If `tier` is resident, `position` must be in the range
     * [0, size), where size is the tier's position count. An invalid position
     * results in undefined behavior.
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
     * `tier` has been created as the solving tier via
     * `Database::CreateSolvingTier`, `Database::CreateConcurrentSolvingTier`,
     * or
     * `Database::CheckpointLoad`, or loaded via `Database::LoadTier`. Returns
     * `kErrorRemoteness` otherwise.
     *
     * @note For the solving tier, this function has the same concurrency
     * guarantees as `Database::GetRemoteness`. It is safe alongside concurrent
     * solving-tier readers and writers when the tier was created via
     * `Database::CreateConcurrentSolvingTier`.
     * @note If `tier` is resident, `position` must be in the range
     * [0, size), where size is the tier's position count. An invalid position
     * results in undefined behavior.
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
     * @brief Initializes the given `Database` `probe`.
     *
     * @note Reinitializing an initialized probe results in undefined behavior.
     * Call `Database::ProbeDestroy` before initializing the same probe again.
     * @note A `NULL` probe is rejected with `kIllegalArgumentError`.
     *
     * @param[in,out] probe Target probe.
     *
     * @return `kSuccess` on success,
     *         `kIllegalArgumentError` if `probe` is `NULL`, or
     *         non-zero error code otherwise.
     */
    int (*ProbeInit)(DbProbe *probe);

    /**
     * @brief Frees the given `Database` `probe`.
     *
     * @note After successful destruction, repeated calls on the same probe
     * succeed until it is initialized again.
     * @note A `NULL` probe is accepted as a no-op and returns `kSuccess`.
     * @note An initialized probe must be destroyed even after a read reports
     * an error. Such cleanup is permitted; the caller must destroy the probe
     * before `Database::Finalize` rather than retrying or continuing reads.
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
     * `probe` is `NULL` or uninitialized.
     * @note If the tier's database file exists, it must not be corrupted and
     * the position must be valid for that tier. Reading a corrupted archive or
     * an invalid position results in undefined behavior. A missing tier is
     * reported by returning `kErrorValue`.
     * @note Probes access permanent storage independently of solving and
     * loaded tiers. Overlap with database lifecycle operations results in
     * undefined behavior.
     * @note A reported read error does not destroy the probe. The caller must
     * use `Database::ProbeDestroy` for cleanup before finalization.
     *
     * @param[in,out] probe `Database` probe initialized using the ProbeInit()
     * function.
     * @param[in] tier_position Probe the value of this TierPosition.
     *
     * @return Value of `tier_position` probed from permanent storage, or
     *         `kErrorValue` if `tier_position` is not found or a read error
     *         occurs during a call satisfying the documented preconditions.
     */
    Value (*ProbeValue)(DbProbe *probe, TierPosition tier_position);

    /**
     * @brief Probes the remoteness of `tier_position` from permanent storage
     * using `probe` and returns it.
     *
     * @note Assumes `probe` is initialized. Results in undefined behavior if
     * `probe` is `NULL` or uninitialized.
     * @note If the tier's database file exists, it must not be corrupted and
     * the position must be valid for that tier. Reading a corrupted archive or
     * an invalid position results in undefined behavior. A missing tier is
     * reported by returning `kErrorRemoteness` from `core/constants.h`.
     * @note Probes access permanent storage independently of solving and
     * loaded tiers. Overlap with database lifecycle operations results in
     * undefined behavior.
     * @note A reported read error does not destroy the probe. The caller must
     * use `Database::ProbeDestroy` for cleanup before finalization.
     *
     * @param[in,out] probe `Database` probe initialized using the ProbeInit()
     * function.
     * @param[in] tier_position Probe the remoteness of this TierPosition.
     *
     * @return Remoteness of `tier_position` probed from permanent storage, or
     *         `kErrorRemoteness` if `tier_position` is not found, or if a read
     *         error occurs during a call satisfying the preconditions.
     */
    int (*ProbeRemoteness)(DbProbe *probe, TierPosition tier_position);

    /**
     * @brief Probes the current data path and returns the solving status of the
     * given `tier`.
     *
     * @note Checkpoints and saved segments do not mark a tier solved. If no
     * final archive exists, a tier with only a checkpoint or saved segment
     * files reports `kDbTierStatusMissing`, regardless of their completeness.
     * Saving a checkpoint does not alter the status of an existing archive.
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
     * @note The caller is responsible for determining game completion and
     * declaring it through `Database::SetGameSolved` when that operation is
     * supported. This interface does not supply a list of all game tiers.
     *
     * @retval `kDbGameStatusSolved` if solved,
     * @retval `kDbGameStatusIncomplete` if not fully solved, or
     * @retval `kDbGameStatusCheckError` if an error occurred when checking the
     * status of the current game.
     */
    int (*GameStatus)(void);

    /**
     * @brief Returns the path the `Database` is initialized with.
     *
     * @return The path the `Database` is initialized with, or
     *         `NULL` if the `Database` is not initialized.
     */
    const char *(*GetPath)(void);
} Database;

#endif  // GAMESMANONE_CORE_TYPES_DATABASE_DATABASE_H_
