/**
 * @file gamesman_memory.c
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Implementation of the Gamesman memory management system.
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
#include "core/gamesman_memory.h"

#include <assert.h>
#include <lzma.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "config.h"  // IWYU pragma: keep

#ifdef _OPENMP
#include <omp.h>
#endif  // _OPENMP

#include "core/concurrency.h"
#include "core/types/gamesman_error.h"

///////////////
// ALLOCATOR //
///////////////

static const GamesmanAllocatorOptions kDefaultAllocatorOptions = {
    .alignment = 0,
    .pool_size = SIZE_MAX,
};

void GamesmanAllocatorOptionsSetDefaults(GamesmanAllocatorOptions *options) {
    *options = kDefaultAllocatorOptions;
}

struct GamesmanAllocator {
    size_t alignment;
    ConcurrentSizeType pool_size;
    ConcurrentSizeType ref_count;
};

static bool IsValidAlignmentForAllocatorOptions(size_t alignment) {
    // Alignment must be strictly positive
    if (alignment == 0) {
        return true;
    }

    // Alignment must be a multiple of pointer size
    if ((alignment % sizeof(void *)) != 0) {
        return false;
    }

    // Alignment must be a power of 2; this formula works because we already
    // verified alignment != 0
    if ((alignment & (alignment - 1)) != 0) {
        return false;
    }

    return true;
}

GamesmanAllocator *GamesmanAllocatorCreate(
    const GamesmanAllocatorOptions *options) {
    // Verify alignment if provided by the caller
    if (options && !IsValidAlignmentForAllocatorOptions(options->alignment)) {
        return NULL;
    }

    GamesmanAllocator *ret =
        (GamesmanAllocator *)GamesmanMalloc(sizeof(GamesmanAllocator));
    if (!ret) {
        return NULL;
    }

    if (!options) {
        options = &kDefaultAllocatorOptions;
    }
    ret->alignment = options->alignment;
    ConcurrentSizeTypeInit(&ret->pool_size, options->pool_size);
    ConcurrentSizeTypeInit(&ret->ref_count, 1);

    return ret;
}

GamesmanAllocator *GamesmanAllocatorAddRef(GamesmanAllocator *allocator) {
    if (!allocator) {
        return NULL;
    }

    ConcurrentSizeTypeAddExplicit(&allocator->ref_count, 1,
                                  kConcurrencyMemoryOrderRelaxed);

    return allocator;
}

void GamesmanAllocatorRelease(GamesmanAllocator *allocator) {
    if (!allocator) {
        return;
    }

    if (ConcurrentSizeTypeSubtractExplicit(
            &allocator->ref_count, 1, kConcurrencyMemoryOrderRelaxed) == 1) {
        GamesmanFree(allocator);
    }
}

size_t GamesmanAllocatorGetRemainingPoolSize(
    const GamesmanAllocator *allocator) {
    //
    return ConcurrentSizeTypeLoadExplicit(&allocator->pool_size,
                                          kConcurrencyMemoryOrderRelaxed);
}

typedef struct AllocHeader {
    /** Total amount of memory in bytes allocated from memory pool, including
     * this header. */
    size_t size;
} AllocHeader;

static size_t NextMultiple(size_t n, size_t mult) {
    return (n + mult - 1) / mult * mult;
}

// Assumes alignment is either 0 or a valid amount.
static size_t GetHeaderSize(size_t alignment) {
#ifdef _OPENMP
    // This also handles the case when alignment is 0.
    if (GM_CACHE_LINE_SIZE > alignment) {
        alignment = GM_CACHE_LINE_SIZE;
    }

    return NextMultiple(sizeof(AllocHeader), alignment);
#else
    if (alignment) {
        return NextMultiple(sizeof(AllocHeader), alignment);
    }

    return sizeof(AllocHeader);
#endif  // _OPENMP
}

static void WriteHeader(void *dest, const AllocHeader *header) {
    *((AllocHeader *)dest) = *header;
}

#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wanalyzer-malloc-leak"
#endif
void *GamesmanAllocatorAllocate(GamesmanAllocator *allocator, size_t size) {
    // If no allocator is provided, use default allocation function.
    if (!allocator) {
        return GamesmanMalloc(size);
    }

    if (size == 0) {
        return NULL;
    }

    // Make an attempt to reserve space from the memory pool. We must also take
    // the header into account.
    size_t header_size = GetHeaderSize(allocator->alignment);

    // Prevent overflow when calculating (header_size + size)
    if (size > SIZE_MAX - header_size) {
        return NULL;
    }

    size_t alloc_size = header_size + size;  // Total amount to allocate.
    bool success = ConcurrentSizeTypeSubtractIfGreaterEqualExplicit(
        &allocator->pool_size, alloc_size, kConcurrencyMemoryOrderRelaxed,
        kConcurrencyMemoryOrderRelaxed);
    if (!success) {
        return NULL;  // Not enough memory left in the pool
    }

    // There is enough space in the pool. Make an allocation large enough for
    // the specified size and a header.
    void *space;
    if (allocator->alignment) {  // Alignment amount specified.
        space = GamesmanAlignedAlloc(allocator->alignment, alloc_size);
    } else {  // Use default alignment.
        space = GamesmanMalloc(alloc_size);
    }

    // Roll back the pool subtraction on underlying allocation failure.
    if (!space) {
        ConcurrentSizeTypeAddExplicit(&allocator->pool_size, alloc_size,
                                      kConcurrencyMemoryOrderRelaxed);
        return NULL;
    }

    // Write the header to the beginning of the allocated space.
    WriteHeader(space, &(AllocHeader){.size = alloc_size});

    // Return the space after the header.
    return (void *)((char *)space + header_size);
}
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic pop
#endif

void GamesmanAllocatorDeallocate(GamesmanAllocator *allocator, void *ptr) {
    // If no allocator is provided, use default deallocation function.
    if (!allocator) {
        GamesmanFree(ptr);
        return;
    }

    // Do nothing if ptr is NULL.
    if (!ptr) {
        return;
    }

    // Read allocation size from header.
    size_t header_size = GetHeaderSize(allocator->alignment);
    void *space = (void *)((char *)ptr - header_size);
    const AllocHeader *header = (const AllocHeader *)space;
    size_t alloc_size = header->size;

    // Deallocate the space.
    GamesmanFree(space);

    // Add size back to the memory pool after the space has been deallocated.
    ConcurrentSizeTypeAddExplicit(&allocator->pool_size, alloc_size,
                                  kConcurrencyMemoryOrderRelaxed);
}

///////////////////////////
// MEMORY ALLOCATION API //
///////////////////////////

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
    if (!ret) {
        return ret;
    }

    memset(ret, 0, required_size);

    return ret;
#else
    // OpenMP is disabled, use normal calloc.
    return calloc(nmemb, size);
#endif  // _OPENMP
}

void *GamesmanAlignedAlloc(size_t alignment, size_t size) {
    assert(alignment > 0);
    assert(alignment % sizeof(void *) == 0);
    assert((alignment & (alignment - 1)) == 0);
#ifdef _OPENMP
    // If OpenMP is enabled, align to max(GM_CACHE_LINE_SIZE, alignment).
    if (GM_CACHE_LINE_SIZE > alignment) {
        alignment = GM_CACHE_LINE_SIZE;
    }

    // omp_aligned_alloc requires allocation size to be a multiple of alignment
    size_t required_size = NextMultiple(size, alignment);

    return omp_aligned_alloc(alignment, required_size, omp_default_mem_alloc);
#else
    // If OpenMP is disabled, use normal aligned_alloc, which requires
    // allocation size to be a multiple of alignment
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
    if (GM_CACHE_LINE_SIZE > alignment) {
        alignment = GM_CACHE_LINE_SIZE;
    }

    size_t required_size = NextMultiple(nmemb * size, alignment);
    void *ret =
        omp_aligned_alloc(alignment, required_size, omp_default_mem_alloc);
    if (!ret) {
        return ret;
    }

    memset(ret, 0, required_size);

    return ret;
#else
    // If OpenMP is disabled, use normal aligned_alloc
    size_t required_size = NextMultiple(nmemb * size, alignment);
    void *ret = aligned_alloc(alignment, required_size);
    if (!ret) {
        return ret;
    }

    memset(ret, 0, required_size);

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

size_t GetPhysicalMemory(void) { return (size_t)lzma_physmem(); }

void *SafeMalloc(size_t size) {
    void *ret = GamesmanMalloc(size);
    if (!ret) {
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
    if (!ret) {
        fprintf(stderr,
                "SafeCalloc: failed to allocate %zd elements each of %zd "
                "bytes. This ususally indicates a bug.\n",
                n, size);
        fflush(stderr);
        _exit(kMallocFailureError);
    }

    return ret;
}
