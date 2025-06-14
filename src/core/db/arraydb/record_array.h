/**
 * @file record_array.h
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Fixed-length \c Record array for the Array Database.
 * @version 2.0.0
 * @date 2025-06-10
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

#include <assert.h>  // assert
#include <stddef.h>  // NULL, size_t
#include <stdint.h>  // int64_t
#include <string.h>  // memset

#include "core/db/arraydb/record.h"
#include "core/gamesman_memory.h"

/** @brief Fixed-length \c Record array. */
typedef struct RecordArray {
    int64_t size;
    Record records[];
} RecordArray;

/**
 * @brief Creates a new RecordArray of \p size elements.
 *
 * @param size Size of the new array in number of \c Records .
 * @return Pointer to the new RecordArray on success,
 * @return \c NULL on malloc failure.
 */
static inline RecordArray *RecordArrayCreate(int64_t size) {
    RecordArray *ret =
        (RecordArray *)GamesmanMalloc(sizeof(int64_t) + size * sizeof(Record));
    if (ret == NULL) return NULL;

    ret->size = size;
    memset(ret->records, 0, size * sizeof(Record));

    return ret;
}

/**
 * @brief Deallocates the \p array .
 *
 * @param array Array to deallocate.
 */
static inline void RecordArrayDestroy(RecordArray *array) {
    GamesmanFree(array);
}

/**
 * @brief Sets the value of position \p position in \p array to \p val. Assumes
 * \p position is greater than or equal to 0 and smaller than the size of
 * \p array .
 *
 * @param array Target array.
 * @param position Position.
 * @param val New value for the \p position.
 */
static inline void RecordArraySetValue(RecordArray *array, Position position,
                                       Value val) {
    assert(position >= 0 && position < array->size);
    RecordSetValue(&array->records[position], val);
}

/**
 * @brief Sets the remoteness of position \p position in \p array to
 * \p remoteness. Assumes \p position is greater than or equal to 0 and smaller
 * than the size of \p array.
 *
 * @param array Target array.
 * @param position Position.
 * @param remoteness New remoteness for the \p position.
 */
static inline void RecordArraySetRemoteness(RecordArray *array,
                                            Position position, int remoteness) {
    assert(position >= 0 && position < array->size);
    RecordSetRemoteness(&array->records[position], remoteness);
}

/**
 * @brief Sets the value and remoteness of \p position in \p array to
 * \p val and \p remoteness , respectively. Assumes \p position is greater than
 * or equal to 0 and smaller than the size of \p array .
 *
 * @param array Target array.
 * @param position Position.
 * @param val New value for the \p position .
 * @param remoteness New remoteness for the \p position .
 */
static inline void RecordArraySetValueRemoteness(RecordArray *array,
                                                 Position position, Value val,
                                                 int remoteness) {
    assert(position >= 0 && position < array->size);
    RecordSetValueRemoteness(&array->records[position], val, remoteness);
}

/**
 * @brief Replaces the value and remoteness of position \p position in \p array
 * with the maximum of its original value-remoteness pair and the one provided
 * by \p val and \p remoteness . The order of value-remoteness pairs are
 * determined by the \p compare function.
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
static inline void RecordArrayMaximize(RecordArray *array, Position position,
                                       Value val, int remoteness,
                                       int (*compare)(Value v1, int r1,
                                                      Value v2, int r2)) {
    assert(position >= 0 && position < array->size);
    RecordMaximize(&array->records[position], val, remoteness, compare);
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
static inline Value RecordArrayGetValue(const RecordArray *array,
                                        Position position) {
    return RecordGetValue(&array->records[position]);
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
static inline int RecordArrayGetRemoteness(const RecordArray *array,
                                           Position position) {
    return RecordGetRemoteness(&array->records[position]);
}

/**
 * @brief Returns a read-only direct pointer to the memory array used internally
 * by the \c RecordArray to store its elements.
 *
 * @param array Source array.
 * @return Read-only direct pointer to the memory array.
 */
static inline const void *RecordArrayGetReadOnlyData(const RecordArray *array) {
    return (const void *)array->records;
}

/**
 * @brief Returns a read-write direct pointer to the memory array used
 * internally by the \c RecordArray to store its elements.
 *
 * @param array Source array.
 * @return Direct pointer to the memory array.
 */
static inline void *RecordArrayGetData(RecordArray *array) {
    return (void *)array->records;
}

/**
 * @brief Returns the size of \p array in number of \c Records.
 *
 * @param array Target array.
 * @return Size of \p array in number of \c Records.
 */
static inline int64_t RecordArrayGetSize(const RecordArray *array) {
    return array->size;
}

/**
 * @brief Returns the size of \p array in bytes.
 *
 * @param array Target array.
 * @return Size of \p array in bytes.
 */
static inline size_t RecordArrayGetRawSize(const RecordArray *array) {
    return array->size * sizeof(Record);
}

#endif  // GAMESMANONE_CORE_DB_ARRAYDB_RECORD_ARRAY_H_
