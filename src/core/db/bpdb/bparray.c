#include "core/db/bpdb/bparray.h"

#include <assert.h>   // assert
#include <stdbool.h>  // bool, true, false
#include <stddef.h>   // NULL
#include <stdint.h>   // uint8_t, int64_t, uint64_t
#include <stdio.h>    // fprintf, stderr
#include <stdlib.h>   // calloc, free
#include <string.h>   // memset

#include "core/gamesman_types.h"

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

// Helper functions.

static int64_t RoundUpDivide(int64_t n, int64_t d);

static int GetBytesPerEntry(uint64_t max_value);
static int GetBitsPerEntry(int32_t num_unique_values);

static int32_t CompressStep0BuildCompressionDict(const BpArray *array,
                                                 int32_t *compression_dict,
                                                 int32_t compression_dict_size);
static void CompressStep1BuildDecompDict(const int32_t *compression_dict,
                                         int32_t compression_dict_size,
                                         int32_t *decomp_dict);
static bool CompressStep2AllocateBitStream(BitStream *stream,
                                           int64_t num_entries,
                                           int32_t num_unique_values);
static void CompressStep3CompressAllEntries(const BpArray *array,
                                            int32_t *compression_dict,
                                            BitStream *dest);
static void CompressStep3_0CompressEntry(const BpArray *array,
                                         int32_t *compression_dict,
                                         int64_t entry_index, BitStream *dest);

// -----------------------------------------------------------------------------

int BpArrayInit(BpArray *array, int64_t size, uint64_t max_value) {
    if (max_value == 0) {
        fprintf(stderr, "BpArrayInit: invalid max_value = 0\n");
        return 2;
    }

    array->bytes_per_entry = GetBytesPerEntry(max_value);
    array->array = (uint8_t *)calloc(size, array->bytes_per_entry);
    if (array->array == NULL) {
        fprintf(stderr, "BpArrayInit: failed to calloc array\n");
        memset(array, 0, sizeof(*array));
        return 1;
    }

    array->max_value = max_value;
    array->num_entries = size;

    return 0;
}

void BpArrayDestroy(BpArray *array) {
    free(array->array);
    memset(array, 0, sizeof(*array));
}

uint64_t BpArrayGet(const BpArray *array, int64_t index) {
    int64_t byte_offset = array->bytes_per_entry * index;
    uint64_t ret = 0;
    memcpy(&ret, array->array + byte_offset, array->bytes_per_entry);
    return ret;
}

void BpArraySet(BpArray *array, int64_t index, uint64_t value) {
    int64_t byte_offset = array->bytes_per_entry * index;
    memcpy(array->array + byte_offset, &value, array->bytes_per_entry);
}

int BpArrayCompress(const BpArray *array, BitStream *dest,
                    int32_t **decomp_dict, int32_t *decomp_dict_size) {
    *decomp_dict = NULL;
    *decomp_dict_size = 0;

    // This method doesn't work if each entry takes more than 3 bytes.
    if (array->bytes_per_entry > 3) {
        fprintf(stderr,
                "BpArrayCompress: unavailable when each entry takes more than "
                "3 bytes (currently %d)\n",
                array->bytes_per_entry);
        return 2;
    }

    // Construct compression dictionary.
    int32_t compression_dict_size = array->max_value + 1;
    int32_t *compression_dict =
        (int32_t *)calloc(compression_dict_size, sizeof(int32_t));
    if (compression_dict == NULL) {
        fprintf(stderr, "BpArrayCompress: failed to calloc compression_dict\n");
        return 1;
    }

    int num_unique_values = CompressStep0BuildCompressionDict(
        array, compression_dict, compression_dict_size);

    // Construct decomp dictionary.
    *decomp_dict = (int32_t *)calloc(num_unique_values, sizeof(int32_t));
    if (*decomp_dict == NULL) {
        fprintf(stderr, "BpArrayCompress: failed to calloc decomp_dict\n");
        free(compression_dict);
        return 1;
    }
    *decomp_dict_size = num_unique_values * sizeof(int32_t);
    CompressStep1BuildDecompDict(compression_dict, compression_dict_size,
                                 *decomp_dict);

    // Allocate BitStream
    bool success = CompressStep2AllocateBitStream(dest, array->num_entries,
                                                  *decomp_dict_size);
    if (!success) {
        fprintf(stderr, "BpArrayCompress: failed to allocate bit stream\n");
        free(compression_dict);
        free(*decomp_dict);
        *decomp_dict = NULL;
        *decomp_dict_size = 0;
        return 1;
    }

    CompressStep3CompressAllEntries(array, compression_dict, dest);

    free(compression_dict);
    return 0;
}

void BitStreamDestroy(BitStream *stream) {
    free(stream->stream);
    memset(stream, 0, sizeof(*stream));
}

// -----------------------------------------------------------------------------

static int64_t RoundUpDivide(int64_t n, int64_t d) { return (n + d - 1) / d; }

static int GetBytesPerEntry(uint64_t max_value) {
    assert(max_value > 0);
    for (size_t i = 1; i < sizeof(uint64_t); ++i) {
        if (max_value < (((uint64_t)1) << (i * kBitsPerByte))) {
            return i;
        }
    }
    return sizeof(uint64_t);
}

static int GetBitsPerEntry(int32_t num_unique_values) {
    assert(num_unique_values > 0);
    static const int kMaxBitsPerEntry = 32;
    for (int i = 1; i < kMaxBitsPerEntry; ++i) {
        if ((uint32_t)num_unique_values <= ((uint32_t)1) << i) {
            return i;
        }
    }
    return kMaxBitsPerEntry;
}

static int32_t CompressStep0BuildCompressionDict(
    const BpArray *array, int32_t *compression_dict,
    int32_t compression_dict_size) {
    // Mark all entries that exist in ARRAY.
    PRAGMA_OMP_PARALLEL_FOR
    for (int64_t i = 0; i < array->num_entries; ++i) {
        compression_dict[BpArrayGet(array, i)] = 1;
    }

    // Count the number of unique entries.
    int32_t num_unique_values = 0;
    for (int32_t i = 0; i < compression_dict_size; ++i) {
        if (compression_dict[i]) {
            compression_dict[i] = num_unique_values++;
        } else {
            compression_dict[i] = -1;
        }
    }

    return num_unique_values;
}

static void CompressStep1BuildDecompDict(const int32_t *compression_dict,
                                         int32_t compression_dict_size,
                                         int32_t *decomp_dict) {
    for (int32_t i = 0; i < compression_dict_size; ++i) {
        if (compression_dict[i] >= 0) {
            decomp_dict[compression_dict[i]] = i;
        }
    }
}

static bool CompressStep2AllocateBitStream(BitStream *stream,
                                           int64_t num_entries,
                                           int32_t num_unique_values) {
    stream->metadata.bits_per_entry = GetBitsPerEntry(num_unique_values);
    stream->metadata.num_entries = num_entries;

    // 8 additional bytes are allocated so that treating the last few entries as
    // uint64_t is safe.
    stream->metadata.stream_length =
        RoundUpDivide(
            stream->metadata.num_entries * stream->metadata.bits_per_entry,
            kBitsPerByte) +
        sizeof(uint64_t);
    stream->stream =
        (uint8_t *)calloc(stream->metadata.stream_length, sizeof(uint8_t));
    if (stream->stream == NULL) {
        memset(stream, 0, sizeof(*stream));
        return false;
    }
    return true;
}

static void CompressStep3_0CompressEntry(const BpArray *array,
                                         int32_t *compression_dict,
                                         int64_t entry_index, BitStream *dest) {
    int bits_per_entry = dest->metadata.bits_per_entry;
    uint64_t entry = BpArrayGet(array, entry_index);
    uint64_t mapped = compression_dict[entry];

    int64_t bit_offset = entry_index * dest->metadata.bits_per_entry;
    int64_t bit_end = bit_offset + bits_per_entry;

    int64_t byte_offset = bit_offset / kBitsPerByte;
    int64_t byte_end = bit_end / kBitsPerByte;
    int64_t num_bytes = byte_end - byte_offset + 1;

    int local_bit_offset = bit_offset % kBitsPerByte;

    uint64_t masked = mapped << local_bit_offset;

    uint64_t result = *((uint64_t *)(dest->stream + byte_offset)) | masked;
    memcpy(dest->stream + byte_offset, &result, num_bytes);
}

static void CompressStep3CompressAllEntries(const BpArray *array,
                                            int32_t *compression_dict,
                                            BitStream *dest) {
    // Compress in chunks
    static const int kEntriesPerChunk = 8;
    int64_t num_chunks = array->num_entries / kEntriesPerChunk;

    // Chunks do not overlap in BitStream and can be processed in parallel.
    PRAGMA_OMP_PARALLEL_FOR
    for (int64_t chunk = 0; chunk < num_chunks; ++chunk) {
        // Entries within each chunk may overlap in bytes, so they must be
        // processed sequentially.
        for (int i = 0; i < kEntriesPerChunk; ++i) {
            int64_t entry_index = chunk * kEntriesPerChunk + i;
            CompressStep3_0CompressEntry(array, compression_dict, entry_index,
                                         dest);
        }
    }

    // Compress remaining entries sequentially.
    int64_t first_remaining_entry = num_chunks * kEntriesPerChunk;
    for (int64_t i = first_remaining_entry; i < array->num_entries; ++i) {
        CompressStep3_0CompressEntry(array, compression_dict, i, dest);
    }
}

#undef PRAGMA
#undef PRAGMA_OMP_PARALLEL_FOR
