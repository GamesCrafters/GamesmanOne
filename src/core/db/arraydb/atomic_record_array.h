/**
 * @file atomic_record_array.h
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Fixed-length \c AtomicRecord array for the Array Database.
 * @version 1.0.0
 * @date 2025-06-09
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

#include <stddef.h>  // size_t
#include <stdint.h>  // int64_t

#include "core/db/arraydb/atomic_record.h"
#include "core/db/arraydb/record.h"
#include "core/types/gamesman_types.h"

/** @brief Fixed-length \c Record array. */
typedef struct AtomicRecordArray {
    int64_t size;
    AtomicRecord records[];
} AtomicRecordArray;

/**
 * @brief Creates a new AtomicRecordArray of \p size elements.
 *
 * @param size Size of the new array in number of \c Records .
 * @return Pointer to the new AtomicRecordArray on success,
 * @return \c NULL on malloc failure.
 */
static inline AtomicRecordArray *AtomicRecordArrayCreate(int64_t size) {
    AtomicRecordArray *ret = (AtomicRecordArray *)GamesmanMalloc(
        sizeof(int64_t) + size * sizeof(AtomicRecord));
    if (ret == NULL) return NULL;

    ret->size = size;
    for (int64_t i = 0; i < size; ++i) {
        AtmoicRecordInit(&ret->records[i], kUndecided, 0);
    }

    return ret;
}

/**
 * @brief Deallocates the \p array .
 *
 * @param array Array to deallocate.
 */
static inline void AtomicRecordArrayDestroy(AtomicRecordArray *array) {
    GamesmanFree(array);
}

/**
 * @brief Atomically sets the value of position \p position in \p array to
 * \p val . Assumes
 * \p position is greater than or equal to 0 and smaller than the size of
 * \p array .
 *
 * @param array Target array.
 * @param position Position.
 * @param val New value for the \p position .
 */
static inline void AtomicRecordArraySetValue(AtomicRecordArray *array,
                                             Position position, Value val) {
    assert(position >= 0 && position < array->size);
    AtomicRecordSetValue(&array->records[position], val);
}

/**
 * @brief Atomically sets the remoteness of position \p position in \p array to
 * \p remoteness . Assumes \p position is greater than or equal to 0 and smaller
 * than the size of \p array .
 *
 * @param array Target array.
 * @param position Position.
 * @param remoteness New remoteness for the \p position.
 */
static inline void AtomicRecordArraySetRemoteness(AtomicRecordArray *array,
                                                  Position position,
                                                  int remoteness) {
    assert(position >= 0 && position < array->size);
    AtomicRecordSetRemoteness(&array->records[position], remoteness);
}

/**
 * @brief Atomically sets the value and remoteness of \p position in \p array to
 * \p val and \p remoteness , respectively. Assumes \p position is greater than
 * or equal to 0 and smaller than the size of \p array .
 *
 * @param array Target array.
 * @param position Position.
 * @param val New value for the \p position .
 * @param remoteness New remoteness for the \p position .
 */
static inline void AtomicRecordArraySetValueRemoteness(AtomicRecordArray *array,
                                                       Position position,
                                                       Value val,
                                                       int remoteness) {
    assert(position >= 0 && position < array->size);
    AtomicRecordSetValueRemoteness(&array->records[position], val, remoteness);
}

/**
 * @brief Atomically replaces the value and remoteness of position \p position
 * in \p array with the maximum of its original value-remoteness pair and the
 * one provided by \p val and \p remoteness . The order of value-remoteness
 * pairs are determined by the \p compare function.
 *
 * @param array Target array.
 * @param position Position.
 * @param val Candidate new value for the \p position .
 * @param remoteness Candidate new remoteness for the \p position .
 * @param compare Pointer to a value-remoteness pair comparison function that
 * takes in two value-remoteness pairs (v1, r1) and (v2, r2) and returns a
 * negative integer if (v1, r1) < (v2, r2), a positive integer if (v1, r1) >
 * (v2, r2), or zero if they are equal.
 */
static inline void AtomicRecordArrayMaximize(
    AtomicRecordArray *array, Position position, Value val, int remoteness,
    int (*compare)(Value v1, int r1, Value v2, int r2)) {
    assert(position >= 0 && position < array->size);
    AtomicRecordMaximize(&array->records[position], val, remoteness, compare);
}

/**
 * @brief Returns the value of \p position in the given \p array, assumes
 * \p position is greater than or equal to 0 and smaller than the size of
 * \p array.
 *
 * @param array Source array.
 * @param position Position.
 * @return Value of \p position.
 */
static inline Value AtomicRecordArrayGetValue(const AtomicRecordArray *array,
                                              Position position) {
    return AtomicRecordGetValue(&array->records[position]);
}

/**
 * @brief Returns the remoteness of \p position in the given \p array, assumes
 * \p position is greater than or equal to 0 and smaller than the size of
 * \p array.
 *
 * @param array Source array.
 * @param position Position.
 * @return Remoteness of \p position.
 */
static inline int AtomicRecordArrayGetRemoteness(const AtomicRecordArray *array,
                                                 Position position) {
    return AtomicRecordGetRemoteness(&array->records[position]);
}

/**
 * @brief Returns the size of \p array after it is serialized in bytes.
 *
 * @param array Target array.
 * @return Size of \p array after it is serialized in bytes.
 */
static inline size_t AtomicRecordArrayGetSerializedSize(
    const AtomicRecordArray *array) {
    return array->size * sizeof(Record);
}

/**
 * @brief Serializes at most \p buf_size bytes of raw contents of \p array into
 * \p buf , which is assumed to be of size at least \p buf_size bytes. The first
 * call to this function should have \p offset set to 0. Then, the function may
 * be called multiple times, each time continuing from the given \p offset ,
 * which is set to the total amount of serialized data, for streaming. The
 * function returns 0 when no data is left for serialization.
 *
 * @param array Array to serialize
 * @param offset Total amount of serialized data.
 * @param buf Output buffer.
 * @param buf_size Size of the output buffer.
 * @return Size of serialized data available in the output buffer in bytes.
 */
static inline size_t AtomicRecordArraySerializeStreaming(
    const AtomicRecordArray *array, size_t offset, void *buf, size_t buf_size) {
    assert(offset % sizeof(Record) == 0);
    size_t total_bytes = AtomicRecordArrayGetSerializedSize(array);
    if (offset >= total_bytes) return 0;

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
