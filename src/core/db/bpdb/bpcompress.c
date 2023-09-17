#include "core/db/bpdb/bpcompress.h"

#include <assert.h>  // assert
#include <stddef.h>  // NULL
#include <stdint.h>  // int32_t
#include <stdio.h>   // fprintf, stderr
#include <stdlib.h>  // calloc, free

#include "core/db/bpdb/bparray.h"
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

static const int kMaxBitsPerEntry = 31;

static int32_t Step0BuildCompressionDict(const BpArray *src,
                                         int32_t *compression_dict,
                                         int32_t compression_dict_size);
static void Step1BuildDecompDict(const int32_t *compression_dict,
                                 int32_t compression_dict_size,
                                 int32_t *decomp_dict);
static void Step3CompressAllEntries(BpArray *dest, const BpArray *src,
                                    const int32_t *compression_dict);

int BpCompress(BpArray *dest, const BpArray *src, int32_t **decomp_dict,
               int32_t *decomp_dict_size) {
    *decomp_dict = NULL;
    *decomp_dict_size = 0;

    if (src->meta.bits_per_entry > kMaxBitsPerEntry) {
        fprintf(stderr,
                "BpCompress: compression unavailable when each entry "
                "is longer than %d bits (currently %d)\n",
                kMaxBitsPerEntry, src->meta.bits_per_entry);
        return 2;
    }

    // Construct compression dictionary.
    int32_t compression_dict_size = (int32_t)src->num_values;
    int32_t *compression_dict =
        (int32_t *)calloc(compression_dict_size, sizeof(int32_t));
    if (compression_dict == NULL) {
        fprintf(stderr, "BpCompress: failed to calloc compression_dict\n");
        return 1;
    }

    int num_unique_values =
        Step0BuildCompressionDict(src, compression_dict, compression_dict_size);

    // Construct decomp dictionary.
    *decomp_dict = (int32_t *)calloc(num_unique_values, sizeof(int32_t));
    if (*decomp_dict == NULL) {
        fprintf(stderr, "BpCompress: failed to calloc decomp_dict\n");
        free(compression_dict);
        return 1;
    }
    *decomp_dict_size = num_unique_values * sizeof(int32_t);
    Step1BuildDecompDict(compression_dict, compression_dict_size, *decomp_dict);

    // Initialize destination array.
    int error = BpArrayInit(dest, src->meta.num_entries, num_unique_values);
    if (error != 0) {
        fprintf(stderr,
                "BpCompress: failed to initialize destination array, code %d\n",
                error);
        free(compression_dict);
        free(*decomp_dict);
        *decomp_dict = NULL;
        *decomp_dict_size = 0;
        return 1;
    }

    Step3CompressAllEntries(dest, src, compression_dict);

    free(compression_dict);
    return 0;
}

static int32_t Step0BuildCompressionDict(const BpArray *src,
                                         int32_t *compression_dict,
                                         int32_t compression_dict_size) {
    // Mark all entries that exist in ARRAY.
    PRAGMA_OMP_PARALLEL_FOR
    for (int64_t i = 0; i < src->meta.num_entries; ++i) {
        compression_dict[BpArrayGet(src, i)] = 1;
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

static void Step1BuildDecompDict(const int32_t *compression_dict,
                                 int32_t compression_dict_size,
                                 int32_t *decomp_dict) {
    // Not worth parallelizing.
    for (int32_t i = 0; i < compression_dict_size; ++i) {
        if (compression_dict[i] >= 0) {
            decomp_dict[compression_dict[i]] = i;
        }
    }
}

static void Step3_0CompressEntry(BpArray *dest, const BpArray *src,
                                 const int32_t *compression_dict,
                                 int64_t entry_index) {
    uint64_t entry = BpArrayGet(src, entry_index);
    uint64_t mapped = compression_dict[entry];
    BpArraySet(dest, entry_index, mapped);
}

static void Step3CompressAllEntries(BpArray *dest, const BpArray *src,
                                    const int32_t *compression_dict) {
    // Compress in chunks
    static const int kEntriesPerChunk = 8;
    int64_t num_chunks = src->meta.num_entries / kEntriesPerChunk;

    // Chunks do not overlap in stream and can be processed in parallel
    // without thread-safety protection.
    PRAGMA_OMP_PARALLEL_FOR
    for (int64_t chunk = 0; chunk < num_chunks; ++chunk) {
        // Entries within each chunk may overlap in bytes, so they must be
        // processed sequentially.
        for (int i = 0; i < kEntriesPerChunk; ++i) {
            int64_t entry_index = chunk * kEntriesPerChunk + i;
            Step3_0CompressEntry(dest, src, compression_dict, entry_index);
        }
    }

    // Compress remaining entries sequentially.
    int64_t first_remaining_entry = num_chunks * kEntriesPerChunk;
    for (int64_t i = first_remaining_entry; i < src->meta.num_entries; ++i) {
        Step3_0CompressEntry(dest, src, compression_dict, i);
    }
}
