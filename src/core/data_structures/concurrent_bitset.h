/**
 * This file is a C language translation and modification of:
 *   folly/folly/ConcurrentBitSet.h from Meta Platforms, Inc. (Facebook)
 * https://github.com/facebook/folly/blob/main/folly/ConcurrentBitSet.h
 *
 * Original file Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Modifications:
 *   - Translated from C++ to C
 *   - Reformatted and adapted to C idioms
 *   - Integrated into a GPL-3.0 project
 *
 * This file is dual-licensed:
 *   - Portions are licensed under the Apache License 2.0.
 *   - Modifications are licensed under the GNU General Public License v3.0.
 *
 * See the COPYING and THIRD_PARTY_LICENSES files for details.
 *
 * @file concurrent_bitset.h
 * @author Meta Platforms, Inc. and affiliates (original C++ version)
 * @author Robert Shi (robertyishi@berkeley.edu): adapted to C.
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief A Concurrent Bitset suitable in a multi-writer multi-reader context.
 * @version 1.0.0
 * @date 2025-03-26
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

#ifndef GAMESMANONE_CORE_DATA_STRUCTURES_CONCURRENT_BITSET_H_
#define GAMESMANONE_CORE_DATA_STRUCTURES_CONCURRENT_BITSET_H_

#include <stdatomic.h>  // memory_order
#include <stdbool.h>    // bool
#include <stddef.h>     // size_t
#include <stdint.h>     // int64_t

#include "core/gamesman_memory.h"

typedef struct ConcurrentBitset ConcurrentBitset;

/**
 * @brief Returns the amount of memory required in bytes to create a
 * ConcurrentBitset of size \p num_bits bits.
 *
 * @param num_bits Number of bits in the new ConcurrentBitset.
 * @return The amount of memory required.
 */
size_t ConcurrentBitsetMemRequired(int64_t num_bits);

/**
 * @brief Constructs a ConcurrentBitset of \p num_bits bits. All bits are
 * initially set to 0.
 *
 * @param num_bits Number of bits in the new ConcurrentBitset.
 * @return Pointer to a newly created ConcurrentBitset object, or
 * @return \c NULL on failure due to invalid \p num_bits or memory allocation
 * failure.
 */
ConcurrentBitset *ConcurrentBitsetCreate(int64_t num_bits);

/**
 * @brief Constructs a ConcurrentBitset of \p num_bits bits using \p allocator
 * as the underlying memory allocator. If \p allocator is \c NULL, the function
 * call is equivalent to ConcurrentBitsetCreate(num_bits). Note that this
 * function does not transfer the ownership of \p allocator to the new array
 * object. The caller is responsible for releasing its own copy of the
 * allocator.
 *
 * @param num_bits Number of bits in the new ConcurrentBitset.
 * @param allocator Memory allocator to use.
 * @return Pointer to a newly created ConcurrentBitset object, or
 * @return \c NULL on failure due to invalid \p num_bits or memory allocation
 * failure.
 */
ConcurrentBitset *ConcurrentBitsetCreateAllocator(int64_t num_bits,
                                                  GamesmanAllocator *allocator);

/**
 * @brief Constructs a copy of the given ConcurrentBitset. If the \p other
 * ConcurrentBitset uses a custom memory allocator, the copy will also create
 * a reference of the same memory allocator and use it.
 * @note This function does not provide thread-safety. The copy operation is
 * not atomic.
 *
 * @param other ConcurrentBitset object to copy.
 * @return Pointer to the copy of the given ConcurrentBitset object, or
 * @return \c NULL on memory allocation failure.
 */
ConcurrentBitset *ConcurrentBitsetCreateCopy(const ConcurrentBitset *other);

/**
 * @brief Destroys the given ConcurrentBitset object.
 *
 * @param s Pointer to the ConcurrentBitset object to be destroyed.
 */
void ConcurrentBitsetDestroy(ConcurrentBitset *s);

/**
 * @brief Returns the number of bits in the given ConcurrentBitset.
 *
 * @param s Pointer to a ConcurrentBitset object.
 * @return Number of bits in the ConcurrentBitset.
 */
int64_t ConcurrentBitsetGetNumBits(const ConcurrentBitset *s);

/**
 * @brief Sets the bit at index \p bit_index to 1 and returns the previous value
 * of the bit. Undefined if \p s is \c NULL or if \p bit_index is out of bounds.
 *
 * @param s Pointer to the target ConcurrentBitset object to modify.
 * @param bit_index Index of the bit to be set.
 * @param memory_order Memory order to use.
 * @return Previous value of the bit.
 */
bool ConcurrentBitsetSet(ConcurrentBitset *s, int64_t bit_index,
                         memory_order order);

/**
 * @brief Resets the bit at index \p bit_index to 0 and returns the previous
 * value of the bit. Undefined if \p s is \c NULL or if \p bit_index is out of
 * bounds.
 *
 * @param s Pointer to the target ConcurrentBitset object to modify.
 * @param bit_index Index of the bit to be reset.
 * @param memory_order Memory order to use.
 * @return Previous value of the bit.
 */
bool ConcurrentBitsetReset(ConcurrentBitset *s, int64_t bit_index,
                           memory_order order);

/**
 * @brief Resets all bits in the given ConcurrentBitset \p s. Does nothing if
 * \p s is \c NULL.
 * @note This function does not provide thread-safety. If \p s is modified
 * by another thread while this function is in progress, the result is
 * undefined.
 *
 * @param s Pointer to the target ConcurrentBitset object to modify.
 */
void ConcurrentBitsetResetAll(ConcurrentBitset *s);

/**
 * @brief Returns the bit at index \p bit_index as a boolean value.
 *
 * @param s Pointer to the source ConcurrentBitset object.
 * @param bit_index Index of the bit to be tested.
 * @param memory_order Memory order to use.
 * @return \c true if the bit at index \p bit_index is 1,
 * @return \c false otherwise.
 */
bool ConcurrentBitsetTest(ConcurrentBitset *s, int64_t bit_index,
                          memory_order order);

/**
 * @brief Returns the amount of memory required in bytes to store the serialized
 * ConcurrentBitset object.
 *
 * @param s ConcurrentBitset object to be serialized.
 * @return Amount of memory required in bytes to store the serialized
 * ConcurrentBitset object.
 */
size_t ConcurrentBitsetGetSerializedSize(const ConcurrentBitset *s);

/**
 * @brief Serializes the ConcurrentBitset object into \p buf, which is assumed
 * to have enough space to hold the serialized result.
 *
 * @note This function is not thread-safe.
 * @note Use ConcurrentBitsetGetSerializedSize to check how long \p buf should
 * be.
 *
 * @param s ConcurrentBitset object to be serialized.
 * @param buf Buffer for the serialized output.
 */
void ConcurrentBitsetSerialize(const ConcurrentBitset *s, void *buf);

/**
 * @brief Deserializes the given buffer \p buf into the target ConcurrentBitset
 * object by overwriting its content. \p buf is assumed to hold serialized
 * contents previously generated by ConcurrentBitsetSerialize on a
 * ConcurrentBitset object of the same length as \p s and be of size
 * ConcurrentBitsetGetSerializedSize(s) bytes.
 * @note This function is not thread-safe.
 * @note The size of the buffer is given in number of bytes instead of bits.
 *
 * @param s Pointer to the destination ConcurrentBitset object.
 * @param buf Buffer that holds the serialized bitset content.
 */
void ConcurrentBitsetDeserialize(ConcurrentBitset *s, const void *buf);

#endif  // GAMESMANONE_CORE_DATA_STRUCTURES_CONCURRENT_BITSET_H_
