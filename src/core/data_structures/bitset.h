/**
 * @file bitset.h
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Fixed-size bit set.
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

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/**
 * @brief Opaque fixed-size bit set type.
 */
typedef struct Bitset Bitset;

/**
 * @brief Calculates the memory required to allocate a Bitset.
 *
 * @param[in] num_bits The total capacity of the Bitset in bits.
 *
 * @pre @p num_bits must be >= 0. Passing a negative size is undefined behavior.
 *
 * @return The amount of memory required in bytes.
 */
size_t BitsetMemRequired(int64_t num_bits);

/**
 * @brief Constructs a Bitset initialized to all zeros.
 *
 * @param[in] num_bits The capacity of the new Bitset in bits.
 *
 * @pre `num_bits` must be >= 0.
 *
 * @return A pointer to a newly created Bitset, or `NULL` on memory allocation
 * failure. Note that a valid pointer will be returned even if `num_bits` is 0.
 * A `NULL` return always indicates an error.
 */
Bitset *BitsetCreate(int64_t num_bits);

/**
 * @brief Deallocates the target bitset.
 *
 * @param[in] bs The bitset to destroy. If `bs` is `NULL`, the function does
 * nothing.
 */
void BitsetDestroy(Bitset *bs);

/**
 * @brief Sets a specific bit to 1.
 *
 * @param[in,out] bs Target bitset.
 * @param[in] i Index of the bit to set.
 *
 * @pre `bs` must not be `NULL`.
 * @pre `i` must be valid (0 <= `i` < `bs->num_bits`). Bounds checking is
 * omitted for performance; out-of-bounds indices result in undefined behavior.
 *
 * @return The previous value of the bit.
 */
bool BitsetSet(Bitset *bs, int64_t i);

/**
 * @brief Resets a specific bit to 0.
 *
 * @param[in,out] bs Target bitset.
 * @param[in] i Index of the bit to reset.
 *
 * @pre `bs` must not be `NULL`.
 * @pre `i` must be valid (0 <= `i` < `bs->num_bits`).
 *
 * @return The previous value of the bit.
 */
bool BitsetReset(Bitset *bs, int64_t i);

/**
 * @brief Sets a specific bit to a given boolean value.
 *
 * @param[in,out] bs Target bitset.
 * @param[in] i Index of the bit to modify.
 * @param[in] val The desired bit value.
 *
 * @pre `bs` must not be `NULL`.
 * @pre `i` must be valid (0 <= `i` < `bs->num_bits`).
 *
 * @return The previous value of the bit.
 */
bool BitsetSetTo(Bitset *bs, int64_t i, bool val);

/**
 * @brief Tests the value of a specific bit.
 *
 * @param[in] bs Source bitset.
 * @param[in] i Index of the bit to test.
 *
 * @pre `bs` must not be `NULL`.
 * @pre `i` must be valid (0 <= `i` < `bs->num_bits`).
 *
 * @retval `true` if the bit is set to 1
 * @retval `false` otherwise.
 */
bool BitsetTest(const Bitset *bs, int64_t i);

/**
 * @brief Returns the total number of bits currently set to 1.
 *
 * @param[in] bs Source bitset.
 *
 * @pre `bs` must not be `NULL`.
 *
 * @return The number of high bits.
 */
int64_t BitsetCount(const Bitset *bs);

/**
 * @brief Returns the serialized size of the bitset in bytes.
 *
 * @param[in] bs Source bitset.
 *
 * @return The exact number of bytes required to serialize `bs`, or 0 if `bs`
 * is `NULL`.
 */
size_t BitSetGetSerializedSize(const Bitset *bs);

/**
 * @brief Exposes the raw internal memory of the Bitset.
 *
 * This function breaks opacity to allow direct, whole-object memory operations.
 * The total size of the memory block matches the return value of
 * `BitSetGetSerializedSize`.
 *
 * @param[in,out] bs The bitset to expose.
 *
 * @return A pointer to the raw struct data, or `NULL` if `bs` is `NULL`.
 */
void *BitsetGetRawData(Bitset *bs);

#endif  // GAMESMANONE_CORE_DATA_STRUCTURES_BITSET_H_
