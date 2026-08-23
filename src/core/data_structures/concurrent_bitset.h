/**
 * This file is a C language translation and modification of:
 *  folly/folly/ConcurrentBitSet.h from Meta Platforms, Inc. (Facebook)
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

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "core/gamesman_memory.h"

/* Handle memory_order compatibility between C11 and C++11 */
#ifdef __cplusplus
#include <atomic>
using memory_order_compat = std::memory_order;
#else
#include <stdatomic.h>
typedef memory_order memory_order_compat;
#endif

/* Forward declaration of the struct if it's opaque */
typedef struct ConcurrentBitset ConcurrentBitset;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Opaque type representing a concurrent bitset.
 */
typedef struct ConcurrentBitset ConcurrentBitset;

/**
 * @brief Returns the amount of memory required in bytes to create a
 * `ConcurrentBitset` of size `num_bits` bits.
 *
 * @param[in] num_bits Number of bits in the new `ConcurrentBitset`. If
 * negative, `SIZE_MAX` will be returned to indicate an error.
 *
 * @return The amount of memory required in bytes.
 */
size_t ConcurrentBitsetMemRequired(int64_t num_bits);

/**
 * @brief Constructs a `ConcurrentBitset` of `num_bits` bits with all bits set
 * to 0.
 *
 * @details This function is multithreaded using all OpenMP threads.
 *
 * @param[in] num_bits Number of bits in the new `ConcurrentBitset`. If set to
 * 0, a `ConcurrentBitset` of 0 bits will be created and returned, and it must
 * still be deallocated using the `ConcurrentBitsetDestroy()` function to
 * prevent memory leak. If negative, the function will return `NULL`.
 *
 * @return Pointer to a newly created `ConcurrentBitset` object. `NULL` on
 * memory allocation failure or if `num_bits` is negative.
 */
ConcurrentBitset *ConcurrentBitsetCreateMt(int64_t num_bits);

/**
 * @brief Constructs a `ConcurrentBitset` of `num_bits` bits using `allocator`
 * as the underlying memory allocator.
 *
 * @details If `allocator` is `NULL`, the function call is equivalent to
 * `ConcurrentBitsetCreateMt(num_bits)`. Note that this function does not
 * transfer the ownership of `allocator` to the new array object. The caller is
 * responsible for releasing its own copy of the allocator. This function is
 * multithreaded using all OpenMP threads. If `num_bits` is negative, the
 * function will return `NULL`.
 *
 * @param[in] num_bits Number of bits in the new `ConcurrentBitset`.
 * @param[in,out] allocator Memory allocator to use.
 *
 * @return Pointer to a newly created `ConcurrentBitset` object. `NULL` on
 * memory allocation failure.
 */
ConcurrentBitset *ConcurrentBitsetCreateAllocatorMt(
    int64_t num_bits, GamesmanAllocator *allocator);

/**
 * @brief Constructs a copy of the given `ConcurrentBitset`.
 *
 * @details If the `other` `ConcurrentBitset` uses a custom memory allocator,
 * the copy will create a reference of the same memory allocator and use it.
 * `NULL` will be returned if `other` is `NULL`. This function is multithreaded
 * using all OpenMP threads.
 *
 * @note This function is not MT-safe unless `other` is `NULL`. The copy
 * operation is not atomic.
 *
 * @param[in] other `ConcurrentBitset` object to copy.
 *
 * @return Pointer to the copy of the given `ConcurrentBitset` object. `NULL` if
 * `other` is `NULL` or on memory allocation failure.
 */
ConcurrentBitset *ConcurrentBitsetCreateCopyMt(const ConcurrentBitset *other);

/**
 * @brief Destroys the given `ConcurrentBitset` object. Does nothing if `s` is
 * `NULL`.
 *
 * @param[in,out] s Pointer to the `ConcurrentBitset` object to be destroyed.
 */
void ConcurrentBitsetDestroy(ConcurrentBitset *s);

/**
 * @brief Returns the number of bits in the given `ConcurrentBitset`.
 *
 * @param[in] s Non-`NULL` pointer to a `ConcurrentBitset` object.
 *
 * @return Number of bits in the `ConcurrentBitset`.
 */
int64_t ConcurrentBitsetGetNumBits(const ConcurrentBitset *s);

/**
 * @brief Sets the bit at index `bit_index` to 1 and returns the previous value
 * of the bit.
 *
 * @details Undefined if `s` is `NULL` or if `bit_index` is out of bounds.
 *
 * @param[in,out] s Non-`NULL` pointer to the target `ConcurrentBitset` object
 * to modify.
 * @param[in] bit_index Index of the bit to be set.
 * @param[in] order Memory order to use.
 *
 * @return Previous boolean value of the bit.
 */
bool ConcurrentBitsetSet(ConcurrentBitset *s, int64_t bit_index,
                         memory_order_compat order);

/**
 * @brief Resets the bit at index `bit_index` to 0 and returns the previous
 * value of the bit.
 *
 * @details Undefined if `s` is `NULL` or if `bit_index` is out of bounds.
 *
 * @param[in,out] s Non-`NULL` pointer to the target `ConcurrentBitset` object
 * to modify.
 * @param[in] bit_index Index of the bit to be reset.
 * @param[in] order Memory order to use.
 *
 * @return Previous boolean value of the bit.
 */
bool ConcurrentBitsetReset(ConcurrentBitset *s, int64_t bit_index,
                           memory_order_compat order);

/**
 * @brief Resets all bits in the given `ConcurrentBitset` `s`.
 *
 * @details Undefined if `s` is `NULL`. This function is multithreaded using all
 * OpenMP threads.
 *
 * @note This function is not MT-safe. If `s` is modified by another thread
 * while this function is in progress, the resulting state is undefined.
 *
 * @param[in,out] s Non-`NULL` pointer to the target `ConcurrentBitset` object
 * to modify.
 */
void ConcurrentBitsetResetAllMt(ConcurrentBitset *s);

/**
 * @brief Returns the bit at index `bit_index` as a boolean value.
 *
 * @details Undefined if `s` is `NULL` or if `bit_index` is out of bounds.
 *
 * @param[in] s Non-`NULL` pointer to the source `ConcurrentBitset` object.
 * @param[in] bit_index Index of the bit to be tested.
 * @param[in] order Memory order to use.
 *
 * @return `true` if the bit at index `bit_index` is 1, `false` otherwise.
 */
bool ConcurrentBitsetTest(ConcurrentBitset *s, int64_t bit_index,
                          memory_order_compat order);

/**
 * @brief Returns the amount of memory required in bytes to store the serialized
 * `ConcurrentBitset` object.
 *
 * @param[in] s Non-`NULL` pointer to the `ConcurrentBitset` object to be
 * serialized.
 *
 * @return Amount of memory required in bytes to store the serialized
 * `ConcurrentBitset` object.
 */
size_t ConcurrentBitsetGetSerializedSize(const ConcurrentBitset *s);

/**
 * @brief Serializes at most `bufsize` bytes starting from the `offset`-th byte
 * of `s` into `buf`.
 *
 * @param[in] s Non-null pointer to the `ConcurrentBitset` object to be
 * serialized.
 * @param[in] offset Byte offset of `s` from which serialization begins. Must
 * be a multiple of the internal block size. Otherwise, the function silently
 * returns 0.
 * @param[out] buf Output buffer.
 * @param[in] bufsize Output buffer size in bytes, which must be positive and a
 * multiple of the internal block size (which is typically `sizeof(unsigned long
 * long)`, but may be smaller depending on the platform's lock-free atomic
 * capabilities). If this requirement is not met or any parameter is invalid,
 * the function will silently return 0 without performing any serialization.
 *
 * @return Number of source bytes serialized, which is guaranteed to be a
 * multiple of the internal block size.
 */
size_t ConcurrentBitsetSerializeStreaming(const ConcurrentBitset *s,
                                          size_t offset, void *buf,
                                          size_t bufsize);

/**
 * @brief Deserialize `bufsize` bytes of data from `buf` into `s` starting
 * from its `offset`-th byte.
 *
 * @param[in,out] s Non-null pointer to the destination `ConcurrentBitset`
 * object.
 * @param[in] offset Byte offset of `s` from which deserialization begins. Must
 * be a multiple of the internal block size. Otherwise, the function silently
 * returns 0.
 * @param[in] buf Input buffer that contains serialized data.
 * @param[in] bufsize Size of `buf` in bytes, which must be positive and a
 * multiple of the internal block size (which is typically `sizeof(unsigned long
 * long)`, but may be smaller depending on the platform's lock-free atomic
 * capabilities). If this requirement is not met or any parameter is invalid,
 * the function will silently return 0 without performing any deserialization.
 *
 * @return Number of deserialized bytes, which is guaranteed to be a
 * multiple of the internal block size.
 */
size_t ConcurrentBitsetDeserializeStreaming(ConcurrentBitset *s, size_t offset,
                                            const void *buf, size_t bufsize);

#ifdef __cplusplus
}
#endif

#endif  // GAMESMANONE_CORE_DATA_STRUCTURES_CONCURRENT_BITSET_H_
