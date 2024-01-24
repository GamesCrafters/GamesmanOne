/**
 * @file bparray.h
 * @author Dan Garcia: designed the "lookup table" compression algorithm
 * @author Max Fierro: improved the algorithm for BpArray compression
 * @author Sameer Nayyar: improved the algorithm for BpArray compression
 * @author Robert Shi (robertyishi@berkeley.edu): improved and implemented
 *         BpArray.
 *         GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Bit-Perfect array of unsigned integers.
 * @version 1.0.0
 * @date 2023-09-26
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

#ifndef GAMESMANONE_CORE_DB_BPDB_BPARRAY_H_
#define GAMESMANONE_CORE_DB_BPDB_BPARRAY_H_

#include <stdint.h>  // int8_t, uint8_t, int32_t, int64_t, uint64_t

#include "core/db/bpdb/bpdict.h"

// Include and use OpenMP if the _OPENMP flag is set.
#ifdef _OPENMP
#include <omp.h>
#endif

/** @brief Bit-Perfect Array metadata. */
typedef struct BpArrayMeta {
    int64_t stream_length; /**< Length of the bit stream in bytes. */
    int64_t num_entries;   /**< Number of entries stored in the array. */
    int8_t bits_per_entry; /**< Number of bits used to store each entry. */
} BpArrayMeta;

/**
 * @brief Fixed-Size Bit-Perfect Array of unsigned integers.
 *
 * @note The current implementation does not support integers greater than
 * the maximum value of int32_t, which is INT32_MAX. The compression algorithm
 * must be redesigned not to use a int32_t array as dictionary before making
 * changes to this restriction.
 *
 * @details On the outside, a BpArray behaves just like an ordinary fixed-size
 * array of uint64_t entries (except the restriction on maximum entry value).
 * Internally, it keeps track of the unique entries inserted to use just enough
 * bits to store each entry in the bit stream.
 */
typedef struct BpArray {
    /** Bit stream of compressed entries. */
    uint8_t *stream;

    /** Dictionary for entry compression/decompression. */
    BpDict dict;

    /** Array metadata. */
    BpArrayMeta meta;
} BpArray;

/**
 * @brief Initializes the given ARRAY to the given SIZE and initializes all
 * entries to zeros (0).
 *
 * @note Assumes ARRAY is uninitialized. May result in memory leak and other
 * undefined behaviors otherwise.
 *
 * @param array Array to initialize.
 * @param size Size of the array in number of entries.
 * @return 0 on success or
 * @return non-zero error code otherwise.
 */
int BpArrayInit(BpArray *array, int64_t size);

/** @brief Destroys the given ARRAY. */
void BpArrayDestroy(BpArray *array);

/** @brief Returns ARRAY[I]. */
uint64_t BpArrayGet(BpArray *array, int64_t i);

/**
 * @brief Sets ARRAY[I] to ENTRY.
 *
 * @details If ENTRY does not exist in the ARRAY, a new encoding will be
 * assigned to ENTRY for bit-perfect compression. May trigger a resize of the
 * bit stream if more bits are needed to store all the unique entry encodings
 * after the insertion of ENTRY. Fails and returns an error if there is not
 * enough memory to perform the resize.
 *
 * @return 0 on success or
 * @return non-zero error code otherwise.
 */
int BpArraySet(BpArray *array, int64_t i, uint64_t entry);

/** @brief Returns the number of unique values stored in the ARRAY. */
int32_t BpArrayGetNumUniqueValues(const BpArray *array);

/**
 * @brief Returns the decompression dictionary for the given ARRAY.
 * @details The decompression dictionary is the reverse map of the compression
 * dictionary. Both dictionaries are managed by a BpDict object as entries
 * are inserted. The compression dictionary maps entries to bit-perfect
 * encodings, and the decompression dictionary maps encodings to entries.
 */
const int32_t *BpArrayGetDecompDict(const BpArray *array);

#endif  // GAMESMANONE_CORE_DB_BPDB_BPARRAY_H_
