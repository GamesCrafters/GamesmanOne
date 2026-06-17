/**
 * @file bitset.h
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Fixed-size bit set.
 * @version 1.0.0
 * @date 2025-06-30
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

#ifndef GAMESMANONE_CORE_DATA_STRUCTURES_BITSET_H_
#define GAMESMANONE_CORE_DATA_STRUCTURES_BITSET_H_

#include <stdbool.h>  // bool
#include <stddef.h>   // size_t
#include <stdint.h>   // int64_t

/** @brief Opaque fixed-size bit set type. */
typedef struct Bitset Bitset;

/**
 * @brief Returns the amount of memory required in bytes to create a Bitset of
 * size \p num_bits bits.
 *
 * @param num_bits Number of bits in the new Bitset.
 * @return The amount of memory required in bytes.
 */
size_t BitsetMemRequired(int64_t num_bits);

/**
 * @brief Constructs a Bitset of \p num_bits bits. All bits are initially set to
 * 0.
 *
 * @param num_bits Number of bits in the new Bitset.
 * @return Pointer to a newly created Bitset object, or
 * @return \c NULL on failure due to invalid \p num_bits or memory allocation
 * failure.
 */
Bitset *BitsetCreate(int64_t num_bits);

/**
 * @brief Deallocates \p bs or does nothing if \p bs is \c NULL .
 */
void BitsetDestroy(Bitset *bs);

/**
 * @brief Sets the \p i -th bit of \p bs to 1 and returns the previous value
 * of the bit.
 *
 * @param bs Target bitset.
 * @param i Index of the bit to be set.
 * @return Previous value of the bit.
 */
bool BitsetSet(Bitset *bs, int64_t i);

/**
 * @brief Resets the \p i -th bit of \p bs to 0 and returns the previous value
 * of the bit.
 *
 * @param bs Target bitset.
 * @param i Index of the bit to be reset.
 * @return Previous value of the bit.
 */
bool BitsetReset(Bitset *bs, int64_t i);

/**
 * @brief Sets the \p i -th bit of \p bs to \p val and returns the previous
 * value of the bit.
 *
 * @param bs Target bitset.
 * @param i Index of the bit to be modified.
 * @param val Desired bit value.
 * @return Previous value of the bit.
 */
bool BitsetSetTo(Bitset *bs, int64_t i, bool val);

/**
 * @brief Returns the \p i -th bit of \p bs .
 *
 * @param bs Source bitset.
 * @param i Index of the bit to be tested.
 * @return \c true if the \p i -th bit of \p bs is set,
 * @return \c false otherwise.
 */
bool BitsetTest(const Bitset *bs, int64_t i);

/**
 * @brief Returns the number of bits set to 1 in \p bs .
 *
 * @param stream Source bit stream.
 * @return Number of bits set to 1.
 */
int64_t BitsetCount(const Bitset *bs);

/**
 * @brief Returns the serialized size of \p bs in bytes.
 *
 * @param bs Source Bitset.
 * @return Serialized size of \p bs in bytes.
 */
size_t BitSetGetSerializedSize(const Bitset *bs);

/**
 * @brief Serializes at most \p buf_size bytes of raw contents of \p bs into
 * \p buf , which is assumed to be of size at least \p buf_size bytes, and
 * returns the number of bytes serialized. The first call to this function
 * should have \p offset set to 0. Then, the function may be called multiple
 * times, each time continuing from the given \p offset , which is set to the
 * total amount of serialized data, for streaming. The function returns 0 when
 * no data is left for serialization.
 *
 * @param bs Source bitset.
 * @param offset Total amount of serialized data before this call.
 * @param buf Output buffer.
 * @param buf_size Size of the output buffer.
 * @return Size of serialized data available in the output buffer in bytes.
 */
size_t BitsetSerializeStreaming(const Bitset *bs, size_t offset, void *buf,
                                size_t buf_size);

/**
 * @brief Deserializes \p in_size bytes of contents from \p in into \p bs ,
 * assuming \p in contains serialized data obtained from
 * BitsetSerializeStreaming and \p offset bytes have already been deserialized
 * before this call. The first call to this function should have \p bs set to
 * \c NULL and \p offset set to 0. Then, the function may be called multiple
 * times with \p bs set to the pointer returned by the first call to continue
 * streaming. If \p bs is set to \c NULL , the function returns a newly
 * constructed Bitset. Otherwise, the function returns \p bs .
 *
 * @note The minimum allowed input size is 8 bytes.
 *
 * @param bs Destination bitset. If set to \c NULL , a new Bitset will be
 * constructed and returned.
 * @param offset Total amount of data deserialized before this call.
 * @param in Input buffer containing serialized data.
 * @param in_size Size of the input buffer.
 * @return Pointer to the destination Bitset.
 */
Bitset *BitsetDeserializeStreaming(Bitset *bs, size_t offset, const void *in,
                                   size_t in_size);

/**
 * @brief Returns a read-write pointer to the raw data of \p bs . The size of
 * the raw data can be obtained using \c BitSetGetSerializedSize. The intended
 * usage is to only read or write whole \c Bitset objects as the API does not
 * expose any methods to manipulate internal states of the \c Bitset object.
 *
 * @param bs Bitset to get the raw pointer of.
 * @return A read-write pointer to the raw data of \p bs , or \c NULL if \p bs
 * is \c NULL .
 */
void *BitsetGetRawData(Bitset *bs);

#endif  // GAMESMANONE_CORE_DATA_STRUCTURES_BITSET_H_
