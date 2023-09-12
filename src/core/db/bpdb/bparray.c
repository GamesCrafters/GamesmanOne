#include "core/db/bpdb/bparray.h"

#include <assert.h>    // assert
#include <inttypes.h>  // PRIu64
#include <stddef.h>    // NULL
#include <stdint.h>    // int8_t, uint8_t, int64_t, uint64_t
#include <stdio.h>     // fprintf, stderr
#include <stdlib.h>    // calloc, free
#include <string.h>    // memset

#include "core/misc.h"

// Include and use OpenMP if the _OPENMP flag is set.
#ifdef _OPENMP
#include <omp.h>
#define PRAGMA(X) _Pragma(#X)
#define PRAGMA_OMP_CRITICAL(name) PRAGMA(omp critical(name))
#define PRAGMA_OMP_PARALLEL_FOR PRAGMA(omp parallel for)

// Otherwise, the following macros do nothing.
#else
#define PRAGMA
#define PRAGMA_OMP_CRITICAL(name)
#define PRAGMA_OMP_PARALLEL_FOR
#endif

static const int kDefaultBitsPerEntry = 1;

/**
 * @brief Maximum number of bits per array entry.
 * @details Currently set to 32 to simplify the implementation of segment
 * reading and writing. A segment is currently defined as 8 consecutive bytes
 * containing all the bits of an entry. Whereas an entry of length at most 32
 * must lie within a single segment, an entry longer than 32 bits may span more
 * than 1 segment, making it impossible to use a single uint64_t to access.
 */
static const int kMaxBitsPerEntry = 32;

static int BpArrayInitHelper(BpArray *array, int64_t size, int bits_per_entry);

static int64_t GetStreamLength(int64_t num_entries, int bits_per_entry);

static uint64_t GetSegment(const BpArray *array, int64_t i);
static void SetSegment(const BpArray *array, int64_t i, uint64_t segment,
                       int num_bytes);
static void SetSegmentTs(const BpArray *array, int64_t i, uint64_t segment);
static int64_t GetBitOffset(int64_t i, int bits_per_entry);
static int64_t GetLocalBitOffset(int64_t i, int bits_per_entry);
static int64_t GetByteOffset(int64_t i, int bits_per_entry);
static uint64_t GetEntryMask(int bits_per_entry, int local_bit_offset);

static int BpArrayExpandIfNeeded(BpArray *array, uint64_t entry);
static int BpArrayExpand(BpArray *array, int new_bits_per_entry);
static int BitsNeededForEntry(uint64_t entry);

// -----------------------------------------------------------------------------

int BpArrayInit(BpArray *array, int64_t size) {
    return BpArrayInitHelper(array, size, kDefaultBitsPerEntry);
}

void BpArrayDestroy(BpArray *array) {
    free(array->stream);
    memset(array, 0, sizeof(*array));
}

// Returns the entry at index I.
uint64_t BpArrayAt(const BpArray *array, int64_t i) {
    int bits_per_entry = array->meta.bits_per_entry;
    int local_bit_offset = GetLocalBitOffset(i, bits_per_entry);
    uint64_t mask = GetEntryMask(bits_per_entry, local_bit_offset);
    uint64_t segment = GetSegment(array, i);

    return (segment & mask) >> local_bit_offset;
}

// Sets the I-th entry to ENTRY. Assumes ENTRY is strictly less than
// ARRAY.num_values.
void BpArraySet(BpArray *array, int64_t i, uint64_t entry) {
    int bits_per_entry = array->meta.bits_per_entry;
    int local_bit_offset = GetLocalBitOffset(i, bits_per_entry);
    int local_bit_end = local_bit_offset + bits_per_entry;
    int num_bytes = RoundUpDivide(local_bit_end, kBitsPerByte);
    uint64_t mask = GetEntryMask(bits_per_entry, local_bit_offset);

    // Get segment containing the old entry.
    uint64_t segment = GetSegment(array, i);
    segment &= ~mask;                          // Zero out old entry.
    segment |= entry << local_bit_offset;      // Put new entry.
    SetSegment(array, i, segment, num_bytes);  // Put segment back.
}

// Thread-safe version of BpArraySet(). Expands ARRAY to have use enough bits
// per entry to store ENTRY.
int BpArraySetTs(BpArray *array, int64_t i, uint64_t entry) {
    int error;
    PRAGMA_OMP_CRITICAL(BpArraySetTs) {
        error = BpArrayExpandIfNeeded(array, entry);

        int bits_per_entry = array->meta.bits_per_entry;
        int local_bit_offset = GetLocalBitOffset(i, bits_per_entry);
        uint64_t mask = GetEntryMask(bits_per_entry, local_bit_offset);

        // Get segment containing the old entry.
        uint64_t segment = GetSegment(array, i);
        segment &= ~mask;                      // Zero out old entry.
        segment |= entry << local_bit_offset;  // Put new entry.
        SetSegmentTs(array, i, segment);       // Put segment back.
    }
    return error;
}

// -----------------------------------------------------------------------------

static int BpArrayInitHelper(BpArray *array, int64_t size, int bits_per_entry) {
    array->max_value = 0;
    array->meta.bits_per_entry = bits_per_entry;
    array->meta.num_entries = size;
    array->meta.stream_length = GetStreamLength(size, bits_per_entry);
    array->stream =
        (uint8_t *)calloc(array->meta.stream_length, sizeof(uint8_t));
    if (array->stream == NULL) {
        fprintf(stderr, "BpArrayInit: failed to calloc array\n");
        memset(array, 0, sizeof(*array));
        return 1;
    }
    return 0;
}

static int64_t GetStreamLength(int64_t num_entries, int bits_per_entry) {
    // 8 additional bytes are allocated so that it is always safe to read
    // 8 bytes for an entry.
    return RoundUpDivide(num_entries * bits_per_entry, kBitsPerByte) +
           sizeof(uint64_t);
}

static uint64_t GetSegment(const BpArray *array, int64_t i) {
    int64_t byte_offset = GetByteOffset(i, array->meta.bits_per_entry);
    const uint8_t *address = array->stream + byte_offset;
    return *((uint64_t *)address);
}

static void SetSegment(const BpArray *array, int64_t i, uint64_t segment,
                       int num_bytes) {
    int64_t byte_offset = GetByteOffset(i, array->meta.bits_per_entry);
    uint8_t *address = array->stream + byte_offset;
    memcpy(address, &segment, num_bytes);
}

static void SetSegmentTs(const BpArray *array, int64_t i, uint64_t segment) {
    int64_t byte_offset = GetByteOffset(i, array->meta.bits_per_entry);
    uint8_t *address = array->stream + byte_offset;
    *((uint64_t *)address) = segment;
}

static int64_t GetBitOffset(int64_t i, int bits_per_entry) {
    return i * bits_per_entry;
}

static int64_t GetLocalBitOffset(int64_t i, int bits_per_entry) {
    return GetBitOffset(i, bits_per_entry) % kBitsPerByte;
}

static int64_t GetByteOffset(int64_t i, int bits_per_entry) {
    int64_t bit_offset = GetBitOffset(i, bits_per_entry);
    return bit_offset / kBitsPerByte;
}

static uint64_t GetEntryMask(int bits_per_entry, int local_bit_offset) {
    return (((uint64_t)1 << bits_per_entry) - 1) << local_bit_offset;
}

static int BpArrayExpandIfNeeded(BpArray *array, uint64_t entry) {
    if (array->max_value >= entry) return 0;

    int new_bits_per_entry = BitsNeededForEntry(entry);
    if (new_bits_per_entry < 0) {
        fprintf(stderr,
                "BpArrayExpandIfNeeded: new entry %" PRIu64
                " cannot be represented by the longest number of bits allowed "
                "(%d) and therefore cannot be stored\n",
                entry, kMaxBitsPerEntry);
        return -1;
    }

    // If ENTRY can be stored without expanding, update max value and return.
    if (array->meta.bits_per_entry >= new_bits_per_entry) {
        array->max_value = entry;
        return 0;
    }

    // Otherwise, we need to expand the array to accomodate the new entry.
    int error = BpArrayExpand(array, entry);
    if (error != 0) return error;

    // Update max value only on success.
    array->max_value = entry;
    return 0;
}

static int BpArrayExpand(BpArray *array, int new_bits_per_entry) {
    BpArray new_array;
    int64_t size = array->meta.num_entries;
    int error = BpArrayInitHelper(&new_array, size, new_bits_per_entry);
    if (error != 0) {
        fprintf(stderr, "BpArrayExpand: failed to allocate new array\n");
        return error;
    }

    // PRAGMA_OMP_PARALLEL_FOR  <- is this nested parallelism safe?
    for (int64_t i = 0; i < size; ++i) {
        uint64_t entry = BpArrayAt(array, i);
        BpArraySet(&new_array, i, entry);  // if parallel, change to Ts version.
    }

    BpArrayDestroy(array);
    memcpy(array, &new_array, sizeof(*array));
    return 0;
}

static int BitsNeededForEntry(uint64_t entry) {
    for (int i = 1; i <= kMaxBitsPerEntry; ++i) {
        if (entry < ((uint64_t)1) << i) {
            return i;
        }
    }
    return -1;
}

#undef PRAGMA
#undef PRAGMA_OMP_CRITICAL
#undef PRAGMA_OMP_PARALLEL_FOR
