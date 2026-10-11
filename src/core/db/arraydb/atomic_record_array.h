/**
 * @file atomic_record_array.h
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Fixed-length `AtomicRecord` array for the Array Database.
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

#ifndef GAMESMANONE_CORE_DB_ARRAYDB_ATOMIC_RECORD_ARRAY_H_
#define GAMESMANONE_CORE_DB_ARRAYDB_ATOMIC_RECORD_ARRAY_H_

#include <stddef.h>
#include <stdint.h>

#include "core/db/arraydb/atomic_record.h"
#include "core/db/arraydb/record.h"
#include "core/gamesman_memory.h"

/** Fixed-length `AtomicRecord` array. */
typedef struct AtomicRecordArray {
    int64_t size;           /**< Number of elements in the array. */
    AtomicRecord records[]; /**< Array of `AtomicRecord`s. */
} AtomicRecordArray;

/**
 * @brief Creates a new `AtomicRecordArray` of `size` elements.
 *
 * @param[in] size Size of the new array in number of `AtomicRecord`s.
 *
 * @return Pointer to the new `AtomicRecordArray` on success.
 * @retval NULL On memory allocation failure.
 */
static inline AtomicRecordArray *AtomicRecordArrayCreate(int64_t size) {
    AtomicRecordArray *ret = (AtomicRecordArray *)GamesmanMalloc(
        sizeof(int64_t) + size * sizeof(AtomicRecord));
    if (ret == NULL) {
        return NULL;
    }

    ret->size = size;
    for (int64_t i = 0; i < size; ++i) {
        AtmoicRecordInit(&ret->records[i], kUndecided, 0);
    }

    return ret;
}

/**
 * @brief Deallocates the `array`.
 *
 * @param[in,out] array Array to deallocate.
 */
static inline void AtomicRecordArrayDestroy(AtomicRecordArray *array) {
    GamesmanFree(array);
}

/**
 * @brief Atomically sets the value of `position` in `array` to `val`. Assumes
 * `position` is greater than or equal to 0 and smaller than the size of
 * `array`.
 *
 * @param[in,out] array Target array.
 * @param[in] position Position to set the value for.
 * @param[in] val New `Value` for `position`.
 */
static inline void AtomicRecordArraySetValue(AtomicRecordArray *array,
                                             Position position, Value val) {
    assert(position >= 0 && position < array->size);
    AtomicRecordSetValue(&array->records[position], val);
}

/**
 * @brief Atomically sets the remoteness of `position` in `array` to
 * `remoteness`. Assumes `position` is greater than or equal to 0 and smaller
 * than the size of `array`.
 *
 * @param[in,out] array Target array.
 * @param[in] position Position to set the remoteness for.
 * @param[in] remoteness New remoteness for `position`.
 */
static inline void AtomicRecordArraySetRemoteness(AtomicRecordArray *array,
                                                  Position position,
                                                  int remoteness) {
    assert(position >= 0 && position < array->size);
    AtomicRecordSetRemoteness(&array->records[position], remoteness);
}

/**
 * @brief Atomically sets the value and remoteness of `position` in `array` to
 * `val` and `remoteness`, respectively. Assumes `position` is greater than or
 * equal to 0 and smaller than the size of `array`.
 *
 * @param[in,out] array Target array.
 * @param[in] position Position to set the value and remoteness for.
 * @param[in] val New `Value` for `position`.
 * @param[in] remoteness New remoteness for `position`.
 */
static inline void AtomicRecordArraySetValueRemoteness(AtomicRecordArray *array,
                                                       Position position,
                                                       Value val,
                                                       int remoteness) {
    assert(position >= 0 && position < array->size);
    AtomicRecordSetValueRemoteness(&array->records[position], val, remoteness);
}

/**
 * @brief Atomically replaces the value and remoteness of `position` in
 * `array` with the maximum of its original value-remoteness pair and the one
 * provided by `val` and `remoteness`. The order of value-remoteness pairs is
 * determined by the `compare` function.
 *
 * @param[in,out] array Target array.
 * @param[in] position Position to replace the value and remoteness for.
 * @param[in] val Candidate new `Value` for `position`.
 * @param[in] remoteness Candidate new remoteness for `position`.
 * @param[in] compare Pointer to a value-remoteness pair comparison function
 * that takes in two value-remoteness pairs (v1, r1) and (v2, r2) and returns a
 * negative integer if (v1, r1) < (v2, r2), a positive integer if (v1, r1) >
 * (v2, r2), or zero if they are equal.
 *
 * @retval true If the provided `val`-`remoteness` pair is greater than the
 * original value-remoteness pair and the old pair is replaced.
 * @retval false Otherwise.
 */
static inline bool AtomicRecordArrayMaximize(
    AtomicRecordArray *array, Position position, Value val, int remoteness,
    int (*compare)(Value v1, int r1, Value v2, int r2)) {
    //
    assert(position >= 0 && position < array->size);
    return AtomicRecordMaximize(&array->records[position], val, remoteness,
                                compare);
}

/**
 * @brief Atomically subtracts one from the number of undecided children of
 * `position` in `array` and returns the number of undecided children
 * immediately preceding the subtraction if the value field of `position` is
 * set to `kUndecided` and the number of undecided children is at least one.
 * Does nothing and returns 0 if the value field of `position` is not
 * `kUndecided` or the number of undecided children is zero.
 *
 * @note The remoteness field of a `Record` is overloaded as the counter for the
 * number of undecided children of that position when its value is `kUndecided`.
 *
 * @param[in,out] array Target array.
 * @param[in] position Position to decrement the number of undecided children
 * for.
 *
 * @return Number of undecided children immediately preceding the subtraction,
 * or 0 if the subtraction is not performed.
 */
static inline int AtomicRecordArrayDecrementNumUndecidedChildren(
    AtomicRecordArray *array, Position position) {
    //
    assert(position >= 0 && position < array->size);
    return AtomicRecordDecrementNumUndecidedChildren(&array->records[position]);
}

/**
 * @brief Atomically sets the number of undecided children of `position` in
 * `array` to zero if the value field of `position` is set to `kUndecided`, and
 * returns the number of undecided children immediately preceding the
 * operation. Does nothing and returns 0 if the value field of `position` is not
 * `kUndecided`.
 *
 * @note The remoteness field of a `Record` is overloaded as the counter for the
 * number of undecided children of that position when its value is `kUndecided`.
 *
 * @param[in,out] array Target array.
 * @param[in] position Position to set the number of undecided children for.
 *
 * @return Number of undecided children immediately preceding the operation, or
 * 0 if the operation is not performed.
 */
static inline int AtomicRecordArrayClearNumUndecidedChildren(
    AtomicRecordArray *array, Position position) {
    //
    assert(position >= 0 && position < array->size);
    return AtomicRecordClearNumUndecidedChildren(&array->records[position]);
}

/**
 * @brief Returns the value of `position` in the given `array`. Assumes
 * `position` is greater than or equal to 0 and smaller than the size of
 * `array`.
 *
 * @param[in] array Source array.
 * @param[in] position Position to get the value of.
 *
 * @return `Value` of `position`.
 */
static inline Value AtomicRecordArrayGetValue(const AtomicRecordArray *array,
                                              Position position) {
    return AtomicRecordGetValue(&array->records[position]);
}

/**
 * @brief Returns the remoteness of `position` in the given `array`. Assumes
 * `position` is greater than or equal to 0 and smaller than the size of
 * `array`.
 *
 * @param[in] array Source array.
 * @param[in] position Position to get the remoteness of.
 *
 * @return Remoteness of `position`.
 */
static inline int AtomicRecordArrayGetRemoteness(const AtomicRecordArray *array,
                                                 Position position) {
    return AtomicRecordGetRemoteness(&array->records[position]);
}

/**
 * @brief Returns the number of undecided children of `position` in `array`
 * if the value field of `position` is set to `kUndecided`. Returns 0
 * otherwise.
 *
 * @note The remoteness field of a `Record` is overloaded as the counter for the
 * number of undecided children of that position when its value is `kUndecided`.
 *
 * @param[in] array Source array.
 * @param[in] position Position to get the number of undecided children of.
 *
 * @return Number of undecided children of `position` if its value is
 * `kUndecided`, or 0 otherwise.
 */
static inline int AtomicRecordArrayGetNumUndecidedChildren(
    const AtomicRecordArray *array, Position position) {
    //
    assert(position >= 0 && position < array->size);
    return AtomicRecordGetNumUndecidedChildren(&array->records[position]);
}

/**
 * @brief Returns the serialized size of `array` in bytes.
 *
 * @param[in] array Target array.
 *
 * @return Serialized size of `array` in bytes.
 */
static inline size_t AtomicRecordArrayGetSerializedSize(
    const AtomicRecordArray *array) {
    //
    return array->size * sizeof(Record);
}

/**
 * @brief Serializes up to `buf_size` bytes of raw contents from `array` into
 * `buf`.
 *
 * @details The buffer `buf` is assumed to have a capacity of at least
 * `buf_size` bytes. For streaming serialization, the first call should set
 * `offset` to 0. Subsequent calls can then advance `offset` by the number of
 * bytes serialized so far to stream the remaining data.
 *
 * @param[in] array Array to serialize.
 * @param[in] offset Total amount of data previously serialized in bytes.
 * @param[out] buf Output buffer.
 * @param[in] buf_size Size of the output buffer in bytes.
 *
 * @return Size of serialized data written to the output buffer in bytes.
 * @retval 0 If no data remains to be serialized.
 */
static inline size_t AtomicRecordArraySerializeStreaming(
    const AtomicRecordArray *array, size_t offset, void *buf, size_t buf_size) {
    //
    assert(offset % sizeof(Record) == 0);
    size_t total_bytes = AtomicRecordArrayGetSerializedSize(array);
    if (offset >= total_bytes) {
        return 0;
    }

    size_t start_rec = offset / sizeof(Record);        // Starting index
    size_t total_recs = total_bytes / sizeof(Record);  // Total records in array
    size_t recs_remaining = total_recs - start_rec;    // Records remaining
    size_t max_recs_buf = buf_size / sizeof(Record);   // Buffer can hold
    size_t n_recs =                                    // Records to serialize
        recs_remaining < max_recs_buf ? recs_remaining : max_recs_buf;
    const AtomicRecord *src = array->records + start_rec;
    Record *dst = (Record *)buf;
    for (size_t i = 0; i < n_recs; ++i) {
        dst[i] = AtomicRecordLoad(src + i);
    }

    return n_recs * sizeof(Record);
}

#endif  // GAMESMANONE_CORE_DB_ARRAYDB_ATOMIC_RECORD_ARRAY_H_
