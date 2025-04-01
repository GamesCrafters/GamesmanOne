/**
 * @file gamesman_memory.c
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Gamesman memory management system.
 * @version 1.0.0
 * @date 2025-03-29
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
#include "core/gamesman_memory.h"

#include <assert.h>  // assert
#include <lzma.h>    // lzma_physmem
#include <stddef.h>  // size_t, NULL
#include <stdint.h>  // intptr_t
#include <stdio.h>   // fprintf, stderr
#include <stdlib.h>  // aligned_alloc, malloc, calloc, free
#include <string.h>  // memset, memcpy
#include <unistd.h>  // _exit

// #ifdef _OPENMP
#include <omp.h>
// #endif  // _OPENMP

#include "core/types/gamesman_types.h"

static size_t NextMultiple(size_t n, size_t mult) {
    return (n + mult - 1) / mult * mult;
}

void *GamesmanMalloc(size_t size) {
#ifdef _OPENMP
    // If OpenMP is enabled, align to GM_CACHE_LINE_SIZE.
    size_t required_size = NextMultiple(size, GM_CACHE_LINE_SIZE);

    return omp_aligned_alloc(GM_CACHE_LINE_SIZE, required_size,
                             omp_default_mem_alloc);
#else
    // OpenMP is disabled, use normal malloc.
    return malloc(size);
#endif  // _OPENMP
}

void *GamesmanCallocWhole(size_t nmemb, size_t size) {
#ifdef _OPENMP
    // If OpenMP is enabled, align to GM_CACHE_LINE_SIZE.
    size_t required_size = NextMultiple(nmemb * size, GM_CACHE_LINE_SIZE);
    void *ret = omp_aligned_alloc(GM_CACHE_LINE_SIZE, required_size,
                                  omp_default_mem_alloc);
    if (ret == NULL) return ret;
    memset(ret, 0, required_size);

    return ret;
#else
    // OpenMP is disabled, use normal calloc.
    return calloc(nmemb, size);
#endif  // _OPENMP
}

void *GamesmanCallocEach(size_t nmemb, size_t size) {
    assert(size % GM_CACHE_LINE_SIZE == 0);
#ifdef _OPENMP
    // OpenMP is enabled.
    return omp_aligned_calloc(GM_CACHE_LINE_SIZE, nmemb, size,
                              omp_default_mem_alloc);
#else
    // OpenMP is disabled, use normal calloc.
    return calloc(nmemb, size);
#endif  // _OPENMP
}

void *GamesmanRealloc(void *ptr, size_t old_size, size_t new_size) {
    // Free the original space if new_size is 0.
    if (new_size == 0) {
        GamesmanFree(ptr);
        return NULL;
    }

    // Perform plain allocation if ptr is NULL.
    if (ptr == NULL) return GamesmanMalloc(new_size);

#ifdef _OPENMP
    // If OpenMP is enabled, align to GM_CACHE_LINE_SIZE.
    size_t required_size = NextMultiple(new_size, GM_CACHE_LINE_SIZE);
    void *ret = omp_aligned_alloc(GM_CACHE_LINE_SIZE, required_size,
                                  omp_default_mem_alloc);
    if (ret == NULL) return ret;
    memcpy(ret, ptr, (old_size < new_size) ? old_size : new_size);
    omp_free(ptr, omp_default_mem_alloc);

    return ret;
#else
    // OpenMP is disabled, use normal malloc.
    (void)old_size;
    return realloc(ptr, new_size);
#endif  // _OPENMP
}

void *GamesmanAlignedAlloc(size_t alignment, size_t size) {
    assert(alignment > 0);
    assert(alignment % sizeof(void *) == 0);
    assert((alignment & (alignment - 1)) == 0);
#ifdef _OPENMP
    // If OpenMP is enabled, align to max(GM_CACHE_LINE_SIZE, alignment).
    if (GM_CACHE_LINE_SIZE > alignment) alignment = GM_CACHE_LINE_SIZE;
    size_t required_size = NextMultiple(size, alignment);

    return omp_aligned_alloc(alignment, required_size, omp_default_mem_alloc);
#else
    // If OpenMP is disabled, use normal aligned_alloc
    size_t required_size = NextMultiple(size, alignment);

    return aligned_alloc(alignment, required_size);
#endif  // _OPENMP
}

void *GamesmanAlignedCallocWhole(size_t alignment, size_t nmemb, size_t size) {
    assert(alignment > 0);
    assert(alignment % sizeof(void *) == 0);
    assert((alignment & (alignment - 1)) == 0);
#ifdef _OPENMP
    // If OpenMP is enabled, align to max(GM_CACHE_LINE_SIZE, alignment).
    if (GM_CACHE_LINE_SIZE > alignment) alignment = GM_CACHE_LINE_SIZE;
    size_t required_size = NextMultiple(nmemb * size, alignment);
    void *ret =
        omp_aligned_alloc(alignment, required_size, omp_default_mem_alloc);
    if (ret == NULL) return ret;
    memset(ret, 0, required_size);

    return ret;
#else
    // If OpenMP is disabled, use normal aligned_alloc
    size_t required_size = NextMultiple(nmemb * size, alignment);
    void *ret = aligned_alloc(alignment, required_size);
    if (ret == NULL) return ret;
    memset(ret, 0, required_size);

    return ret;
#endif  // _OPENMP
}

void *GamesmanAlignedCallocEach(size_t alignment, size_t nmemb, size_t size) {
    assert(alignment > 0);
    assert(alignment % sizeof(void *) == 0);
    assert((alignment & (alignment - 1)) == 0);
    assert(size % alignment == 0);
    assert(size % GM_CACHE_LINE_SIZE == 0);
#ifdef _OPENMP
    // If OpenMP is enabled, align each element to max(GM_CACHE_LINE_SIZE,
    // alignment).
    if (GM_CACHE_LINE_SIZE > alignment) alignment = GM_CACHE_LINE_SIZE;

    return omp_aligned_calloc(alignment, nmemb, size, omp_default_mem_alloc);
#else
    // If OpenMP is disabled, use normal aligned_alloc
    void *ret = aligned_alloc(alignment, nmemb * size);
    if (ret == NULL) return ret;
    memset(ret, 0, nmemb * size);

    return ret;
#endif  // _OPENMP
}

void *GamesmanAlignedRealloc(size_t alignment, void *ptr, size_t old_size,
                             size_t new_size) {
    assert(alignment > 0);
    assert(alignment % sizeof(void *) == 0);
    assert((alignment & (alignment - 1)) == 0);

    // Free the original space if new_size is 0.
    if (new_size == 0) {
        GamesmanFree(ptr);
        return NULL;
    }

    // Perform plain allocation if ptr is NULL.
    if (ptr == NULL) return GamesmanAlignedAlloc(alignment, new_size);

#ifdef _OPENMP
    // If OpenMP is enabled, align to max(GM_CACHE_LINE_SIZE, alignment).
    if (GM_CACHE_LINE_SIZE > alignment) alignment = GM_CACHE_LINE_SIZE;
    size_t required_size = NextMultiple(new_size, alignment);
    void *ret =
        omp_aligned_alloc(alignment, required_size, omp_default_mem_alloc);
    if (ret == NULL) return ret;
    memcpy(ret, ptr, (old_size < new_size) ? old_size : new_size);
    omp_free(ptr, omp_default_mem_alloc);

    return ret;
#else
    // If OpenMP is disabled, use normal aligned_alloc
    size_t required_size = NextMultiple(new_size, alignment);
    void *ret = aligned_alloc(alignment, required_size);
    if (ret == NULL) return ret;
    memcpy(ret, ptr, (old_size < new_size) ? old_size : new_size);
    free(ptr);

    return ret;
#endif  // _OPENMP
}

void GamesmanFree(void *ptr) {
#ifdef _OPENMP
    omp_free(ptr, omp_default_mem_alloc);
#else
    free(ptr);
#endif
}

intptr_t GetPhysicalMemory(void) { return (intptr_t)lzma_physmem(); }

void *SafeMalloc(size_t size) {
    void *ret = GamesmanMalloc(size);
    if (ret == NULL) {
        fprintf(stderr,
                "SafeMalloc: failed to allocate %zd bytes. This ususally "
                "indicates a bug.\n",
                size);
        fflush(stderr);
        _exit(kMallocFailureError);
    }
    return ret;
}

void *SafeCalloc(size_t n, size_t size) {
    void *ret = GamesmanCallocWhole(n, size);
    if (ret == NULL) {
        fprintf(stderr,
                "SafeCalloc: failed to allocate %zd elements each of %zd "
                "bytes. This ususally "
                "indicates a bug.\n",
                n, size);
        fflush(stderr);
        _exit(kMallocFailureError);
    }
    return ret;
}
