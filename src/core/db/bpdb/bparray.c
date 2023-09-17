#include "core/db/bpdb/bparray.h"

#include <assert.h>    // assert
#include <stdbool.h>   // bool
#include <inttypes.h>  // PRIu64
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

static int BpArrayInitHelper(BpArray *array, int64_t size, int bits_per_entry);

static int64_t GetStreamLength(int64_t num_entries, int bits_per_entry);

static uint64_t GetSegment(BpArray *array, int64_t i);
static void SetSegment(BpArray *array, int64_t i, uint64_t segment);
static int64_t GetBitOffset(int64_t i, int bits_per_entry);
static int64_t GetLocalBitOffset(int64_t i, int bits_per_entry);
static int64_t GetByteOffset(int64_t i, int bits_per_entry);
static uint64_t GetEntryMask(int bits_per_entry, int local_bit_offset);

static uint64_t CompressEntry(LookupDict *lookup, uint64_t entry);
static bool ExpansionNeeded(BpArray *array, uint64_t entry);
static int BpArrayExpand(BpArray *array, uint64_t entry);
static int ExpandHelper(BpArray *array, int new_bits_per_entry);

// -----------------------------------------------------------------------------

int BpArrayInit(BpArray *array, int64_t size) {
    return BpArrayInitHelper(array, size, kDefaultBitsPerEntry);
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
    if (ExpansionNeeded(array, entry)) {
        int error = BpArrayExpand(array, compressed);
        if (error != 0) {
            fprintf(stderr, "BpArraySet: failed to expand array, code %d\n", error);
            return error;
        }

        // Must compress again to register the new entry in the new dictionary.
        compressed = CompressEntry(&array->lookup, entry);
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

static int BpArrayInitHelper(BpArray *array, int64_t size, int bits_per_entry) {
    array->meta.stream_length = GetStreamLength(size, bits_per_entry);
    array->stream =
        (uint8_t *)calloc(array->meta.stream_length, sizeof(uint8_t));
    if (array->stream == NULL) {
        fprintf(stderr, "BpArrayInit: failed to calloc array\n");
        memset(array, 0, sizeof(*array));
        return 1;
    }

    array->meta.bits_per_entry = bits_per_entry;
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

static int64_t GetStreamLength(int64_t num_entries, int bits_per_entry) {
    // 8 additional bytes are allocated so that it is always safe to read
    // 8 bytes for an entry.
    return RoundUpDivide(num_entries * bits_per_entry, kBitsPerByte) +
           sizeof(uint64_t);
}

static uint64_t GetSegment(BpArray *array, int64_t i) {
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

static bool ExpansionNeeded(BpArray *array, uint64_t entry) {
    return entry >= ((uint64_t)1) << array->meta.bits_per_entry;
}

static int BpArrayExpand(BpArray *array, uint64_t entry) {
    int new_bits_per_entry = array->meta.bits_per_entry + 1;
    if (new_bits_per_entry > kMaxBitsPerEntry) {
        fprintf(stderr,
                "BpArrayExpand: new entry %" PRIu64
                " cannot be represented by the longest number of bits allowed "
                "(%d) and therefore cannot be stored\n",
                entry, kMaxBitsPerEntry);
        return -1;
    }

    return ExpandHelper(array, new_bits_per_entry);
}

// TODO: do expansion in-place.
static int ExpandHelper(BpArray *array, int new_bits_per_entry) {
    BpArray new_array;
    int64_t size = array->meta.num_entries;
    int error = BpArrayInitHelper(&new_array, size, new_bits_per_entry);
    if (error != 0) {
        fprintf(stderr,
                "ExpandHelper: failed to initialize new array, code %d\n",
                error);
        return error;
    }

    // PRAGMA_OMP_PARALLEL_FOR
    for (int64_t i = 0; i < size; ++i) {
        uint64_t entry = BpArrayGet(array, i);
        BpArraySet(&new_array, i, entry);
    }

    BpArrayDestroy(array);
    *array = new_array;
    return 0;
}

#undef PRAGMA
#undef PRAGMA_OMP_PARALLEL_FOR
