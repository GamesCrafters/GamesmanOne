#include "core/db/bpdb/bparray.h"

#include <assert.h>    // assert
#include <inttypes.h>  // PRIu64
#include <stdbool.h>   // bool
#include <stddef.h>    // NULL
#include <stdint.h>    // int8_t, uint8_t, int32_t, int64_t, uint64_t
#include <stdio.h>     // fprintf, stderr
#include <stdlib.h>    // calloc, free
#include <string.h>    // memset

#include "core/db/bpdb/lookup_dict.h"
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

static const int kDefaultBitsPerEntry = 1;

/**
 * @brief Maximum number of bits per array entry.
 *
 * @details Currently set to 31 because LookupDict uses int32_t arrays for
 * compression and decompression.
 *
 * Also note that the algorithm in this module requires kMaxBitsPerEntry <= 32
 * to simplify the implementation of segment reading and writing. A segment is
 * currently defined as 8 consecutive bytes containing all the bits of an entry.
 * While an entry of length at most 32 bits must lie within a single segment, an
 * entry longer than 32 bits may span more than 1 segment, making it impossible
 * to use a single uint64_t to access.
 */
static const int kMaxBitsPerEntry = 31;

static int64_t GetStreamLength(int64_t num_entries, int bits_per_entry);

static uint64_t GetSegment(const BpArray *array, int64_t i);
static void SetSegment(BpArray *array, int64_t i, uint64_t segment);
static int64_t GetBitOffset(int64_t i, int bits_per_entry);
static int64_t GetLocalBitOffset(int64_t i, int bits_per_entry);
static int64_t GetByteOffset(int64_t i, int bits_per_entry);
static uint64_t GetEntryMask(int bits_per_entry, int local_bit_offset);

static uint64_t CompressEntry(LookupDict *lookup, uint64_t entry);
static bool ExpansionNeeded(const BpArray *array, uint64_t entry);
static int BpArrayExpand(BpArray *array);
static int ExpandHelper(BpArray *array, int new_bits_per_entry);

// -----------------------------------------------------------------------------

int BpArrayInit(BpArray *array, int64_t size) {
    array->meta.stream_length = GetStreamLength(size, kDefaultBitsPerEntry);
    array->stream =
        (uint8_t *)calloc(array->meta.stream_length, sizeof(uint8_t));
    if (array->stream == NULL) {
        fprintf(stderr, "BpArrayInit: failed to calloc array\n");
        memset(array, 0, sizeof(*array));
        return 1;
    }

    array->meta.bits_per_entry = kDefaultBitsPerEntry;
    array->meta.num_entries = size;
    int error = LookupDictInit(&array->lookup);
    if (error != 0) {
        fprintf(
            stderr,
            "BpArrayInit: failed to initialize lookup dictionary, code %d\n",
            error);
        free(array->stream);
        memset(array, 0, sizeof(*array));
        return error;
    }

    return 0;
}

void BpArrayDestroy(BpArray *array) {
    free(array->stream);
    LookupDictDestroy(&array->lookup);
    memset(array, 0, sizeof(*array));
}

// Returns the entry at index I.
uint64_t BpArrayGet(BpArray *array, int64_t i) {
    int bits_per_entry = array->meta.bits_per_entry;
    int local_bit_offset = GetLocalBitOffset(i, bits_per_entry);
    uint64_t mask = GetEntryMask(bits_per_entry, local_bit_offset);
    uint64_t segment = GetSegment(array, i);
    uint64_t value = (segment & mask) >> local_bit_offset;

    return LookupDictGetKey(&array->lookup, value);
}

int BpArraySet(BpArray *array, int64_t i, uint64_t entry) {
    uint64_t compressed = CompressEntry(&array->lookup, entry);
    if (ExpansionNeeded(array, compressed)) {
        int error = BpArrayExpand(array);
        if (error != 0) {
            fprintf(stderr, "BpArraySet: failed to expand array, code %d\n",
                    error);
            return error;
        }
    }

    int bits_per_entry = array->meta.bits_per_entry;
    int local_bit_offset = GetLocalBitOffset(i, bits_per_entry);
    uint64_t mask = GetEntryMask(bits_per_entry, local_bit_offset);

    // Get segment containing the old entry.
    uint64_t segment = GetSegment(array, i);
    segment &= ~mask;                           // Zero out old entry.
    segment |= compressed << local_bit_offset;  // Put new entry.
    SetSegment(array, i, segment);              // Put segment back.

    return 0;
}

int32_t BpArrayGetNumUniqueValues(const BpArray *array) {
    return array->lookup.num_unique;
}

const int32_t *BpArrayGetDecompDict(const BpArray *array) {
    return array->lookup.decomp_dict;
}

// -----------------------------------------------------------------------------

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

static void SetSegment(BpArray *array, int64_t i, uint64_t segment) {
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

static uint64_t CompressEntry(LookupDict *lookup, uint64_t entry) {
    int32_t compressed = LookupDictGet(lookup, entry);
    if (compressed < 0) {
        int error = LookupDictSet(lookup, entry);
        if (error != 0) {
            fprintf(stderr, "BpArraySet: failed to set LookupDict, code %d\n",
                    error);
            return error;
        }
        compressed = LookupDictGet(lookup, entry);
        assert(compressed >= 0);
    }
    return compressed;
}

static bool ExpansionNeeded(const BpArray *array, uint64_t entry) {
    return entry >= ((uint64_t)1) << array->meta.bits_per_entry;
}

static int BpArrayExpand(BpArray *array) {
    int new_bits_per_entry = array->meta.bits_per_entry + 1;
    if (new_bits_per_entry > kMaxBitsPerEntry) {
        fprintf(stderr,
                "BpArrayExpand: new entry cannot be represented by the longest "
                "number of bits allowed (%d) and therefore cannot be stored\n",
                kMaxBitsPerEntry);
        return -1;
    }

    return ExpandHelper(array, new_bits_per_entry);
}

static void CopyEntry(uint8_t *dest, const BpArray *src, int64_t i) {
    int bits_per_entry = src->meta.bits_per_entry;
    int local_bit_offset = GetLocalBitOffset(i, bits_per_entry);
    int new_local_bit_offset = GetLocalBitOffset(i, bits_per_entry + 1);
    uint64_t mask = GetEntryMask(bits_per_entry, local_bit_offset);

    // Get segment containing the old entry.
    uint64_t segment = GetSegment(src, i);
    segment = (segment & mask) >> local_bit_offset;  // Get old entry.

    // Make new segment.
    int64_t byte_offset = GetByteOffset(i, bits_per_entry + 1);
    uint8_t *address = dest + byte_offset;
    segment = *((uint64_t *)address) | (segment << new_local_bit_offset);

    // Put new segment.
    int64_t byte_end = GetByteOffset(i + 1, bits_per_entry + 1);
    int bytes_to_copy = byte_end - byte_offset + 1;
    memcpy(address, &segment, bytes_to_copy);
}

static int ExpandHelper(BpArray *array, int new_bits_per_entry) {
    int64_t size = array->meta.num_entries;
    int64_t new_stream_length = GetStreamLength(size, new_bits_per_entry);
    uint8_t *new_stream = (uint8_t *)calloc(new_stream_length, sizeof(uint8_t));
    if (new_stream == NULL) {
        fprintf(stderr, "ExpandHelper: failed to calloc new stream\n");
        return 1;
    }

    static const int kEntriesPerChunk = 8;
    int64_t num_chunks = size / kEntriesPerChunk;
    PRAGMA_OMP_PARALLEL_FOR
    for (int64_t chunk = 0; chunk < num_chunks; ++chunk) {
        for (int i = 0; i < kEntriesPerChunk; ++i) {
            CopyEntry(new_stream, array, chunk * kEntriesPerChunk + i);
        }
    }

    for (int64_t i = num_chunks * kEntriesPerChunk; i < size; ++i) {
        CopyEntry(new_stream, array, i);
    }

    free(array->stream);
    array->stream = new_stream;
    array->meta.bits_per_entry = new_bits_per_entry;
    array->meta.stream_length = new_stream_length;
    return 0;
}

#undef PRAGMA
#undef PRAGMA_OMP_PARALLEL_FOR
