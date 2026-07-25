#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <vector>

extern "C" {
#include "config.h"
#include "core/gamesman_memory.h"
}

// =================================== Macro ===================================

// Verifies that a size of exactly zero correctly evaluates to needing zero
// bytes of padding.
TEST(CacheLinePadTest, ZeroSize) { EXPECT_EQ(GM_CACHE_LINE_PAD(0), 0); }

// Verifies that sizes that are exact multiples of the cache line size require
// no additional padding.
TEST(CacheLinePadTest, ExactMultiple) {
    EXPECT_EQ(GM_CACHE_LINE_PAD(GM_CACHE_LINE_SIZE), 0);
    EXPECT_EQ(GM_CACHE_LINE_PAD(GM_CACHE_LINE_SIZE * 2), 0);
    EXPECT_EQ(GM_CACHE_LINE_PAD(GM_CACHE_LINE_SIZE * 10), 0);
}

// Verifies that a single byte allocation forces almost an entire cache line of
// padding to reach the next boundary.
TEST(CacheLinePadTest, SmallValue) {
    EXPECT_EQ(GM_CACHE_LINE_PAD(1), GM_CACHE_LINE_SIZE - 1);
}

// Verifies the upper boundary condition where the size falls exactly one byte
// short of a full cache line multiple.
TEST(CacheLinePadTest, OneByteUnder) {
    EXPECT_EQ(GM_CACHE_LINE_PAD(GM_CACHE_LINE_SIZE - 1), 1);
    EXPECT_EQ(GM_CACHE_LINE_PAD((GM_CACHE_LINE_SIZE * 3) - 1), 1);
}

// Verifies the lower boundary condition where the size spills exactly one byte
// over a full cache line multiple.
TEST(CacheLinePadTest, OneByteOver) {
    EXPECT_EQ(GM_CACHE_LINE_PAD(GM_CACHE_LINE_SIZE + 1),
              GM_CACHE_LINE_SIZE - 1);
    EXPECT_EQ(GM_CACHE_LINE_PAD((GM_CACHE_LINE_SIZE * 4) + 1),
              GM_CACHE_LINE_SIZE - 1);
}

// Verifies that the macro safely handles larger memory block padding math
// without truncating or evaluating poorly.
TEST(CacheLinePadTest, LargeMemoryBlock) {
    // 1MB memory block plus half a cache line offset to verify boundary
    // arithmetic at higher scales
    const size_t one_mb = 1 << 20;
    ASSERT_TRUE(one_mb % GM_CACHE_LINE_SIZE == 0)
        << "GM_CACHE_LINE_SIZE == " << GM_CACHE_LINE_SIZE
        << " does not divide 1 MiB. This is very likely the result of a bug "
           "in the cache line size detection program.";
    const size_t large_size = one_mb + (GM_CACHE_LINE_SIZE / 2);
    EXPECT_EQ(GM_CACHE_LINE_PAD(large_size), GM_CACHE_LINE_SIZE / 2);
}

// ================================= Allocator =================================

// Verifies that populating default options behaves as specified.
TEST(GamesmanAllocatorOptionsSetDefaultsTest, OverwritesExistingData) {
    GamesmanAllocatorOptions options = {9999, 9999};

    GamesmanAllocatorOptionsSetDefaults(&options);

    EXPECT_EQ(options.alignment, 0) << "GamesmanAllocatorOptionsSetDefaults() "
                                       "should set alignment to 0 by default";
    EXPECT_EQ(options.pool_size, SIZE_MAX)
        << "GamesmanAllocatorOptionsSetDefaults() should set alignment to "
           "SIZE_MAX by default";
}

// Verifies that passing NULL options creates an allocator with default settings
// successfully.
TEST(GamesmanAllocatorCreateTest, NullOptionsCreatesWithDefaults) {
    GamesmanAllocator *allocator = GamesmanAllocatorCreate(nullptr);

    ASSERT_NE(allocator, nullptr);
    EXPECT_EQ(GamesmanAllocatorGetRemainingPoolSize(allocator), SIZE_MAX);

    GamesmanAllocatorRelease(allocator);
}

// Verifies that an allocator can be successfully created using an explicitly
// populated default options struct.
TEST(GamesmanAllocatorCreateTest, ExplicitDefaultOptions) {
    GamesmanAllocatorOptions default_options;
    GamesmanAllocatorOptionsSetDefaults(&default_options);
    GamesmanAllocator *allocator = GamesmanAllocatorCreate(&default_options);
    ASSERT_NE(allocator, nullptr);
    EXPECT_EQ(GamesmanAllocatorGetRemainingPoolSize(allocator), SIZE_MAX);
    GamesmanAllocatorRelease(allocator);
}

// Verifies that providing a valid custom alignment (power of 2 and a multiple
// of pointer size) succeeds.
TEST(GamesmanAllocatorCreateTest, ValidCustomAlignment) {
    GamesmanAllocatorOptions custom_options;
    GamesmanAllocatorOptionsSetDefaults(&custom_options);

    // Using a multiplier of 2 guarantees the alignment remains a power of 2
    // while acting as a multiple of the pointer size.
    custom_options.alignment = sizeof(void *) * 2;

    GamesmanAllocator *allocator = GamesmanAllocatorCreate(&custom_options);
    ASSERT_NE(allocator, nullptr);
    GamesmanAllocatorRelease(allocator);
}

// Verifies the boundary condition where the alignment is a power of 2, but
// strictly less than the pointer size.
TEST(GamesmanAllocatorCreateTest, InvalidAlignmentNotPointerMultiple) {
    GamesmanAllocatorOptions custom_options;
    GamesmanAllocatorOptionsSetDefaults(&custom_options);

    // 2 is a power of 2, but smaller than any standard pointer size (4 or 8
    // bytes), invalidating the creation request.
    custom_options.alignment = 2;

    GamesmanAllocator *allocator = GamesmanAllocatorCreate(&custom_options);
    EXPECT_EQ(allocator, nullptr);

    if (allocator != nullptr) {
        GamesmanAllocatorRelease(allocator);
    }
}

// Verifies the edge case where the alignment is a valid multiple of the pointer
// size, but fails the power of 2 requirement.
TEST(GamesmanAllocatorCreateTest, InvalidAlignmentNotPowerOfTwo) {
    GamesmanAllocatorOptions custom_options;
    GamesmanAllocatorOptionsSetDefaults(&custom_options);

    // A multiplier of 3 ensures the resulting value is a multiple of pointer
    // size but explicitly breaks the power of 2 rule.
    custom_options.alignment = sizeof(void *) * 3;

    GamesmanAllocator *allocator = GamesmanAllocatorCreate(&custom_options);
    EXPECT_EQ(allocator, nullptr);

    if (allocator != nullptr) {
        GamesmanAllocatorRelease(allocator);
    }
}

// Verifies that an allocator can be instantiated with a zero-byte pool size
// limit.
TEST(GamesmanAllocatorCreateTest, ZeroPoolSize) {
    GamesmanAllocatorOptions custom_options;
    GamesmanAllocatorOptionsSetDefaults(&custom_options);
    custom_options.pool_size = 0;

    GamesmanAllocator *allocator = GamesmanAllocatorCreate(&custom_options);
    ASSERT_NE(allocator, nullptr)
        << "GamesmanAllocatorCreate should create an allocator with zero pool "
           "size when requested.";

    EXPECT_EQ(GamesmanAllocatorGetRemainingPoolSize(allocator), 0);
    GamesmanAllocatorRelease(allocator);
}

// Verifies that passing a null pointer to Release safely acts as a no-op
// without crashing.
TEST(GamesmanAllocatorReleaseTest, ReleaseNullPointer) {
    // A segfault here would cause the test runner to fail immediately.
    GamesmanAllocatorRelease(nullptr);
    SUCCEED();
}

// Verifies that passing a null pointer to AddRef safely acts as a no-op and
// returns null.
TEST(GamesmanAllocatorAddRefTest, AddRefNullPointer) {
    EXPECT_EQ(GamesmanAllocatorAddRef(nullptr), nullptr);
}

// Verifies the basic reference counting lifecycle by incrementing and
// decrementing sequentially.
TEST(GamesmanAllocatorAddRefTest, SequentialReferenceCounting) {
    GamesmanAllocator *allocator = GamesmanAllocatorCreate(nullptr);
    ASSERT_NE(allocator, nullptr);

    GamesmanAllocator *returned_allocator = GamesmanAllocatorAddRef(allocator);

    // Ensure AddRef returns the exact same pointer instance.
    EXPECT_EQ(returned_allocator, allocator);

    // The first release should decrement the ref count back to 1.
    // If it incorrectly freed the memory here, the subsequent release would
    // trigger a double-free crash.
    GamesmanAllocatorRelease(allocator);
    GamesmanAllocatorRelease(allocator);
    SUCCEED();
}

// Verifies that an allocator remains fully functional after a partial release,
// proving that AddRef correctly prevented premature deallocation.
TEST(GamesmanAllocatorAddRefTest, RetainsValidityAfterPartialRelease) {
    GamesmanAllocator *allocator = GamesmanAllocatorCreate(nullptr);
    ASSERT_NE(allocator, nullptr);

    GamesmanAllocatorAddRef(allocator);

    // Decrements the internal reference count from 2 to 1.
    GamesmanAllocatorRelease(allocator);

    // If the allocator was improperly destroyed above, the following will fail
    // or cause a segfault.
    EXPECT_EQ(GamesmanAllocatorGetRemainingPoolSize(allocator), SIZE_MAX);
    void *allocated_memory = GamesmanAllocatorAllocate(allocator, 16);
    EXPECT_NE(allocated_memory, nullptr);
    GamesmanAllocatorDeallocate(allocator, allocated_memory);

    // Safely perform the final destruction.
    GamesmanAllocatorRelease(allocator);
}

// Verifies that passing a NULL allocator falls back to global allocation and
// returns valid memory.
TEST(GamesmanAllocatorAllocateTest, NullAllocatorFallsBackToGlobalAlloc) {
    void *ptr = GamesmanAllocatorAllocate(nullptr, 128);

    ASSERT_NE(ptr, nullptr);

    // The API contract dictates that memory allocated with a NULL allocator
    // must be deallocated with a NULL allocator.
    GamesmanAllocatorDeallocate(nullptr, ptr);
}

// Verifies that allocating exactly 0 bytes returns NULL.
TEST(GamesmanAllocatorAllocateTest, ZeroSizeReturnsNull) {
    GamesmanAllocator *allocator = GamesmanAllocatorCreate(nullptr);
    ASSERT_NE(allocator, nullptr);

    // Standard C malloc(0) behavior is implementation-defined (it may return
    // NULL or a unique, zero-sized pointer). This test enforces the strict API
    // contract that GamesmanAllocatorAllocate must deterministically return
    // NULL for 0-byte requests, normalizing the behavior across platforms.
    void *ptr = GamesmanAllocatorAllocate(allocator, 0);
    EXPECT_EQ(ptr, nullptr);

    GamesmanAllocatorRelease(allocator);
}

// Verifies that allocating a valid size within the pool limits returns a
// non-NULL pointer.
TEST(GamesmanAllocatorAllocateTest, ValidSizeReturnsNonNullPointer) {
    GamesmanAllocator *allocator = GamesmanAllocatorCreate(nullptr);
    ASSERT_NE(allocator, nullptr);

    constexpr size_t kAllocSize = 256;
    void *ptr = GamesmanAllocatorAllocate(allocator, kAllocSize);

    ASSERT_NE(ptr, nullptr);

    // Writing a payload pattern ensures the allocator didn't just return a
    // mapped virtual address that lacks backing physical pages. This write
    // forces a page fault (if uncommitted) and validates that the memory
    // is actually writable and usable by the application.
    std::memset(ptr, 0xAA, kAllocSize);

    GamesmanAllocatorDeallocate(allocator, ptr);
    GamesmanAllocatorRelease(allocator);
}

// Verifies that the memory address returned strictly adheres to the alignment
// specified during creation.
TEST(GamesmanAllocatorAllocateTest, AllocationRespectsCustomAlignment) {
    GamesmanAllocatorOptions options;
    GamesmanAllocatorOptionsSetDefaults(&options);

    // A 64-byte alignment typically mirrors cache-line sizes, which is a common
    // requirement for avoiding false sharing in high-performance computing
    // contexts.
    constexpr size_t kCustomAlignment = 64;
    options.alignment = kCustomAlignment;

    GamesmanAllocator *allocator = GamesmanAllocatorCreate(&options);
    ASSERT_NE(allocator, nullptr);

    void *ptr = GamesmanAllocatorAllocate(allocator, 128);
    ASSERT_NE(ptr, nullptr);

    // Casting the pointer to an integer type allows us to perform a modulo
    // operation. This mathematically enforces the strict alignment guarantee at
    // the ABI level, ensuring the backend allocator isn't silently dropping the
    // alignment constraint or miscalculating the padding requirement.
    auto address = reinterpret_cast<uintptr_t>(ptr);
    EXPECT_EQ(address % kCustomAlignment, 0u);

    GamesmanAllocatorDeallocate(allocator, ptr);
    GamesmanAllocatorRelease(allocator);
}

// Verifies that requesting more memory than the remaining pool size returns
// NULL.
TEST(GamesmanAllocatorAllocateTest, ExceedingPoolSizeReturnsNull) {
    GamesmanAllocatorOptions options;
    GamesmanAllocatorOptionsSetDefaults(&options);
    options.pool_size = 1024;

    GamesmanAllocator *allocator = GamesmanAllocatorCreate(&options);
    ASSERT_NE(allocator, nullptr);

    // Requesting memory strictly larger than the entire pool size guarantees
    // deterministic failure regardless of internal fragmentation or padding
    // heuristics.
    void *ptr = GamesmanAllocatorAllocate(allocator, 2048);
    EXPECT_EQ(ptr, nullptr);
    // Relying on another test to verify that the pool size is not modified on
    // failed allocations.

    GamesmanAllocatorRelease(allocator);
}

// Verifies that allocating from a zero-pool-size allocator (which was
// successfully created) returns NULL for any size > 0.
TEST(GamesmanAllocatorAllocateTest, AllocationFailsOnZeroPoolSizeAllocator) {
    GamesmanAllocatorOptions options;
    GamesmanAllocatorOptionsSetDefaults(&options);
    options.pool_size = 0;

    GamesmanAllocator *allocator = GamesmanAllocatorCreate(&options);
    ASSERT_NE(allocator, nullptr);

    // Even a 1-byte request must be rejected. This tests the extreme edge case
    // of resource availability constraints. It ensures that the allocator logic
    // doesn't rely on unsigned underflows (e.g., 0 - 1 = SIZE_MAX) when
    // deducting allocation sizes from the remaining pool size.
    void *ptr = GamesmanAllocatorAllocate(allocator, 1);
    EXPECT_EQ(ptr, nullptr);

    GamesmanAllocatorRelease(allocator);
}

// Verifies that passing a NULL pointer to Deallocate is a safe no-op.
TEST(GamesmanAllocatorDeallocateTest, NullPointerDoesNothing) {
    GamesmanAllocator *allocator = GamesmanAllocatorCreate(nullptr);
    ASSERT_NE(allocator, nullptr);

    GamesmanAllocatorDeallocate(allocator, nullptr);
    // If no segmentation fault or assertion failure occurs, the test naturally
    // succeeds.

    GamesmanAllocatorRelease(allocator);
    SUCCEED();
}

// Verifies that passing a NULL allocator gracefully falls back to global free
// (GamesmanFree).
TEST(GamesmanAllocatorDeallocateTest, NullAllocatorFallsBackToGlobalFree) {
    // First, we acquire a block of memory using the global fallback mechanism.
    void *ptr = GamesmanAllocatorAllocate(nullptr, 64);
    ASSERT_NE(ptr, nullptr);

    GamesmanAllocatorDeallocate(nullptr, ptr);
    SUCCEED();
}

// Verifies that returning memory back to the allocator allows previously
// failing allocations (due to exhaustion) to succeed again.
TEST(GamesmanAllocatorDeallocateTest, DeallocateAllowsSubsequentAllocations) {
    GamesmanAllocatorOptions options;
    GamesmanAllocatorOptionsSetDefaults(&options);
    options.pool_size = 1024;

    GamesmanAllocator *allocator = GamesmanAllocatorCreate(&options);
    ASSERT_NE(allocator, nullptr);

    // We allocate a conservatively large chunk (more than half the pool) to
    // push the allocator into a state where a subsequent identical request must
    // fail. This avoids guessing exact padding/overhead geometries while still
    // guaranteeing exhaustion.
    constexpr size_t kChunkSize = 600;
    void *ptr1 = GamesmanAllocatorAllocate(allocator, kChunkSize);
    ASSERT_NE(ptr1, nullptr);

    // Confirming the pool is sufficiently exhausted.
    void *ptr2 = GamesmanAllocatorAllocate(allocator, kChunkSize);
    EXPECT_EQ(ptr2, nullptr);

    // Reclaiming the initial chunk verifies that the allocator properly
    // recycles the freed blocks rather than operating as a simple monotonic
    // bump allocator that lacks a reusability mechanism.
    GamesmanAllocatorDeallocate(allocator, ptr1);

    // The previously failing allocation geometry must now succeed.
    void *ptr3 = GamesmanAllocatorAllocate(allocator, kChunkSize);
    EXPECT_NE(ptr3, nullptr);

    GamesmanAllocatorDeallocate(allocator, ptr3);
    GamesmanAllocatorRelease(allocator);
}

// Verifies that requesting an overwhelmingly large allocation size correctly
// returns NULL instead of causing an internal integer overflow.
TEST(GamesmanAllocatorAllocateTest, ExtremelyLargeSizeFailsGracefully) {
    GamesmanAllocator *allocator = GamesmanAllocatorCreate(nullptr);
    ASSERT_NE(allocator, nullptr);

    // We request SIZE_MAX, the maximum representable value for size_t.
    // Custom allocators frequently add padding for alignment or internal
    // headers (e.g., size + sizeof(BlockHeader)). If the allocator lacks
    // safe-math checks, this addition will overflow, wrapping around to a very
    // small integer. The allocator might then erroneously succeed in allocating
    // a tiny block.
    void *ptr = GamesmanAllocatorAllocate(allocator, SIZE_MAX);
    EXPECT_EQ(ptr, nullptr);

    // Testing an additional near-max value to catch implementations that might
    // only hardcode a check against exactly SIZE_MAX.
    void *ptr_near_max = GamesmanAllocatorAllocate(allocator, SIZE_MAX - 16);
    EXPECT_EQ(ptr_near_max, nullptr);

    GamesmanAllocatorRelease(allocator);
}

// Verifies that the initial pool size exactly matches the size requested in the
// options.
TEST(GamesmanAllocatorPoolSizeTest, InitialPoolSizeMatchesOptions) {
    GamesmanAllocatorOptions options;
    GamesmanAllocatorOptionsSetDefaults(&options);

    // Choosing an arbitrary, bounded pool size distinct from SIZE_MAX
    // ensures the underlying storage logic correctly stores and reports
    // custom boundaries rather than falling back to defaults.
    constexpr size_t kTargetPoolSize = 4096;
    options.pool_size = kTargetPoolSize;

    GamesmanAllocator *allocator = GamesmanAllocatorCreate(&options);
    ASSERT_NE(allocator, nullptr);
    EXPECT_EQ(GamesmanAllocatorGetRemainingPoolSize(allocator),
              kTargetPoolSize);

    GamesmanAllocatorRelease(allocator);
}

// Verifies that successfully allocating memory decreases the remaining pool
// size appropriately.
TEST(GamesmanAllocatorPoolSizeTest, PoolSizeDecreasesAfterAllocation) {
    GamesmanAllocatorOptions options;
    GamesmanAllocatorOptionsSetDefaults(&options);

    constexpr size_t kInitialSize = 2048;
    options.pool_size = kInitialSize;

    GamesmanAllocator *allocator = GamesmanAllocatorCreate(&options);
    ASSERT_NE(allocator, nullptr);

    constexpr size_t kAllocSize = 256;
    void *ptr = GamesmanAllocatorAllocate(allocator, kAllocSize);
    ASSERT_NE(ptr, nullptr);

    // The remaining pool size must drop. Because internal metadata or alignment
    // padding might introduce overhead beyond the raw request size, we verify
    // that the reduction is at least equal to the requested size rather than
    // assuming a strict equality which could break on opaque block headers.
    size_t remaining = GamesmanAllocatorGetRemainingPoolSize(allocator);
    EXPECT_LE(remaining, kInitialSize - kAllocSize);

    GamesmanAllocatorDeallocate(allocator, ptr);
    GamesmanAllocatorRelease(allocator);
}

// Verifies that a failed allocation (e.g., requested size > pool size) does not
// alter the remaining pool size.
TEST(GamesmanAllocatorPoolSizeTest, FailedAllocationDoesNotChangePoolSize) {
    GamesmanAllocatorOptions options;
    GamesmanAllocatorOptionsSetDefaults(&options);

    constexpr size_t kInitialSize = 1024;
    options.pool_size = kInitialSize;

    GamesmanAllocator *allocator = GamesmanAllocatorCreate(&options);
    ASSERT_NE(allocator, nullptr);

    // We explicitly trigger an out-of-memory (OOM) scenario for this pool.
    void *ptr = GamesmanAllocatorAllocate(allocator, 2048);
    EXPECT_EQ(ptr, nullptr);

    // This validates the transactionality of the allocation routine.
    EXPECT_EQ(GamesmanAllocatorGetRemainingPoolSize(allocator), kInitialSize)
        << "Remaining pool size should not change after a failed allocation.";

    GamesmanAllocatorRelease(allocator);
}

// Verifies that asking for 0 bytes (which returns NULL) does not alter the
// remaining pool size.
TEST(GamesmanAllocatorPoolSizeTest, ZeroByteAllocationDoesNotChangePoolSize) {
    GamesmanAllocatorOptions options;
    GamesmanAllocatorOptionsSetDefaults(&options);

    constexpr size_t kInitialSize = 1024;
    options.pool_size = kInitialSize;

    GamesmanAllocator *allocator = GamesmanAllocatorCreate(&options);
    ASSERT_NE(allocator, nullptr);

    // Requesting 0 bytes is a guaranteed NULL return per the API contract.
    void *ptr = GamesmanAllocatorAllocate(allocator, 0);
    EXPECT_EQ(ptr, nullptr);
    EXPECT_EQ(GamesmanAllocatorGetRemainingPoolSize(allocator), kInitialSize);

    GamesmanAllocatorRelease(allocator);
}

// Verifies that deallocating a valid pointer increases the pool size back by
// the allocated amount.
TEST(GamesmanAllocatorPoolSizeTest, PoolSizeIncreasesAfterDeallocation) {
    GamesmanAllocatorOptions options;
    GamesmanAllocatorOptionsSetDefaults(&options);

    constexpr size_t kInitialSize = 2048;
    options.pool_size = kInitialSize;

    GamesmanAllocator *allocator = GamesmanAllocatorCreate(&options);
    ASSERT_NE(allocator, nullptr);

    void *ptr = GamesmanAllocatorAllocate(allocator, 512);
    ASSERT_NE(ptr, nullptr);

    // Capture the state mid-lifecycle to prove the pool was actually reduced.
    size_t size_after_alloc = GamesmanAllocatorGetRemainingPoolSize(allocator);
    EXPECT_LT(size_after_alloc, kInitialSize);

    GamesmanAllocatorDeallocate(allocator, ptr);

    EXPECT_EQ(GamesmanAllocatorGetRemainingPoolSize(allocator), kInitialSize)
        << "Allocator pool size should restore to initial size after all "
           "allocations are deallocated.";

    GamesmanAllocatorRelease(allocator);
}

// =========================== Memory Allocation API ===========================

// Verifies that requesting a standard, valid memory size returns a usable,
// non-NULL pointer.
TEST(GamesmanMallocTest, ValidSizeReturnsNonNull) {
    constexpr size_t kAllocSize = 128;
    void *ptr = GamesmanMalloc(kAllocSize);
    ASSERT_NE(ptr, nullptr);

    // We write a payload to the allocated memory to force the OS to physically
    // back the virtual pages. If the allocator mistakenly returned a pointer to
    // a read-only page or unmapped memory, this memset will trigger a SIGSEGV.
    std::memset(ptr, 0xAA, kAllocSize);
    GamesmanFree(ptr);
    SUCCEED();
}

// Verifies the behavior when allocating 0 bytes (either returning NULL or a
// unique safe pointer, depending on exact implementation).
TEST(GamesmanMallocTest, ZeroSizeHandlesGracefully) {
    void *ptr = GamesmanMalloc(0);
    GamesmanFree(ptr);
    SUCCEED();
}

// Verifies that requesting an overwhelmingly large amount of memory (e.g.,
// SIZE_MAX) safely returns NULL instead of crashing.
TEST(GamesmanMallocTest, OutOfMemoryReturnsNull) {
    // Requesting SIZE_MAX guarantees a legitimate OOM scenario. If the
    // allocator tries to add internal padding or headers to this size before
    // checking for overflow, it will wrap around to a small number, falsely
    // succeeding. This test ensures the allocator calculates bounds safely
    // before attempting kernel requests.
    void *ptr = GamesmanMalloc(SIZE_MAX);
    EXPECT_EQ(ptr, nullptr);
}

// Verifies that the returned memory address respects standard alignment (and
// GM_CACHE_LINE_SIZE if built with MT enabled).
TEST(GamesmanMallocTest, ReturnedMemoryIsProperlyAligned) {
    // We allocate a small block, as alignment guarantees must hold true
    // regardless of the requested payload size.
    void *ptr = GamesmanMalloc(16);
    ASSERT_NE(ptr, nullptr);

    auto address = reinterpret_cast<uintptr_t>(ptr);

#ifdef _OPENMP
    // In multithreaded builds (detected via OpenMP flag), the API strictly
    // guarantees cache-line alignment to prevent false sharing between threads
    // operating on adjacent memory blocks.
    EXPECT_EQ(address % GM_CACHE_LINE_SIZE, 0u);
#else
    // C11/C++17 guarantees that any address returned by malloc satisfies the
    // fundamental alignment requirement formally represented by the
    // `max_align_t` type.
    EXPECT_EQ(address % alignof(max_align_t), 0u);
#endif  // _OPENMP

    GamesmanFree(ptr);
}

// Verifies that providing valid element counts and sizes returns a non-NULL
// pointer.
TEST(GamesmanCallocWholeTest, ValidInputsReturnNonNull) {
    constexpr size_t kNumElements = 64;
    constexpr size_t kElementSize = 32;

    void *ptr = GamesmanCallocWhole(kNumElements, kElementSize);
    ASSERT_NE(ptr, nullptr);

    // We write to the boundaries of the allocated block to prove that the
    // entire requested span (nmemb * size) is safely backed by physical memory
    // and accessible, ruling out partial allocations or internal alignment bugs
    // that truncate the span.
    auto *bytes = static_cast<volatile uint8_t *>(ptr);
    bytes[0] = 0xFF;
    bytes[(kNumElements * kElementSize) - 1] = 0xFF;

    GamesmanFree(ptr);
}

// Verifies that the allocated memory block is entirely initialized to zero.
TEST(GamesmanCallocWholeTest, MemoryIsZeroInitialized) {
    constexpr size_t kNumElements = 128;
    constexpr size_t kElementSize = 16;
    constexpr size_t kTotalSize = kNumElements * kElementSize;

    void *ptr = GamesmanCallocWhole(kNumElements, kElementSize);
    ASSERT_NE(ptr, nullptr);

    // The primary contract of calloc-family functions over standard malloc is
    // the zeroing of memory. This prevents non-deterministic behavior and
    // information disclosure vulnerabilities (e.g., leaking stale heap data).
    // We use memcmp against a known-zeroed buffer for a fast, optimized block
    // comparison.
    std::vector<uint8_t> zero_buffer(kTotalSize, 0);
    EXPECT_EQ(std::memcmp(ptr, zero_buffer.data(), kTotalSize), 0);

    GamesmanFree(ptr);
}

// Verifies behavior when the number of elements requested is 0.
TEST(GamesmanCallocWholeTest, ZeroElementsHandlesGracefully) {
    constexpr size_t kElementSize = 32;
    void *ptr = GamesmanCallocWhole(0, kElementSize);
    GamesmanFree(ptr);
    SUCCEED();
}

// Verifies behavior when the size of each element requested is 0.
TEST(GamesmanCallocWholeTest, ZeroSizeHandlesGracefully) {
    constexpr size_t kNumElements = 32;
    void *ptr = GamesmanCallocWhole(kNumElements, 0);
    GamesmanFree(ptr);
    SUCCEED();
}

// Verifies that passing values that cause an integer overflow when multiplied
// (e.g., nmemb = SIZE_MAX, size = 2) returns NULL.
TEST(GamesmanCallocWholeTest, IntegerOverflowReturnsNull) {
    // A classic heap vulnerability occurs when `nmemb * size` overflows a
    // size_t, wrapping around to a small integer. If the allocator fails to
    // explicitly check for this multiplication overflow (e.g., via `SIZE_MAX /
    // nmemb < size`), it allocates a tiny buffer, but the caller assumes it has
    // a massive array, leading to immediate heap corruption upon writing. We
    // strictly enforce that the API detects the overflow and returns NULL.
    void *ptr = GamesmanCallocWhole(SIZE_MAX, 2);
    EXPECT_EQ(ptr, nullptr);
}

// Verifies that a total size just shy of SIZE_MAX correctly fails without
// internal wrap-around.
TEST(GamesmanCallocWholeTest, NearOverflowSizeWithPaddingReturnsNull) {
    // We set up a scenario where the raw multiplication of nmemb and size
    // is mathematically valid and strictly less than SIZE_MAX (meaning standard
    // overflow checks on `nmemb * size` will pass).
    //
    // However, the requested size is close enough to SIZE_MAX that when the
    // allocator attempts to add internal alignment padding or hidden metadata
    // headers (e.g., `actual_size = (nmemb * size) + metadata_size`), the
    // addition wraps around to a tiny number.
    //
    // A robust implementation must foresee this secondary internal overflow
    // and safely return NULL, rather than falsely succeeding and handing the
    // caller a dangerously small buffer.

    constexpr size_t kElementSize = 1;

    // 16 bytes shy of the absolute maximum. Most custom allocators attach at
    // least 16 to 32 bytes of hidden metadata (block headers, size tags) or
    // round up to the nearest 16-byte boundary, guaranteeing an internal
    // addition overflow.
    constexpr size_t kNearOverflowCount = SIZE_MAX - 16;

    void *ptr = GamesmanCallocWhole(kNearOverflowCount, kElementSize);

    EXPECT_EQ(ptr, nullptr);
}

// Verifies that requesting a massive (but non-overflowing) total size safely
// returns NULL.
TEST(GamesmanCallocWholeTest, OutOfMemoryReturnsNull) {
    // By requesting exactly half of the maximum representable memory space with
    // an element size of 1, we avoid multiplication overflow but guarantee a
    // legitimate Out-Of-Memory (OOM) rejection from the OS/virtual memory
    // subsystem. This verifies that the allocator propagates the OOM failure as
    // a NULL pointer rather than crashing or asserting internally.
    constexpr size_t kMassiveElements = SIZE_MAX / 2;
    void *ptr = GamesmanCallocWhole(kMassiveElements, 1);
    EXPECT_EQ(ptr, nullptr);
}

// Verifies that requesting memory with a valid alignment returns a non-NULL
// pointer matching that exact boundary.
TEST(GamesmanAlignedAllocTest, ValidAlignmentReturnsCorrectlyAlignedPointer) {
    // We choose an alignment of 64 bytes. This guarantees compliance with the
    // API's requirement of being a power of two and a multiple of
    // sizeof(void*), while also matching the typical cache line size on
    // x86_64/ARM64 architectures.
    constexpr size_t kAlignment = 64;
    constexpr size_t kAllocSize = 128;

    void *ptr = GamesmanAlignedAlloc(kAlignment, kAllocSize);
    ASSERT_NE(ptr, nullptr);

    // Bitwise verify that the least significant bits of the returned address
    // are zero, conforming to the requested power-of-two alignment boundary.
    auto address = reinterpret_cast<uintptr_t>(ptr);
    EXPECT_EQ(address % kAlignment, 0u);

    // Fault in the pages by writing to the memory, confirming that the
    // allocator didn't return an unmapped virtual address while doing alignment
    // pointer arithmetic.
    std::memset(ptr, 0xBB, kAllocSize);

    GamesmanFree(ptr);
}

#ifdef _OPENMP
// Verifies that a valid alignment smaller than the cache line is upgraded when
// compiled with multithreading.
TEST(GamesmanAlignedAllocTest, SubCacheLineAlignmentUpgradesToCacheLine) {
    // Requesting a valid, small power-of-two alignment (e.g., 8 or 16 bytes).
    // While valid from a C-standard perspective, the library specification
    // dictates that all allocations must be at least cache-line aligned to
    // prevent "false sharing" in highly concurrent, multi-threaded memory
    // access patterns.

    constexpr size_t kAllocSize = 256;
    constexpr size_t kRequestedAlignment = 16;

    // Ensure the test environment is configured such that the requested
    // alignment is actually smaller than the cache line, otherwise the test is
    // invalid.
    ASSERT_LT(kRequestedAlignment, GM_CACHE_LINE_SIZE)
        << "Test requires GM_CACHE_LINE_SIZE to be greater than 16.";

    void *ptr = GamesmanAlignedAlloc(kRequestedAlignment, kAllocSize);
    ASSERT_NE(ptr, nullptr);

    auto address = reinterpret_cast<uintptr_t>(ptr);

    // Even though we only asked for 16-byte alignment, the pointer must satisfy
    // the stricter GM_CACHE_LINE_SIZE boundary due to the max() calculation.
    EXPECT_EQ(address % GM_CACHE_LINE_SIZE, 0u)
        << "Pointer was not promoted to GM_CACHE_LINE_SIZE alignment.";

    // By mathematical definition, if it is aligned to the cache line (e.g.,
    // 64), it is also automatically aligned to the requested sub-alignment
    // (e.g., 16).
    EXPECT_EQ(address % kRequestedAlignment, 0u);

    GamesmanFree(ptr);
}
#endif  // _OPENMP

// Verifies that providing an alignment that is not a power of 2 (e.g., 24)
// returns NULL.
TEST(GamesmanAlignedAllocTest, AlignmentNotPowerOfTwoReturnsNull) {
    // The C11 aligned_alloc and POSIX posix_memalign standards strictly require
    // the alignment to be a power of two. This is because hardware caches and
    // MMUs map memory in power-of-two blocks, making bitwise masking operations
    // (e.g., `x & (x - 1)`) for alignment calculations fast and predictable. We
    // use 24 because it is a multiple of 8 (pointer size on 64-bit systems) but
    // breaks the power-of-two rule.
    constexpr size_t kInvalidPositiveAlignment = 24;
    constexpr size_t kAllocSize = 128;

    void *ptr = GamesmanAlignedAlloc(kInvalidPositiveAlignment, kAllocSize);
    EXPECT_EQ(ptr, nullptr);

    ptr = GamesmanAlignedAlloc(0, kAllocSize);
    EXPECT_EQ(ptr, nullptr)
        << "0 is not a power of 2, hence the allocation should return NULL.";
}

// Verifies that providing an alignment smaller than the pointer size (e.g., 2
// on a 64-bit system) returns NULL.
TEST(GamesmanAlignedAllocTest, AlignmentNotMultipleOfPointerSizeReturnsNull) {
    // To safely store intrusive heap metadata or ensure that fundamental types
    // (like pointers themselves) can be stored within the allocated block
    // without tearing or bus errors, the alignment must be at least
    // sizeof(void*). While 2 is a power of two, it violates the pointer-size
    // multiple constraint on modern 32-bit and 64-bit ISAs.
    constexpr size_t kSmallAlignment = 2;
    constexpr size_t kAllocSize = 128;

    void *ptr = GamesmanAlignedAlloc(kSmallAlignment, kAllocSize);
    EXPECT_EQ(ptr, nullptr);
}

// Verifies behavior when the requested allocation size is 0 bytes.
TEST(GamesmanAlignedAllocTest, ZeroSizeHandlesGracefully) {
    constexpr size_t kAlignment = 64;

    // Under C11 (aligned_alloc) and POSIX (posix_memalign), behavior for a
    // 0-byte request is implementation-defined. The allocator may return a
    // strict NULL pointer or a unique zero-sized sentinel. This test ensures
    // that the allocator's internal masking and alignment-shifting logic does
    // not fault on a zero size and that the resulting pointer successfully
    // passes through the free() pipeline.
    void *ptr = GamesmanAlignedAlloc(kAlignment, 0);
    GamesmanFree(ptr);
    SUCCEED();
}

// Verifies that requesting a massively large block of aligned memory safely
// returns NULL.
TEST(GamesmanAlignedAllocTest, OutOfMemoryReturnsNull) {
    constexpr size_t kAlignment = 4096;  // Standard page size alignment

    // Requesting half of the addressable virtual memory space guarantees a
    // legitimate Out-Of-Memory failure from the kernel's virtual memory
    // subsystem. We verify that GamesmanAlignedAlloc propagates this condition
    // as a graceful NULL return rather than crashing or triggering an abort
    // inside the allocator.
    constexpr size_t kMassiveSize = SIZE_MAX / 2;

    void *ptr = GamesmanAlignedAlloc(kAlignment, kMassiveSize);
    EXPECT_EQ(ptr, nullptr);
}

// Verifies that internal padding calculations do not cause an integer overflow.
TEST(GamesmanAlignedAllocTest, SizeOverflowDueToAlignmentPaddingReturnsNull) {
    constexpr size_t kAlignment = 4096;

    // Custom aligned allocators typically request extra memory from the
    // underlying system to guarantee they can shift the returned pointer to the
    // correct boundary. The internal formula usually looks like: `actual_size =
    // size + alignment - 1 + metadata`. If `size` is extremely close to
    // SIZE_MAX (e.g., SIZE_MAX - 10), adding `alignment` causes an arithmetic
    // wrap-around. If the allocator fails to check for this overflow, it will
    // allocate a tiny chunk of memory but the caller will assume they have
    // near-infinite space, leading to immediate and catastrophic heap
    // corruption.
    constexpr size_t kOverflowingSize = SIZE_MAX - 16;

    void *ptr = GamesmanAlignedAlloc(kAlignment, kOverflowingSize);
    EXPECT_EQ(ptr, nullptr);
}

// Verifies that passing a NULL pointer is a safe, non-crashing operation.
TEST(GamesmanFreeTest, NullPointerIsNoOp) {
    void *ptr = nullptr;
    GamesmanFree(ptr);
    SUCCEED();
}

// Verifies that a standard allocated block can be freed without triggering
// internal asserts.
TEST(GamesmanFreeTest, ValidPointerDeallocation) {
    // While we cannot easily inspect the internal free-list state from a
    // black-box unit test, we can verify that passing a legitimately acquired
    // pointer back to the deallocator does not trigger metadata corruption
    // checks, segfaults, or alignment panics.
    constexpr size_t kAllocSize = 256;
    void *ptr = GamesmanMalloc(kAllocSize);
    ASSERT_NE(ptr, nullptr);

    // Touching the memory ensures it's mapped into the page table.
    auto *bytes = static_cast<volatile uint8_t *>(ptr);
    bytes[0] = 0xAA;
    bytes[kAllocSize - 1] = 0xBB;

    GamesmanFree(ptr);
    SUCCEED();
}

// Verifies that the system reports a strictly positive amount of physical
// memory.
TEST(GetPhysicalMemoryTest, ReturnsNonZeroValue) {
    size_t total_memory = GetPhysicalMemory();
    EXPECT_GT(total_memory, 0u);
}

// Verifies that sequential calls yield the same hardware limit.
TEST(GetPhysicalMemoryTest, ReturnsConsistentValue) {
    size_t memory_first_call = GetPhysicalMemory();
    size_t memory_second_call = GetPhysicalMemory();
    EXPECT_EQ(memory_first_call, memory_second_call);
}

// Verifies the reported memory meets a reasonable minimum threshold for a
// modern system.
TEST(GetPhysicalMemoryTest, PlausibleMinimumThreshold) {
    // A sanity check to ensure the return value isn't returning a truncated
    // bitfield or an unscaled page count. Any modern environment running these
    // tests (even a micro-container) will have at least 16 Megabytes of RAM.
    // If we return less, the underlying OS query conversion logic is flawed.
    constexpr size_t kAbsoluteMinimumMemory = 16 * 1024 * 1024;  // 16 MB
    size_t total_memory = GetPhysicalMemory();

    EXPECT_GE(total_memory, kAbsoluteMinimumMemory);
}

// Verifies that a reasonable allocation request succeeds and returns a usable
// pointer.
TEST(SafeMallocTest, ValidAllocationSucceeds) {
    constexpr size_t kAllocSize = 128;

    // Unlike standard malloc tests where we ASSERT_NE(ptr, nullptr), the
    // contract of SafeMalloc guarantees that if it returns at all, the pointer
    // is valid.
    void *ptr = SafeMalloc(kAllocSize);

    // Touch memory to ensure it is mapped.
    std::memset(ptr, 0xAA, kAllocSize);

    GamesmanFree(ptr);
    SUCCEED();
}

// Verifies that exhausting memory forcibly terminates the application.
TEST(SafeMallocDeathTest, OutOfMemoryTerminatesProcess) {
    // We request an impossibly large block of memory.
    // EXPECT_DEATH takes a statement and a regex to match against stderr.
    // We use ".*" to match any crash message (e.g., abort, assertion failure),
    // as the exact stderr output often varies by platform and logging
    // implementation.
    constexpr size_t kMassiveSize = SIZE_MAX / 2;

    EXPECT_DEATH(
        {
            void *ptr = SafeMalloc(kMassiveSize);
            // The test should never reach this point.
            (void)ptr;
        },
        ".*");
}

// Verifies that a valid allocation succeeds and is strictly zero-initialized.
TEST(SafeCallocTest, ValidAllocationSucceedsAndIsZeroed) {
    constexpr size_t kElementCount = 16;
    constexpr size_t kElementSize = 8;
    constexpr size_t kTotalBytes = kElementCount * kElementSize;

    void *ptr = SafeCalloc(kElementCount, kElementSize);

    // Verify the zero-initialization contract. This is critical for security
    // to prevent leaking sensitive data from recycled heap pages.
    auto *bytes = static_cast<uint8_t *>(ptr);
    for (size_t i = 0; i < kTotalBytes; ++i) {
        EXPECT_EQ(bytes[i], 0x00);
    }

    GamesmanFree(ptr);
}

// Verifies that exhausting memory via a massive element count terminates the
// process.
TEST(SafeCallocDeathTest, OutOfMemoryTerminatesProcess) {
    constexpr size_t kMassiveCount = SIZE_MAX / 4;
    constexpr size_t kElementSize =
        2;  // Keeps total size below SIZE_MAX to avoid math overflow

    EXPECT_DEATH(
        {
            void *ptr = SafeCalloc(kMassiveCount, kElementSize);
            (void)ptr;
        },
        ".*");
}

// Verifies that an integer overflow in the size calculation triggers
// termination.
TEST(SafeCallocDeathTest, IntegerOverflowTerminatesProcess) {
    // A classic vulnerability in naive calloc implementations is failing to
    // check if `count * size` wraps around zero. If it wraps, the allocator
    // requests a tiny block, but the caller iterates over a massive boundary,
    // leading to catastrophic heap overwrites. SafeCalloc must detect this and
    // abort.
    constexpr size_t kCount = SIZE_MAX / 2 + 1;
    constexpr size_t kSize = 4;
    // kCount * kSize > SIZE_MAX (Arithmetic Overflow)

    EXPECT_DEATH(
        {
            void *ptr = SafeCalloc(kCount, kSize);
            (void)ptr;
        },
        ".*");
}
