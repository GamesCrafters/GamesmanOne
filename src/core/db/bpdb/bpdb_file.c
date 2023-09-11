#include "core/db/bpdb/bpdb_file.h"

#include <inttypes.h>  // PRId64
#include <stddef.h>    // NULL
#include <stdio.h>     // fprintf, stderr
#include <stdlib.h>    // free
#include <string.h>    // strlen

#include "core/db/bpdb/bparray.h"
#include "core/db/bpdb/bpcompress.h"
#include "core/gamesman_types.h"
#include "core/misc.h"
#include "libs/mgz/mgz.h"

static const int kMgzMinBlockSize = 1 << 14;  // 16 KiB as specified by mgz.
static const int kMgzCompressionLevel = 9;    // Maximum compression.
static const bool kMgzLookupNeeded = true;

static int FlushStep0CompressToBitStream(BpdbFileHeader *header,
                                         BpArray *stream,
                                         const BpArray *records,
                                         int32_t **decomp_dict);
static mgz_res_t FlushStep1MgzCompress(BpdbFileHeader *header,
                                       const BpArray *stream);
static int FlushStep2WriteToFile(ReadOnlyString full_path,
                                 const BpdbFileHeader *header,
                                 int32_t *decomp_dict, mgz_res_t result);

// -----------------------------------------------------------------------------

/**
 * @brief Returns the full path to the DB file for the given tier. The user is
 * responsible for free()ing the pointer returned by this function. Returns
 * NULL on failure.
 */
char *BpdbFileGetFullPath(ConstantReadOnlyString current_path, Tier tier) {
    // Full path: "<path>/<tier>.bpdb".
    static ConstantReadOnlyString kBpdbExtension = ".bpdb";
    int path_length = strlen(current_path) + 1 + kInt64Base10StringLengthMax +
                      strlen(kBpdbExtension) + 1;
    char *full_path = (char *)calloc(path_length, sizeof(char));
    if (full_path == NULL) {
        fprintf(stderr, "GetFullPathToFile: failed to calloc full_path.\n");
        return NULL;
    }

    char
        file_name[1 + kInt64Base10StringLengthMax + strlen(kBpdbExtension) + 1];
    snprintf(file_name, kInt64Base10StringLengthMax, "/%" PRId64 "%s", tier,
             kBpdbExtension);
    strcat(full_path, current_path);
    strcat(full_path, file_name);
    return full_path;
}

int BpdbFileFlush(ReadOnlyString full_path, const BpArray *records) {
    BpdbFileHeader header;
    BpArray stream;
    int32_t *decomp_dict = NULL;

    // Compress BpArray to bit stream.
    int error =
        FlushStep0CompressToBitStream(&header, &stream, records, &decomp_dict);
    if (error != 0) {
        fprintf(stderr,
                "BpdbLiteFlushSolvingTier: failed to compress records to "
                "bit stream, code %d\n",
                error);
        return error;
    }

    // Compress stream using mgz
    mgz_res_t result = FlushStep1MgzCompress(&header, &stream);
    if (result.out == NULL) {
        fprintf(stderr,
                "BpdbLiteFlushSolvingTier: failed to compress records using "
                "mgz.\n");
        return 1;
    }

    // Write compressed data to file.
    return FlushStep2WriteToFile(full_path, &header, decomp_dict, result);
}

int BpdbFileGetBlockSize(int bits_per_entry) {
    // Makes sure that each block contains an integer number of entries.
    return NextMultiple(kMgzMinBlockSize, bits_per_entry * sizeof(uint64_t));
}

// -----------------------------------------------------------------------------

static int FlushStep0CompressToBitStream(BpdbFileHeader *header,
                                         BpArray *stream,
                                         const BpArray *records,
                                         int32_t **decomp_dict) {
    int error = BpCompress(stream, records, decomp_dict,
                           &(header->decomp_dict_meta.size));
    if (error != 0) return error;
    header->stream_meta = stream->meta;
    return 0;
}

static mgz_res_t FlushStep1MgzCompress(BpdbFileHeader *header,
                                       const BpArray *stream) {
    int block_size = BpdbFileGetBlockSize(stream->meta.bits_per_entry);
    header->lookup_meta.block_size = block_size;
    mgz_res_t result =
        MgzParallelDeflate(stream->stream, stream->meta.stream_length,
                           kMgzCompressionLevel, block_size, kMgzLookupNeeded);
    if (result.out == NULL) return result;

    header->lookup_meta.size = result.num_blocks * sizeof(int64_t);
    return result;
}

static int FlushStep2WriteToFile(ReadOnlyString full_path,
                                 const BpdbFileHeader *header,
                                 int32_t *decomp_dict, mgz_res_t result) {
    // Write header to file.
    FILE *db_file = GuardedFopen(full_path, "wb");
    if (db_file == NULL) return 2;

    int error = GuardedFwrite(header, sizeof(*header), 1, db_file);
    if (error != 0) return BailOutFclose(db_file, error);

    // Write decomp dict.
    error =
        GuardedFwrite(decomp_dict, 1, header->decomp_dict_meta.size, db_file);
    if (error != 0) return BailOutFclose(db_file, error);
    free(decomp_dict);
    decomp_dict = NULL;

    // Write mgz lookup table.
    error = GuardedFwrite(result.lookup, sizeof(int64_t), result.num_blocks,
                          db_file);
    if (error != 0) return BailOutFclose(db_file, error);
    free(result.lookup);
    result.lookup = NULL;

    // Write mgz compressed stream.
    error = GuardedFwrite(result.out, 1, result.size, db_file);
    if (error != 0) return BailOutFclose(db_file, error);
    free(result.out);
    result.out = NULL;

    // Close the file.
    return GuardedFclose(db_file);
}
