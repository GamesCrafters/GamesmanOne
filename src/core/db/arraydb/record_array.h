/**
 * @file record_array.h
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Fixed-length \c Record array for the Array Database.
 * @version 1.0.1
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

#ifndef GAMESMANONE_CORE_DB_BPDB_RECORD_ARRAY_H_
#define GAMESMANONE_CORE_DB_BPDB_RECORD_ARRAY_H_

#include <stdint.h>  // int64_t

#include "core/db/arraydb/record.h"

/** @brief Fixed-length \c Record array. */
typedef struct RecordArray {
    Record *records;
    int64_t size;
} RecordArray;

/**
 * @brief Initializes \p array to \p size elements.
 * @note Assumes \p array is uninitialized. Results in undefined behavior if
 * \p array has been initialized with a prior call to the same function.
 *
 * @param array Array to be initialized.
 * @param size Size of the new array in number of \c Records.
 * @return \c kNoError on success, or
 * @return \c kMallocFailureError if malloc fails to allocate enough space for
 * \p size records.
 */
int RecordArrayInit(RecordArray *array, int64_t size);

/**
 * @brief Deallocates the \p array.
 *
 * @param array Array to deallocate.
 */
void RecordArrayDestroy(RecordArray *array);

/**
 * @brief Sets the value of position \p position in \p array to \p val. Assumes
 * \p position is greater than or equal to 0 and smaller than the size of \p
 * array.
 *
 * @param array Target array.
 * @param position Position.
 * @param val New value for the \p position.
 */
void RecordArraySetValue(RecordArray *array, Position position, Value val);

/**
 * @brief Sets the remoteness of position \p position in \p array to
 * \p remoteness. Assumes \p position is greater than or equal to 0 and smaller
 * than the size of \p array.
 *
 * @param array Target array.
 * @param position Position.
 * @param remoteness New remoteness for the \p position.
 */
void RecordArraySetRemoteness(RecordArray *array, Position position,
                              int remoteness);

/**
 * @brief Returns the \c Value of \p position in the given \p array, assumes
 * \p position is greater than or equal to 0 and smaller than the size of
 * \p array.
 *
 * @param array Source array.
 * @param position Position.
 * @return \c Value of \p position.
 */
Value RecordArrayGetValue(const RecordArray *array, Position position);

/**
 * @brief Returns the remoteness of \p position in the given \p array, assumes
 * \p position is greater than or equal to 0 and smaller than the size of
 * \p array.
 *
 * @param array Source array.
 * @param position Position.
 * @return Remoteness of \p position.
 */
int RecordArrayGetRemoteness(const RecordArray *array, Position position);

/**
 * @brief Returns a read-only direct pointer to the memory array used internally
 * by the \c RecordArray to store its elements.
 *
 * @param array Source array.
 * @return Read-only direct pointer to the memory array.
 */
const void *RecordArrayGetReadOnlyData(const RecordArray *array);

/**
 * @brief Returns a R/W direct pointer to the memory array used internally
 * by the \c RecordArray to store its elements.
 *
 * @param array Source array.
 * @return Direct pointer to the memory array.
 */
void *RecordArrayGetData(RecordArray *array);

/**
 * @brief Returns the size of \p array in number of \c Records.
 *
 * @param array Target array.
 * @return Size of \p array in number of \c Records.
 */
int64_t RecordArrayGetSize(const RecordArray *array);

/**
 * @brief Returns the size of \p array in bytes.
 *
 * @param array Target array.
 * @return Size of \p array in bytes.
 */
int64_t RecordArrayGetRawSize(const RecordArray *array);

#endif  // GAMESMANONE_CORE_DB_BPDB_RECORD_ARRAY_H_
