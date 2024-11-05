/**
 * @file bitstream.h
 * @author Robert Shi (robertyishi@berkeley.edu)
 *         GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Fixed-size bit stream.
 * @version 1.1.0
 * @date 2024-09-05
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

#ifndef GAMESMANONE_CORE_DATA_STRUCTURES_BITSTREAM_H_
#define GAMESMANONE_CORE_DATA_STRUCTURES_BITSTREAM_H_

#include <stdbool.h>  // bool
#include <stdint.h>   // uint8_t, int64_t

/** @brief Fixed-size bit stream. */
typedef struct BitStream {
    uint8_t *stream;   /**< Bit stream as raw bytes. */
    int64_t size;      /**< Size of the bit stream in number of bits. */
    int64_t num_bytes; /**< Size of stream in bytes. */
    int64_t count;     /**< Counter for the number of bits set to true. */
} BitStream;

/**
 * @brief Initializes the bit stream STREAM to have SIZE bits that are all set
 * to 0.
 *
 * @param stream Bit stream object to initialize.
 * @param size Size of the bit stream in number of bits.
 * @return 0 on success, non-zero error code otherwise.
 */
int BitStreamInit(BitStream *stream, int64_t size);

/**
 * @brief Destroys STREAM, freeing all dynamically allocated space.
 */
void BitStreamDestroy(BitStream *stream);

/**
 * @brief Sets the I-th bit of STREAM.
 * @return 0 if I is valid for STREAM, -1 otherwise.
 */
int BitStreamSet(BitStream *stream, int64_t i);

/**
 * @brief Clears the I-th bit of STREAM.
 * @return 0 if I is valid for STREAM, -1 otherwise.
 */
int BitStreamClear(BitStream *stream, int64_t i);

/**
 * @brief Returns the I-th bit of STREAM.
 * @return \c true if the I-th bit of STREAM is set,
 * @return \c false otherwise.
 */
bool BitStreamGet(const BitStream *stream, int64_t i);

/**
 * @brief Returns the number of bits set to \c true in \p stream.
 *
 * @param stream Target bit stream.
 * @return Number of bits set to \c true.
 */
int64_t BitStreamCount(const BitStream *stream);

#endif  // GAMESMANONE_CORE_DATA_STRUCTURES_BITSTREAM_H_
