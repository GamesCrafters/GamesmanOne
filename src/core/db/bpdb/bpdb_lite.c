#include "core/db/bpdb/bpdb_lite.h"

#include <inttypes.h>  // PRId64
#include <stdbool.h>   // true
#include <stddef.h>    // NULL
#include <stdint.h>    // int64_t, uint64_t, uint32_t
#include <stdio.h>     // fprintf, stderr, FILE, fopen, fwrite, fread, fclose
#include <stdlib.h>    // malloc, free
#include <string.h>    // memset

#include "core/db/bpdb/bparray.h"
#include "core/gamesman_types.h"
#include "core/misc.h"
#include "libs/mgz/mgz.h"

// Database API.

static int BpdbLiteInit(ReadOnlyString game_name, int variant,
                        ReadOnlyString path, void *aux);
static void BpdbLiteFinalize(void);

static int BpdbLiteCreateSolvingTier(Tier tier, int64_t size);
static int BpdbLiteFlushSolvingTier(void *aux);
static int BpdbLiteFreeSolvingTier(void);

static int BpdbLiteSetValue(Position position, Value value);
static int BpdbLiteSetRemoteness(Position position, int remoteness);
static Value BpdbLiteGetValue(Position position);
static int BpdbLiteGetRemoteness(Position position);

static int BpdbLiteProbeInit(DbProbe *probe);
static int BpdbLiteProbeDestroy(DbProbe *probe);
static Value BpdbLiteProbeValue(DbProbe *probe, TierPosition tier_position);
static int BpdbLiteProbeRemoteness(DbProbe *probe, TierPosition tier_position);

const Database kBpdbLite = {
    .name = "bpdb_lite",
    .formal_name = "Bit-Perfect Database Lite",

    .Init = &BpdbLiteInit,
    .Finalize = &BpdbLiteFinalize,

    // Solving
    .CreateSolvingTier = &BpdbLiteCreateSolvingTier,
    .FlushSolvingTier = &BpdbLiteFlushSolvingTier,
    .FreeSolvingTier = &BpdbLiteFreeSolvingTier,
    .SetValue = &BpdbLiteSetValue,
    .SetRemoteness = &BpdbLiteSetRemoteness,
    .GetValue = &BpdbLiteGetValue,
    .GetRemoteness = &BpdbLiteGetRemoteness,

    // Probing
    .ProbeInit = &BpdbLiteProbeInit,
    .ProbeDestroy = &BpdbLiteProbeDestroy,
    .ProbeValue = &BpdbLiteProbeValue,
    .ProbeRemoteness = &BpdbLiteProbeRemoteness,
};

// -----------------------------------------------------------------------------

static const int64_t kMgzBlockSize = 1 << 14;  // 16 KiB.
static const int kMgzCompressionLevel = 9;     // Maximum compression.

static const int kBytesPerRecord = 2;
static const int kHeaderSize = sizeof(BpdbFileHeader);

static const int kDefaultDecompDictSize =
    (2 << (kBytesPerRecord * kBitsPerByte)) * sizeof(uint32_t);

// Format: [header][decomp_dict][bit_stream_block_0][bit_stream_block_1]
static const int kBufferSize =
    kHeaderSize + kDefaultDecompDictSize + kMgzBlockSize * 2;

static char current_game_name[kGameNameLengthMax + 1];
static int current_variant;
static char *current_path;
static Tier current_tier;
static int64_t current_tier_size;
static BpArray records;

static char *GetFullPathToFile(Tier tier);
static int FlushStep0CompressToBitStream(BpdbFileHeader *header,
                                         BitStream *stream,
                                         uint32_t **decomp_dict);
static mgz_res_t FlushStep1MgzCompress(BpdbFileHeader *header,
                                       const BitStream *stream);
static int FlushStep2WriteToFile(const BpdbFileHeader *header,
                                 uint32_t *decomp_dict, mgz_res_t result);

static int BpdbLiteInit(ReadOnlyString game_name, int variant,
                        ReadOnlyString path, void *aux) {
    assert(current_path == NULL);

    current_path = (char *)malloc((strlen(path) + 1) * sizeof(char));
    if (current_path == NULL) {
        fprintf(stderr, "BpdbLiteInit: failed to malloc path.\n");
        return 1;
    }

    strncpy(current_game_name, game_name, kGameNameLengthMax + 1);
    current_variant = variant;
    current_tier = -1;
    current_tier_size = -1;

    return 0;
}

static void BpdbLiteFinalize(void) {
    free(current_path);
    current_path = NULL;
    BpArrayDestroy(&records);
}

static int BpdbLiteCreateSolvingTier(Tier tier, int64_t size) {
    current_tier = tier;
    current_tier_size = size;

    BpArrayDestroy(&records);
    int error =
        BpArrayInit(&records, current_tier_size, kNumValues * kNumRemotenesses);
    if (error != 0) {
        fprintf(stderr,
                "BpdbLiteCreateSolvingTier: failed to initialize records, code "
                "%d\n",
                error);
        return error;
    }
    return 0;
}

static int BpdbLiteFlushSolvingTier(void *aux) {
    (void)aux;  // Unused.
    BpdbFileHeader header;
    BitStream stream;
    uint32_t *decomp_dict = NULL;

    // Compress BpArray to BitStream.
    int error = FlushStep0CompressToBitStream(&header, &stream, &decomp_dict);
    if (error != 0) {
        fprintf(stderr,
                "BpdbLiteFlushSolvingTier: failed to compress records to "
                "BitStream, code %d\n",
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
    return FlushStep2WriteToFile(&header, decomp_dict, result);
}

static int BpdbLiteFreeSolvingTier(void) {
    BpArrayDestroy(&records);
    current_tier = -1;
    current_tier_size = -1;
    return 0;
}

static int BpdbLiteSetValue(Position position, Value value) {
    uint64_t record = BpArrayGet(&records, position);
    record = (record / kNumValues) * kNumValues + value;
    BpArraySet(&records, position, record);

    return 0;
}

static int BpdbLiteSetRemoteness(Position position, int remoteness) {
    uint64_t record = BpArrayGet(&records, position);
    Value value = record % kNumValues;
    record = remoteness * kNumValues + value;
    BpArraySet(&records, position, record);

    return 0;
}

static Value BpdbLiteGetValue(Position position) {
    uint64_t record = BpArrayGet(&records, position);
    return record % kNumValues;
}

static int BpdbLiteGetRemoteness(Position position) {
    uint64_t record = BpArrayGet(&records, position);
    return record / kNumValues;
}

static int BpdbLiteProbeInit(DbProbe *probe) {
    probe->buffer = malloc(kBufferSize);
    if (probe->buffer == NULL) return 1;

    probe->tier = -1;
    probe->begin = 0;
    probe->size = kBufferSize;

    return 0;
}

static int BpdbLiteProbeDestroy(DbProbe *probe) {
    free(probe->buffer);
    memset(probe, 0, sizeof(*probe));
    return 0;
}

static Value BpdbLiteProbeValue(DbProbe *probe, TierPosition tier_position) {
    // TODO
    return kErrorValue;
}

static int BpdbLiteProbeRemoteness(DbProbe *probe, TierPosition tier_position) {
    // TODO
    return -1;
}

/**
 * @brief Returns the full path to the DB file for the given tier. The user is
 * responsible for free()ing the pointer returned by this function. Returns
 * NULL on failure.
 */
static char *GetFullPathToFile(Tier tier) {
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

static int FlushStep0CompressToBitStream(BpdbFileHeader *header,
                                         BitStream *stream,
                                         uint32_t **decomp_dict) {
    int error = BpArrayCompress(&records, stream, decomp_dict,
                                &(header->decomp_dict_metadata.size));
    if (error != 0) return error;
    header->stream_metadata = stream->metadata;
    return 0;
}

static mgz_res_t FlushStep1MgzCompress(BpdbFileHeader *header,
                                       const BitStream *stream) {
    header->lookup_table_metadata.block_size = kMgzBlockSize;
    mgz_res_t result =
        mgz_parallel_deflate(stream->stream, stream->metadata.stream_length,
                             kMgzCompressionLevel, kMgzBlockSize, true);
    if (result.out == NULL) return result;
    header->lookup_table_metadata.size = result.num_blocks * sizeof(int64_t);
    return result;
}

static int FlushStep2WriteToFile(const BpdbFileHeader *header,
                                 uint32_t *decomp_dict, mgz_res_t result) {
    // Write header to file
    char *full_path_to_file = GetFullPathToFile(current_tier);
    FILE *db_file = SafeFopen(full_path_to_file, "wb");
    if (db_file == NULL) return 2;

    int error = SafeFwrite(header, sizeof(*header), 1, db_file);
    if (error != 0) return error;

    // Write decomp dict
    error = SafeFwrite(decomp_dict, sizeof(int32_t),
                       header->decomp_dict_metadata.size, db_file);
    if (error != 0) return error;
    free(decomp_dict);
    decomp_dict = NULL;

    // Write mgz lookup table
    error =
        SafeFwrite(result.lookup, sizeof(int64_t), result.num_blocks, db_file);
    if (error != 0) return error;
    free(result.lookup);
    result.lookup = NULL;

    // Write mgz compressed stream
    error = SafeFwrite(result.out, 1, result.size, db_file);
    if (error != 0) return error;
    free(result.out);
    result.out = NULL;

    // Close the file.
    return SafeFclose(db_file);
}
