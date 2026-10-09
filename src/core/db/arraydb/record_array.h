/**
 * @file record_array.h
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Fixed-length `Record` array for the Array Database.
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

#ifndef GAMESMANONE_CORE_DB_ARRAYDB_RECORD_ARRAY_H_
#define GAMESMANONE_CORE_DB_ARRAYDB_RECORD_ARRAY_H_

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "core/db/arraydb/record.h"
#include "core/gamesman_memory.h"

/** Fixed-length `Record` array. */
typedef struct RecordArray {
    int64_t size;     /**< Number of `Record`s. */
    Record records[]; /**< Array of `Record`s. */
} RecordArray;

/**
 * @brief Creates a new `RecordArray` of `size` elements.
 *
 * @param[in] size Size of the new array in number of `Record`s.
 *
 * @returns Pointer to the new `RecordArray` on success, or `NULL` on malloc
 * failure.
 */
static inline RecordArray *RecordArrayCreate(int64_t size) {
    RecordArray *ret =
        (RecordArray *)GamesmanMalloc(sizeof(int64_t) + size * sizeof(Record));
    if (!ret) {
        return NULL;
    }

    ret->size = size;
    memset(ret->records, 0, size * sizeof(Record));

    return ret;
}

/**
 * @brief Deallocates the `array`.
 *
 * @param[in,out] array Array to deallocate.
 */
static inline void RecordArrayDestroy(RecordArray *array) {
    GamesmanFree(array);
}

/**
 * @brief Sets the value of `position` in `array` to `val`. Assumes
 * `position` is greater than or equal to 0 and smaller than the size of
 * `array`.
 *
 * @param[in,out] array Target array.
 * @param[in] position Position.
 * @param[in] val New `Value` for the `position`.
 */
static inline void RecordArraySetValue(RecordArray *array, Position position,
                                       Value val) {
    assert(position >= 0 && position < array->size);  // LCOV_EXCL_BR_LINE
    RecordSetValue(&array->records[position], val);
}

/**
 * @brief Sets the remoteness of `position` in `array` to
 * `remoteness`. Assumes `position` is greater than or equal to 0 and smaller
 * than the size of `array`.
 *
 * @param[in,out] array Target array.
 * @param[in] position Position.
 * @param[in] remoteness New remoteness for the `position`.
 */
static inline void RecordArraySetRemoteness(RecordArray *array,
                                            Position position, int remoteness) {
    assert(position >= 0 && position < array->size);  // LCOV_EXCL_BR_LINE
    RecordSetRemoteness(&array->records[position], remoteness);
}

/**
 * @brief Sets the value and remoteness of `position` in `array` to
 * `val` and `remoteness`, respectively. Assumes `position` is greater than
 * or equal to 0 and smaller than the size of `array`.
 *
 * @param[in,out] array Target array.
 * @param[in] position Position.
 * @param[in] val New `Value` for the `position`.
 * @param[in] remoteness New remoteness for the `position`.
 */
static inline void RecordArraySetValueRemoteness(RecordArray *array,
                                                 Position position, Value val,
                                                 int remoteness) {
    assert(position >= 0 && position < array->size);  // LCOV_EXCL_BR_LINE
    RecordSetValueRemoteness(&array->records[position], val, remoteness);
}

/**
 * @brief Replaces the value and remoteness of `position` in `array`
 * with the maximum of its original value-remoteness pair and the one provided
 * by `val` and `remoteness`. The order of value-remoteness pairs is
 * determined by the `compare` function.
 *
 * @param[in,out] array Target array.
 * @param[in] position Position.
 * @param[in] val Candidate new `Value` for the `position`.
 * @param[in] remoteness Candidate new remoteness for the `position`.
 * @param[in] compare Pointer to a value-remoteness pair comparison function
 * that takes in two value-remoteness pairs (v1, r1) and (v2, r2) and returns a
 * negative integer if (v1, r1) < (v2, r2), a positive integer if (v1, r1) >
 * (v2, r2), or zero if they are equal.
 *
 * @retval true If the provided `val`-`remoteness` pair is greater than
 * the original value-remoteness pair and the old pair is replaced.
 * @retval false Otherwise.
 */
static inline bool RecordArrayMaximize(RecordArray *array, Position position,
                                       Value val, int remoteness,
                                       int (*compare)(Value v1, int r1,
                                                      Value v2, int r2)) {
    assert(position >= 0 && position < array->size);  // LCOV_EXCL_BR_LINE
    return RecordMaximize(&array->records[position], val, remoteness, compare);
}

/**
 * @brief Subtracts one from the number of undecided children of `position`
 * in `array` and returns the number of undecided children
 * immediately preceding the subtraction if the value field of `position` is
 * set to `kUndecided` and the number of undecided children is at least one.
 * Does nothing and returns 0 if the value field of `position` is not
 * `kUndecided` or the number of undecided children is zero.
 *
 * @note The remoteness field of a `Record` is overloaded as the counter for the
 * number of undecided children of that position when its value is `kUndecided`.
 *
 * @param[in,out] array Target array.
 * @param[in] position Position.
 *
 * @returns Number of undecided children immediately preceding the subtraction,
 * or 0 if the subtraction is not performed.
 */
static inline int RecordArrayDecrementNumUndecidedChildren(RecordArray *array,
                                                           Position position) {
    assert(position >= 0 && position < array->size);  // LCOV_EXCL_BR_LINE
    return RecordDecrementNumUndecidedChildren(&array->records[position]);
}

/**
 * @brief Sets the number of undecided children of `position` in
 * `array` to zero if the value field of `position` is set to `kUndecided`,
 * and returns the number of undecided children immediately preceding the
 * operation. Does nothing and returns 0 if the value field of `position` is not
 * `kUndecided`.
 *
 * @note The remoteness field of a `Record` is overloaded as the counter for the
 * number of undecided children of that position when its value is `kUndecided`.
 *
 * @param[in,out] array Target array.
 * @param[in] position Position.
 *
 * @returns Number of undecided children immediately preceding the operation, or
 * 0 if the operation is not performed.
 */
static inline int RecordArrayClearNumUndecidedChildren(RecordArray *array,
                                                       Position position) {
    assert(position >= 0 && position < array->size);  // LCOV_EXCL_BR_LINE
    return RecordClearNumUndecidedChildren(&array->records[position]);
}

/**
 * @brief Returns the value of `position` in the given `array`. Assumes
 * `position` is greater than or equal to 0 and smaller than the size of
 * `array`.
 *
 * @param[in] array Source array.
 * @param[in] position Position.
 *
 * @returns `Value` of `position`.
 */
static inline Value RecordArrayGetValue(const RecordArray *array,
                                        Position position) {
    return RecordGetValue(&array->records[position]);
}

/**
 * @brief Returns the remoteness of `position` in the given `array`. Assumes
 * `position` is greater than or equal to 0 and smaller than the size of
 * `array`.
 *
 * @param[in] array Source array.
 * @param[in] position Position.
 *
 * @returns Remoteness of `position`.
 */
static inline int RecordArrayGetRemoteness(const RecordArray *array,
                                           Position position) {
    return RecordGetRemoteness(&array->records[position]);
}

/**
 * @brief Returns the number of undecided children of `position` in
 * `array` if the value field of `position` is set to `kUndecided`. Returns
 * 0 otherwise.
 *
 * @note The remoteness field of a `Record` is overloaded as the counter for the
 * number of undecided children of that position when its value is `kUndecided`.
 *
 * @param[in] array Source array.
 * @param[in] position Position.
 *
 * @returns Number of undecided children of `position` if its value is
 * `kUndecided`, or 0 otherwise.
 */
static inline int RecordArrayGetNumUndecidedChildren(const RecordArray *array,
                                                     Position position) {
    return RecordGetNumUndecidedChildren(&array->records[position]);
}

/**
 * @brief Returns a read-only direct pointer to the raw array used internally
 * by the `RecordArray` to store its elements.
 *
 * @param[in] array Source array.
 *
 * @returns Read-only direct pointer to the raw array.
 */
static inline const void *RecordArrayGetReadOnlyData(const RecordArray *array) {
    return (const void *)array->records;
}

/**
 * @brief Returns a read-write direct pointer to the raw array used
 * internally by the `RecordArray` to store its elements.
 *
 * @param[in,out] array Source array.
 *
 * @returns Direct pointer to the raw array.
 */
static inline void *RecordArrayGetData(RecordArray *array) {
    return (void *)array->records;
}

/**
 * @brief Returns the size of `array` in number of `Record`s.
 *
 * @param[in] array Target array.
 *
 * @returns Size of `array` in number of `Record`s.
 */
static inline int64_t RecordArrayGetSize(const RecordArray *array) {
    return array->size;
}

/**
 * @brief Returns the size of `array` in bytes.
 *
 * @param[in] array Target array.
 *
 * @returns Size of `array` in bytes.
 */
static inline size_t RecordArrayGetRawSize(const RecordArray *array) {
    return array->size * sizeof(Record);
}

#endif  // GAMESMANONE_CORE_DB_ARRAYDB_RECORD_ARRAY_H_
