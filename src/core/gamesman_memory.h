/**
 * @file gamesman_memory.h
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Gamesman memory management system. All provided functions are
 * MT-safe unless otherwise noted.
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

#ifndef GAMESMANONE_CORE_GAMESMAN_MEMORY_H_
#define GAMESMANONE_CORE_GAMESMAN_MEMORY_H_

#include <assert.h>
#include <stddef.h>

#include "config.h"

// =================================== Macro ===================================

static_assert(GM_CACHE_LINE_SIZE > 0,
              "GM_CACHE_LINE_SIZE is not defined as a positive value");
static_assert(
    (GM_CACHE_LINE_SIZE % sizeof(void *)) == 0,
    "GM_CACHE_LINE_SIZE is not defined as a multiple of pointer size");
static_assert((GM_CACHE_LINE_SIZE & (GM_CACHE_LINE_SIZE - 1)) == 0,
              "GM_CACHE_LINE_SIZE is not defined as a power of 2");

/**
 * @brief Returns the number of bytes to be padded to an object of size `n` so
 * that its size becomes a multiple of `GM_CACHE_LINE_SIZE`.
 *
 * @param[in] n Size of the object in bytes.
 *
 * @return The number of bytes to be padded.
 */
#define GM_CACHE_LINE_PAD(n)                                    \
    ((((n) + (GM_CACHE_LINE_SIZE) - 1) / (GM_CACHE_LINE_SIZE) * \
      (GM_CACHE_LINE_SIZE)) -                                   \
     n)

// ================================= Allocator =================================

/**
 * @brief Options for configuring a `GamesmanAllocator`.
 */
typedef struct GamesmanAllocatorOptions {
    size_t alignment; /**< Alignment in bytes, or 0 for default alignment. */
    size_t pool_size; /**< Maximum memory pool size in bytes. */
} GamesmanAllocatorOptions;

/**
 * @brief Populates the given allocator options with default values.
 *
 * @note This function is not MT-safe.
 *
 * @param[out] options Pointer to the options struct to be populated.
 */
void GamesmanAllocatorOptionsSetDefaults(GamesmanAllocatorOptions *options);

/**
 * @brief Opaque allocator type.
 */
typedef struct GamesmanAllocator GamesmanAllocator;

/**
 * @brief Creates a new `GamesmanAllocator` object using the provided `options`,
 * setting its reference count to 1.
 *
 * @details If `options` is `NULL`, the default settings will be used. If an
 * alignment is provided, it must be a positive integral multiple of pointer
 * size and a power of 2. To prevent memory leaks, the returned object must be
 * deallocated using the `GamesmanAllocatorRelease` function.
 *
 * @param[in] options Allocator options, or `NULL` for defaults.
 *
 * @return Pointer to a new `GamesmanAllocator` object.
 * @retval NULL If the alignment is invalid or memory allocation fails.
 */
GamesmanAllocator *GamesmanAllocatorCreate(
    const GamesmanAllocatorOptions *options);

/**
 * @brief Increments the reference count of the given `allocator`.
 *
 * @details Does nothing if `allocator` is `NULL`. Otherwise, the function
 * assumes that `allocator` has been initialized and has not been deallocated.
 *
 * @param[in,out] allocator Allocator object to retain.
 *
 * @return The `allocator` parameter passed in.
 * @retval NULL If `allocator` is `NULL`.
 */
GamesmanAllocator *GamesmanAllocatorAddRef(GamesmanAllocator *allocator);

/**
 * @brief Decrements the reference count of the given `allocator`, deallocating
 * it if its reference count decreases to 0.
 *
 * @details Does nothing if `NULL` is provided.
 *
 * @param[in,out] allocator The `GamesmanAllocator` object to release.
 */
void GamesmanAllocatorRelease(GamesmanAllocator *allocator);

/**
 * @brief Returns the remaining size of the memory pool allotted to the given
 * `allocator` in number of bytes.
 *
 * @note In a multithreaded context, simply testing the remaining pool size with
 * this function is not sufficient to guarantee that the next allocation of a
 * smaller size will succeed. The caller of `GamesmanAllocatorAllocate` still
 * needs to test if the pointer returned is `NULL`.
 *
 * @param[in] allocator Allocator to query.
 *
 * @return The remaining size of the memory pool in bytes.
 */
size_t GamesmanAllocatorGetRemainingPoolSize(
    const GamesmanAllocator *allocator);

/**
 * @brief Allocates a space of size at least `size` bytes using the given
 * `allocator`.
 *
 * @details If `allocator` is `NULL`, the call is equivalent to
 * `GamesmanMalloc(size)`. To prevent memory leaks, the returned pointer must be
 * deallocated using `GamesmanAllocatorDeallocate` with the same `allocator`.
 *
 * @param[in,out] allocator Allocator to use, or `NULL` for default allocation.
 * @param[in] size Minimum number of bytes to allocate.
 *
 * @return Pointer to the allocated space.
 * @retval NULL If `size` is 0, the memory pool is exhausted, or allocation
 * fails.
 */
void *GamesmanAllocatorAllocate(GamesmanAllocator *allocator, size_t size);

/**
 * @brief Deallocates the space at `ptr`, adding its size back to the memory
 * pool.
 *
 * @details The space is assumed to be previously allocated by the given
 * `allocator`. If `allocator` is `NULL`, the call is equivalent to
 * `GamesmanFree(ptr)`. Does nothing if `ptr` is `NULL`.
 *
 * @param[in,out] allocator Allocator that was used to allocate the space.
 * @param[in] ptr Pointer to the space to deallocate.
 */
void GamesmanAllocatorDeallocate(GamesmanAllocator *allocator, void *ptr);

// =========================== Memory Allocation API ===========================

/**
 * @brief Returns a space of size at least `size` bytes.
 *
 * @details If Gamesman is built with multithreading enabled, the requested size
 * is padded to a multiple of `GM_CACHE_LINE_SIZE` and the returned memory
 * address will be aligned to at least the `GM_CACHE_LINE_SIZE` byte boundary.
 * To prevent memory leaks, the returned pointer must be deallocated using
 * `GamesmanFree`.
 *
 * @note `size` does not need to be a multiple of `GM_CACHE_LINE_SIZE`.
 *
 * @param[in] size Minimum size of memory to allocate in bytes.
 *
 * @return Pointer to the allocated space.
 * @retval NULL On failure.
 */
void *GamesmanMalloc(size_t size);

/**
 * @brief Returns a zero-initialized space of size enough to hold at least
 * `nmemb` elements of `size` bytes each.
 *
 * @details If Gamesman is built with multithreading enabled, the requested size
 * is padded to a multiple of `GM_CACHE_LINE_SIZE` and the returned memory
 * address will be aligned to at least the `GM_CACHE_LINE_SIZE` byte boundary.
 * To prevent memory leaks, the returned pointer must be deallocated using
 * `GamesmanFree`.
 *
 * @note When memory alignment is applied, the allocated space is aligned as a
 * whole - there is no guarantee that each individual element is aligned to the
 * cache line boundary. The resulting size to be allocated does not need to be a
 * multiple of `GM_CACHE_LINE_SIZE`.
 *
 * @param[in] nmemb Number of elements.
 * @param[in] size Size of each element in bytes.
 *
 * @return Pointer to the allocated space.
 * @retval NULL On failure.
 */
void *GamesmanCallocWhole(size_t nmemb, size_t size);

/**
 * @brief Returns a space of size at least `size` bytes aligned to the boundary
 * of at least `alignment` bytes.
 *
 * @details If Gamesman is built with multithreading enabled, the returned
 * memory address will be aligned to `GM_CACHE_LINE_SIZE` if the provided
 * `alignment` is smaller. To prevent memory leaks, the returned pointer must be
 * deallocated using `GamesmanFree`.
 *
 * @param[in] alignment Specifies the alignment in bytes, which must be a
 * positive integral multiple of `sizeof(void *)` and a power of 2.
 * @param[in] size Minimum size of memory to allocate in bytes. Does not need to
 * be a multiple of `alignment`.
 *
 * @return Pointer to the allocated space.
 * @retval NULL On failure.
 */
void *GamesmanAlignedAlloc(size_t alignment, size_t size);

/**
 * @brief Returns an `alignment` byte aligned zero-initialized space of size
 * enough to hold at least `nmemb` elements of `size` bytes each.
 *
 * @details If Gamesman is built with multithreading enabled, the returned
 * memory address will be aligned to `GM_CACHE_LINE_SIZE` if the provided
 * `alignment` is smaller. To prevent memory leaks, the returned pointer must be
 * deallocated using `GamesmanFree`.
 *
 * @note The allocated space is aligned as a whole - there is no guarantee that
 * each element is aligned to the given `alignment`.
 *
 * @param[in] alignment Specifies the alignment in bytes, which must be a
 * positive integral multiple of `sizeof(void *)` and a power of 2.
 * @param[in] nmemb Number of elements.
 * @param[in] size Size of each element in bytes.
 *
 * @return Pointer to the allocated space.
 * @retval NULL On failure.
 */
void *GamesmanAlignedCallocWhole(size_t alignment, size_t nmemb, size_t size);

/**
 * @brief Deallocates the space pointed to by `ptr`.
 *
 * @details The pointer is assumed to be previously returned by one of the
 * memory allocation functions provided by the Gamesman memory management
 * system. Does nothing if `ptr` is `NULL`.
 *
 * @param[in] ptr Pointer to the space to deallocate.
 */
void GamesmanFree(void *ptr);

/**
 * @brief Returns the amount of physical memory available on the system in
 * bytes.
 *
 * @return Amount of physical memory in bytes.
 * @retval 0 If the detection fails.
 */
size_t GetPhysicalMemory(void);

/**
 * @brief Same behavior as `GamesmanMalloc` on success; terminates GAMESMAN on
 * failure.
 *
 * @param[in] size Minimum size of memory to allocate in bytes.
 *
 * @return Pointer to the allocated space.
 */
void *SafeMalloc(size_t size);

/**
 * @brief Same behavior as `GamesmanCallocWhole` on success; terminates GAMESMAN
 * on failure.
 *
 * @param[in] n Number of elements.
 * @param[in] size Size of each element in bytes.
 *
 * @return Pointer to the allocated space.
 */
void *SafeCalloc(size_t n, size_t size);

#endif  // GAMESMANONE_CORE_GAMESMAN_MEMORY_H_
