/**
 * @file gamesman_memory.h
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Gamesman memory management system. All provided functions are
 * thread-safe unless otherwise noted.
 * @version 1.0.0
 * @date 2025-04-04
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

#include <assert.h>  // static_assert
#include <stddef.h>  // size_t

#include "config.h"

////////////
// MACROS //
////////////

static_assert(GM_CACHE_LINE_SIZE > 0,
              "GM_CACHE_LINE_SIZE is not defined as a positive value");
static_assert(
    (GM_CACHE_LINE_SIZE % sizeof(void *)) == 0,
    "GM_CACHE_LINE_SIZE is not defined as a multiple of pointer size");
static_assert((GM_CACHE_LINE_SIZE & (GM_CACHE_LINE_SIZE - 1)) == 0,
              "GM_CACHE_LINE_SIZE is not defined as a power of 2");

/**
 * @brief Returns the number of bytes to be padded to an object of size \p n so
 * that its size becomes a multiple of \c GM_CACHE_LINE_SIZE.
 *
 * @param n Size of the object.
 * @return The number of bytes to be padded to an object of size \p n so
 * that its size becomes a multiple of \c GM_CACHE_LINE_SIZE.
 */
#define GM_CACHE_LINE_PAD(n)                                    \
    ((((n) + (GM_CACHE_LINE_SIZE) - 1) / (GM_CACHE_LINE_SIZE) * \
      (GM_CACHE_LINE_SIZE)) -                                   \
     n)

///////////////
// ALLOCATOR //
///////////////

typedef struct GamesmanAllocatorOptions {
    size_t alignment;
    size_t pool_size;
} GamesmanAllocatorOptions;

void GamesmanAllocatorOptionsSetDefaults(GamesmanAllocatorOptions *options);

/**
 * @brief Opaque allocator type.
 */
typedef struct GamesmanAllocator GamesmanAllocator;

/**
 * @brief Creates a new GamesmanAllocator object using the \p options provided,
 * setting its reference count to 1. If \c NULL is provided, the default
 * settings will be used. To prevent memory leak, the returned object must be
 * deallocated using the GamesmanAllocatorRelease function.
 *
 * @param options Allocator options.
 * @return Pointer to a new GamesmanAllocator object, or
 * @return \c NULL if the Allocator cannot be created.
 */
GamesmanAllocator *GamesmanAllocatorCreate(
    const GamesmanAllocatorOptions *options);

/**
 * @brief Increments the reference count of the given \p allocator and returns
 * it. Does nothing if \p allocator is \c NULL. Otherwise, the function assumes
 * that \p allocator has been initialized and has not been deallocated.
 *
 * @param allocator Allocator object to retain.
 * @return The \p allocator parameter passed in, or
 * @return \c NULL if \p allocator is \c NULL.
 */
GamesmanAllocator *GamesmanAllocatorAddRef(GamesmanAllocator *allocator);

/**
 * @brief Decrements the reference count of the given \p allocator, deallocating
 * it if its reference count decreases to 0. Does nothing if \c NULL is
 * provided.
 *
 * @param allocator The GamesmanAllocator object to release.
 */
void GamesmanAllocatorRelease(GamesmanAllocator *allocator);

/**
 * @brief Returns the remaining size of the memory pool allotted to the given
 * \p allocator in number of bytes.
 *
 * @note In a multithreaded context, simply testing the remaining pool size with
 * this function is not sufficient to guarantee that the next allocation of a
 * smaller size will succeed. The caller of GamesmanAllocatorAllocate still
 * needs to test if the pointer returned is \c NULL.
 *
 * @param allocator Allocator to use.
 * @return The remaining size of the memory pool in bytes.
 */
size_t GamesmanAllocatorGetRemainingPoolSize(
    const GamesmanAllocator *allocator);

/**
 * @brief Allocates a space of size at least \p size bytes using the given
 * \p allocator. If \p allocator is \c NULL, the call is equivalent to
 * GamesmanMalloc(size). Returns \p NULL if \p size is 0 or on failure.
 * To prevent memory leak, the returned pointer must be deallocated using
 * GamesmanAllocatorDeallocate with the same \p allocator.
 *
 * @param allocator Allocator to use.
 * @param size Minimum number of bytes to allocate.
 * @return Pointer to the allocated space, or
 * @return \c NULL if \p size is 0 or on failure.
 */
void *GamesmanAllocatorAllocate(GamesmanAllocator *allocator, size_t size);

/**
 * @brief Deallocates the space at \p ptr, which is assumed to be previously
 * allocated by the given \p allocator. If \p allocator is \p NULL, the call is
 * equivalent to GamesmanFree(ptr). Does nothing if \p ptr is \c NULL.
 *
 * @param allocator Allocator that was used to allocate the provided space.
 * @param ptr Pointer to the space to deallocate.
 */
void GamesmanAllocatorDeallocate(GamesmanAllocator *allocator, void *ptr);

///////////////////////////
// MEMORY ALLOCATION API //
///////////////////////////

/**
 * @brief Returns a space of size at least \p size bytes. If Gamesman is
 * built with multithreading enabled, the returned memory address will also
 * be aligned at least to the \c GM_CACHE_LINE_SIZE -byte boundary. Returns
 * \c NULL on failure. To prevent memory leak, the returned pointer must be
 * deallocated using the GamesmanFree function.
 *
 * @note \p size does not need to be a multiple of \c GM_CACHE_LINE_SIZE.
 *
 * @param size Minimum size of memory to allocate in bytes.
 * @return Pointer to the allocated space, or
 * @return \c NULL on failure.
 */
void *GamesmanMalloc(size_t size);

/**
 * @brief Returns a zero-initialized space of size enough to hold at least
 * \p nmemb elements of \p size bytes each. If Gamesman is built with
 * multithreading enabled, the returned memory address will also be aligned at
 * least to the \c GM_CACHE_LINE_SIZE -byte boundary. Returns \c NULL on
 * failure. To prevent memory leak, the returned pointer must be deallocated
 * using the GamesmanFree function.
 *
 * @note When memory alignemnt is applied, the allocated space is aligned as a
 * whole - there is no guarantee that each element is aligned to the cache line
 * boundary.
 * @note The resulting size to be allocated does not need to be a multiple of
 * \c GM_CACHE_LINE_SIZE.
 *
 * @param nmemb Number of elements.
 * @param size Size of each element.
 * @return Pointer to the allocated space, or
 * @return \c NULL on failure.
 */
void *GamesmanCallocWhole(size_t nmemb, size_t size);

/**
 * @brief Returns a zero-initialized space of size enough to hold at least
 * \p nmemb elements of \p size bytes each. If Gamesman is built with
 * multithreading enabled, each element will also be aligned at least to the
 * \c GM_CACHE_LINE_SIZE -byte boundary. Since this function may be called
 * with or without multithreading enabled, \p size must always be a
 * multiple of \c GM_CACHE_LINE_SIZE or the behavior is undefined. Returns
 * \c NULL on failure. To prevent memory leak, the returned pointer must be
 * deallocated using the GamesmanFree function.
 *
 * @param nmemb Number of elements.
 * @param size Size of each element. Must be a multiple of
 * \c GM_CACHE_LINE_SIZE.
 * @return Pointer to the allocated space, or
 * @return \c NULL on failure.
 */
void *GamesmanCallocEach(size_t nmemb, size_t size);

/**
 * @brief Reallocates the space of size \p old_size and pointed to by \p ptr to
 * be of size at least \p new_size bytes. If Gamesman is built with
 * multithreading enabled, the returned memory address will also be aligned at
 * least to the \c GM_CACHE_LINE_SIZE -byte boundary. The \p ptr must be
 * previously allocated using one of the memory allocation functions provided by
 * the Gamesman memory management system. On success, \p ptr will be deallocated
 * using GamesmanFree and a pointer to the new space is returned. Otherwise, the
 * original space is unmodified and \c NULL is returned. If \p new_size is 0,
 * \c NULL will be returned and the original space will be deallocated. To
 * prevent memory leak, the returned pointer must be deallocated using the
 * GamesmanFree function.
 *
 * @note \p old_size and \p new_size do not have to be multiples of
 * \c GM_CACHE_LINE_SIZE.
 *
 * @param ptr Pointer to the original space allocated by one of the memory
 * allocation functions provided by the Gamesman memory management system.
 * @param old_size Size of the original space in bytes.
 * @param new_size Minimum size of the new space in bytes.
 * @return Pointer to the allocated space, or
 * @return \c NULL on failure.
 */
void *GamesmanRealloc(void *ptr, size_t old_size, size_t new_size);

/**
 * @brief Returns a space of size at least \p size bytes aligned to the boundary
 * of at least \p alignment bytes. If Gamesman is built with multithreading
 * enabled, the returned memory address will also be aligned at least to the
 * \c GM_CACHE_LINE_SIZE -byte boundary. Returns \c NULL on failure. To prevent
 * memory leak, the returned pointer must be deallocated using the GamesmanFree
 * function.
 *
 * @param alignment Specifies the alignment in bytes, which must be a positive
 * integral multiple of sizeof(void *) and a power of 2.
 * @param size Minimum size of memory to allocated in bytes. Does not need to be
 * a multiple of \p alignment.
 * @return Pointer to the allocated space, or
 * @return \c NULL on failure.
 */
void *GamesmanAlignedAlloc(size_t alignment, size_t size);

/**
 * @brief Returns a \p alignment -byte aligned zero-initialized space of size
 * enough to hold at least \p nmemb elements of \p size bytes each. If Gamesman
 * is built with multithreading enabled, the returned memory address will also
 * be aligned at least to the \c GM_CACHE_LINE_SIZE -byte boundary. Returns
 * \c NULL on failure. To prevent memory leak, the returned pointer must be
 * deallocated using the GamesmanFree function.
 *
 * @note The allocated space is aligned as a whole - there is no guarantee that
 * each element is aligned to the given \p alignment.
 *
 * @param alignment Specifies the alignment in bytes, which must be a positive
 * integral multiple of sizeof(void *) and a power of 2.
 * @param nmemb Number of elements.
 * @param size Size of each element.
 * @return Pointer to the allocated space, or
 * @return \c NULL on failure.
 */
void *GamesmanAlignedCallocWhole(size_t alignment, size_t nmemb, size_t size);

/**
 * @brief Returns a zero-initialized space of size enough to hold at least
 * \p nmemb \p alignemnt -byte aligned elements of \p size bytes each, where
 * \p size is assumed to be a multiple of \p alignment. If Gamesman is built
 * with multithreading enabled, each element will also be aligned at least to
 * the \c GM_CACHE_LINE_SIZE -byte boundary. Since this function may be called
 * with or without multithreading enabled, \p size must always be a multiple of
 * max( \c GM_CACHE_LINE_SIZE , \p alignment ) or the behavior is undefined.
 * Returns \c NULL on failure. To prevent memory leak, the returned pointer must
 * be deallocated using the GamesmanFree function.
 *
 * @param alignment Specifies the alignment in bytes, which must be a positive
 * integral multiple of sizeof(void *) and a power of 2.
 * @param nmemb Number of elements.
 * @param size Size of each element. Must be a multiple of max(
 * \c GM_CACHE_LINE_SIZE , \p alignment ).
 * @return Pointer to the allocated space, or
 * @return \c NULL on failure.
 */
void *GamesmanAlignedCallocEach(size_t alignment, size_t nmemb, size_t size);

/**
 * @brief Reallocates the space of size \p old_size and pointed to by \p ptr to
 * be of size at least \p new_size bytes and aligned to the boundary of at least
 * \p alignment bytes. If Gamesman is built with multithreading enabled, the
 * returned memory address will also be aligned at least to the
 * \c GM_CACHE_LINE_SIZE -byte boundary. The \p ptr must be previously allocated
 * using one of the memory allocation functions provided by the Gamesman memory
 * management system. On success, \p ptr will be deallocated using GamesmanFree
 * and a pointer to the new space is returned. Otherwise, the original space is
 * unmodified and \c NULL is returned. If \p size is 0, \p NULL will be returned
 * and the original space will be deallocated. To prevent memory leak, the
 * returned pointer must be deallocated using the GamesmanFree function.
 *
 * @param alignment Specifies the alignment in bytes, which must be a positive
 * integral multiple of sizeof(void *) and a power of 2.
 * @param ptr Pointer to the original space allocated by one of the memory
 * allocation functions provided by the Gamesman memory management system.
 * @param old_size Size of the original space in bytes.
 * @param new_size Minimum size of the new space in bytes. Does not need to be
 * a multiple of \p alignment.
 * @return Pointer to the allocated space, or
 * @return \c NULL on failure.
 */
void *GamesmanAlignedRealloc(size_t alignment, void *ptr, size_t old_size,
                             size_t new_size);

/**
 * @brief Deallocates the space pointed to by \p ptr, which is assumed to be
 * previously returned by one of the memory allocation functions provided by the
 * Gamesman memory management system. Does nothing if \p ptr is \c NULL.
 *
 * @param ptr Pointer to the space to deallocate.
 */
void GamesmanFree(void *ptr);

/**
 * @brief Returns the amount of physical memory available on the system in
 * bytes.
 *
 * @return Amount of physical memory in bytes or
 * @return 0 if the detection fails.
 */
size_t GetPhysicalMemory(void);

/**
 * @brief Same behavior as GamesmanMalloc on success; terminates GAMESMAN on
 * failure.
 */
void *SafeMalloc(size_t size);

/**
 * @brief Same behavior as GamesmanCalloc on success; terminates GAMESMAN on
 * failure.
 */
void *SafeCalloc(size_t n, size_t size);

#endif  // GAMESMANONE_CORE_GAMESMAN_MEMORY_H_
