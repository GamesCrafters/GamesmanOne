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
#define PRAGMA_OMP_PARALLEL_FOR PRAGMA(omp parallel for)

// Otherwise, the following macros do nothing.
#else
#define PRAGMA
#define PRAGMA_OMP_PARALLEL_FOR
#endif

/**
 * @brief Maximum number of bits per array entry.
 * @details Currently set to 32 to simplify the implementation of segment
 * reading and writing. A segment is currently defined as 8 consecutive bytes
 * containing all the bits of an entry. Whereas an entry of length at most 32
 * must lie within a single segment, an entry longer than 32 bits may span more
 * than 1 segment, making it impossible to use a single uint64_t to access.
 */
static const int kMaxBitsPerEntry = 32;

static int GetBitsPerEntry(uint64_t num_values);
static int64_t GetStreamSize(int64_t num_entries, int bits_per_entry);

static uint64_t GetSegment(const BpArray *array, int64_t i);
static void SetSegment(const BpArray *array, int64_t i, uint64_t segment,
                       int num_bytes);
static void SetSegmentTs(const BpArray *array, int64_t i, uint64_t segment);
static int64_t GetBitOffset(int64_t i, int bits_per_entry);
static int64_t GetLocalBitOffset(int64_t i, int bits_per_entry);
static int64_t GetByteOffset(int64_t i, int bits_per_entry);
static uint64_t GetEntryMask(int bits_per_entry, int local_bit_offset);

// -----------------------------------------------------------------------------

int BpArrayInit(BpArray *array, int64_t size, uint64_t num_values) {
    int bits_per_entry = GetBitsPerEntry(num_values);
    if (bits_per_entry < 0) {
        fprintf(stderr,
                "BpArrayInit: the given num_values [%" PRIu64
                "] exceeds %d bits and is not supported by BpArray.\n",
                num_values, kMaxBitsPerEntry);
        memset(array, 0, sizeof(*array));
        return -1;
    }
    array->num_values = num_values;
    array->meta.bits_per_entry = bits_per_entry;
    array->meta.num_entries = size;
    array->meta.stream_length = GetStreamSize(size, bits_per_entry);
    array->stream =
        (uint8_t *)calloc(array->meta.stream_length, sizeof(uint8_t));
    if (array->stream == NULL) {
        memset(array, 0, sizeof(*array));
        return 1;
    }
#ifdef _OPENMP
    omp_init_lock(&array->lock);
#endif
    return 0;
}

void BpArrayDestroy(BpArray *array) {
    free(array->stream);
#ifdef _OPENMP
    omp_destroy_lock(&array->lock);
#endif
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

// Sets the Ith entry to ENTRY. Assumes ENTRY is strictly less than
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

// Thread-safe version of BpArraySet().
int BpArraySetTs(BpArray *array, int64_t i, uint64_t entry) {
    int bits_per_entry = array->meta.bits_per_entry;
    int local_bit_offset = GetLocalBitOffset(i, bits_per_entry);
    uint64_t mask = GetEntryMask(bits_per_entry, local_bit_offset);

    // Get segment containing the old entry.
#ifdef _OPENMP
    omp_set_lock(&array->lock);
#endif
    uint64_t segment = GetSegment(array, i);
    segment &= ~mask;                      // Zero out old entry.
    segment |= entry << local_bit_offset;  // Put new entry.
    SetSegmentTs(array, i, segment);       // Put segment back.
#ifdef _OPENMP
    omp_unset_lock(&array->lock);
#endif
    return 0;
}

// -----------------------------------------------------------------------------

static int GetBitsPerEntry(uint64_t num_values) {
    assert(num_values > 0);
    for (int i = 1; i <= kMaxBitsPerEntry; ++i) {
        if (num_values <= ((uint64_t)1) << i) {
            return i;
        }
    }
    return -1;
}

static int64_t GetStreamSize(int64_t num_entries, int bits_per_entry) {
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
