#include "core/db/bpdb/bpdb_lite.h"

#include <assert.h>    // assert
#include <fcntl.h>     // O_RDONLY
#include <inttypes.h>  // PRId64
#include <stdbool.h>   // bool, true, false
#include <stddef.h>    // NULL
#include <stdint.h>    // int64_t, uint64_t, uint32_t, UINT64_MAX
#include <stdio.h>     // fprintf, stderr, FILE, SEEK_SET
#include <stdlib.h>    // malloc, free, realloc
#include <string.h>    // memset
#include <zlib.h>

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

static const int kNumValues = 5;            // undecided, lose, draw, tie, win.
static const int kMinBlockSize = 1 << 14;   // 16 KiB as specified by mgz.
static const int kMgzCompressionLevel = 9;  // Maximum compression.

static const int kBytesPerRecord = 2;
static const int kHeaderSize = sizeof(BpdbFileHeader);
static const int kDefaultDecompDictSize =
    (2 << (kBytesPerRecord * kBitsPerByte)) * sizeof(uint32_t);
static const int kDefaultBitsPerEntry = 8;
static const int kBlocksPerBuffer = 2;

static char current_game_name[kGameNameLengthMax + 1];
static int current_variant;
static char *current_path;
static Tier current_tier;
static int64_t current_tier_size;
static BpArray records;

static char *GetFullPathToFile(Tier tier);
static uint64_t BuildRecord(Value value, int remoteness);
static Value GetValueFromRecord(uint64_t record);
static int GetRemotenessFromRecord(uint64_t record);
static int GetBlockSize(int bits_per_entry);
static int GetBufferSize(int32_t decomp_dict_size, int bits_per_entry);

// Flush helper functions

static int FlushStep0CompressToBitStream(BpdbFileHeader *header,
                                         BitStream *stream,
                                         int32_t **decomp_dict);
static mgz_res_t FlushStep1MgzCompress(BpdbFileHeader *header,
                                       const BitStream *stream);
static int FlushStep2WriteToFile(const BpdbFileHeader *header,
                                 int32_t *decomp_dict, mgz_res_t result);

// Probe helper functions
static uint64_t ProbeRecord(DbProbe *probe, TierPosition tier_position);
const BpdbFileHeader *ProbeGetHeader(const DbProbe *probe);
const int32_t *ProbeGetDecompDict(const DbProbe *probe);
int ProbeGetBlockSize(const DbProbe *probe);
int32_t ProbeGetDecompDictSize(const DbProbe *probe);
int32_t ProbeGetLookupTableSize(const DbProbe *probe);
int ProbeGetBitsPerEntry(const DbProbe *probe);

static int BpdbLiteInit(ReadOnlyString game_name, int variant,
                        ReadOnlyString path, void *aux) {
    (void)aux;  // Unused.
    assert(current_path == NULL);

    current_path = (char *)malloc((strlen(path) + 1) * sizeof(char));
    if (current_path == NULL) {
        fprintf(stderr, "BpdbLiteInit: failed to malloc path.\n");
        return 1;
    }
    strcpy(current_path, path);

    SafeStrncpy(current_game_name, game_name, kGameNameLengthMax + 1);
    current_game_name[kGameNameLengthMax] = '\0';
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
    int32_t *decomp_dict = NULL;

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
    int remoteness = GetRemotenessFromRecord(record);
    record = BuildRecord(value, remoteness);
    BpArraySet(&records, position, record);

    return 0;
}

static int BpdbLiteSetRemoteness(Position position, int remoteness) {
    uint64_t record = BpArrayGet(&records, position);
    Value value = GetValueFromRecord(record);
    record = BuildRecord(value, remoteness);
    BpArraySet(&records, position, record);

    return 0;
}

static Value BpdbLiteGetValue(Position position) {
    uint64_t record = BpArrayGet(&records, position);
    return GetValueFromRecord(record);
}

static int BpdbLiteGetRemoteness(Position position) {
    uint64_t record = BpArrayGet(&records, position);
    return GetRemotenessFromRecord(record);
}

static int BpdbLiteProbeInit(DbProbe *probe) {
    int default_buffer_size =
        GetBufferSize(kDefaultDecompDictSize, kDefaultBitsPerEntry);
    probe->buffer = malloc(default_buffer_size);
    if (probe->buffer == NULL) return 1;

    probe->tier = -1;
    probe->begin = -1;
    probe->size = default_buffer_size;

    return 0;
}

static int BpdbLiteProbeDestroy(DbProbe *probe) {
    free(probe->buffer);
    memset(probe, 0, sizeof(*probe));
    return 0;
}

static Value BpdbLiteProbeValue(DbProbe *probe, TierPosition tier_position) {
    uint64_t record = ProbeRecord(probe, tier_position);
    return GetValueFromRecord(record);
}

static int BpdbLiteProbeRemoteness(DbProbe *probe, TierPosition tier_position) {
    uint64_t record = ProbeRecord(probe, tier_position);
    return GetRemotenessFromRecord(record);
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

static uint64_t BuildRecord(Value value, int remoteness) {
    return remoteness * kNumValues + value;
}

static Value GetValueFromRecord(uint64_t record) { return record % kNumValues; }

static int GetRemotenessFromRecord(uint64_t record) {
    return record / kNumValues;
}

static int GetBlockSize(int bits_per_entry) {
    return NextMultiple(kMinBlockSize, bits_per_entry * sizeof(uint64_t));
}

static int GetBufferSize(int32_t decomp_dict_size, int bits_per_entry) {
    // Header format:
    // [header][decomp_dict][bit_stream_block_0][bit_stream_block_1][...]
    // plus 8 bytes of additional space for safe uint64_t masking.
    int block_size = GetBlockSize(bits_per_entry);
    return kHeaderSize + decomp_dict_size + kBlocksPerBuffer * block_size +
           sizeof(uint64_t);
}

static int FlushStep0CompressToBitStream(BpdbFileHeader *header,
                                         BitStream *stream,
                                         int32_t **decomp_dict) {
    int error = BpArrayCompress(&records, stream, decomp_dict,
                                &(header->decomp_dict_metadata.size));
    if (error != 0) return error;
    header->stream_metadata = stream->metadata;
    return 0;
}

static mgz_res_t FlushStep1MgzCompress(BpdbFileHeader *header,
                                       const BitStream *stream) {
    int block_size = GetBlockSize(stream->metadata.bits_per_entry);
    header->lookup_table_metadata.block_size = block_size;
    mgz_res_t result =
        MgzParallelDeflate(stream->stream, stream->metadata.stream_length,
                           kMgzCompressionLevel, block_size, true);
    if (result.out == NULL) return result;

    header->lookup_table_metadata.size = result.num_blocks * sizeof(int64_t);
    return result;
}

static int FlushStep2WriteToFile(const BpdbFileHeader *header,
                                 int32_t *decomp_dict, mgz_res_t result) {
    // Write header to file
    char *full_path = GetFullPathToFile(current_tier);
    if (full_path == NULL) return 1;

    FILE *db_file = GuardedFopen(full_path, "wb");
    free(full_path);
    full_path = NULL;
    if (db_file == NULL) return 2;

    int error = GuardedFwrite(header, sizeof(*header), 1, db_file);
    if (error != 0) return BailOutFclose(db_file, error);

    // Write decomp dict
    error = GuardedFwrite(decomp_dict, 1, header->decomp_dict_metadata.size,
                          db_file);
    if (error != 0) return BailOutFclose(db_file, error);
    free(decomp_dict);
    decomp_dict = NULL;

    // Write mgz lookup table
    error = GuardedFwrite(result.lookup, sizeof(int64_t), result.num_blocks,
                          db_file);
    if (error != 0) return BailOutFclose(db_file, error);
    free(result.lookup);
    result.lookup = NULL;

    // Write mgz compressed stream
    error = GuardedFwrite(result.out, 1, result.size, db_file);
    if (error != 0) return BailOutFclose(db_file, error);
    free(result.out);
    result.out = NULL;

    // Close the file.
    return GuardedFclose(db_file);
}

static bool ExpandProbeBuffer(DbProbe *probe, int target_size) {
    if (probe->size >= target_size) return true;
    int new_size = probe->size;
    while (new_size < target_size) {
        new_size *= 2;
    }

    void *new_buffer = realloc(probe->buffer, new_size);
    if (new_buffer == NULL) return false;
    probe->buffer = new_buffer;
    probe->size = new_size;

    return true;
}

// Reloads tier bpdb file header and decomp dict into probe's cache.
static int ReloadProbeHeader(DbProbe *probe, Tier tier) {
    char *full_path = GetFullPathToFile(tier);
    if (full_path == NULL) return 1;

    FILE *db_file = GuardedFopen(full_path, "rb");
    free(full_path);
    full_path = NULL;
    if (db_file == NULL) return 2;

    // Read header. Assumes probe->buffer has enough space to store the header.
    int error = GuardedFread(probe->buffer, kHeaderSize, 1, db_file);
    if (error != 0) return BailOutFclose(db_file, error);

    // Make sure probe->buffer has enough space for the next read.
    int bits_per_entry = ProbeGetBitsPerEntry(probe);
    int32_t decomp_dict_size = ProbeGetDecompDictSize(probe);
    int target_size = GetBufferSize(decomp_dict_size, bits_per_entry);
    if (!ExpandProbeBuffer(probe, target_size)) {
        fprintf(stderr, "ReloadProbeHeader: failed to expand probe buffer\n");
        return BailOutFclose(db_file, 1);
    }

    // Read decompression dictionary.
    error = GuardedFread(GenericPointerAdd(probe->buffer, kHeaderSize),
                         decomp_dict_size, 1, db_file);
    if (error != 0) return BailOutFclose(db_file, error);

    // Uninitialize probe->begin so that a cache miss is triggered.
    probe->begin = -1;
    probe->tier = tier;

    return GuardedFclose(db_file);
}

static bool ProbeCacheMiss(const DbProbe *probe, Position position) {
    if (probe->begin < 0) return true;

    // Read header from probe
    int bits_per_entry = ProbeGetBitsPerEntry(probe);
    int block_size = ProbeGetBlockSize(probe);

    int64_t entry_bit_begin = position * bits_per_entry;
    int64_t entry_bit_end = entry_bit_begin + bits_per_entry;

    int64_t buffer_bit_begin = probe->begin * kBitsPerByte;
    int64_t buffer_bit_end =
        buffer_bit_begin + kBlocksPerBuffer * block_size * kBitsPerByte;

    if (entry_bit_begin < buffer_bit_begin) return false;
    if (entry_bit_end > buffer_bit_end) return false;
    return true;
}

static int64_t GetBitOffset(Position position, int bits_per_entry) {
    return (int64_t)position * bits_per_entry;
}

static int64_t GetByteOffset(Position position, int bits_per_entry) {
    int64_t bit_offset = GetBitOffset(position, bits_per_entry);
    return bit_offset / kBitsPerByte;
}

static int64_t GetBlockOffset(Position position, int bits_per_entry,
                              int block_size) {
    int64_t byte_offset = GetByteOffset(position, bits_per_entry);
    return byte_offset / block_size;
}

static int64_t ReadCompressedOffset(const DbProbe *probe, Position position,
                                    const char *full_path) {
    int32_t decomp_dict_size = ProbeGetDecompDictSize(probe);
    int bits_per_entry = ProbeGetBitsPerEntry(probe);
    int block_size = ProbeGetBlockSize(probe);

    FILE *db_file = GuardedFopen(full_path, "rb");
    if (db_file == NULL) return -1;

    int64_t block_offset = GetBlockOffset(position, bits_per_entry, block_size);

    // Seek to the entry in lookup that corresponds to block_offset.
    int64_t seek_length =
        kHeaderSize + decomp_dict_size + block_offset * sizeof(int64_t);
    int error = GuardedFseek(db_file, seek_length, SEEK_SET);
    if (error) return BailOutFclose(db_file, -1);

    // Read offset into the compressed bit stream.
    int64_t compressed_offset;
    error = GuardedFread(&compressed_offset, sizeof(int64_t), 1, db_file);
    if (error) return BailOutFclose(db_file, -1);

    // Finalize.
    error = GuardedFclose(db_file);
    if (error != 0) return -1;

    return compressed_offset;
}

static void *ProbeGetBitStream(const DbProbe *probe) {
    int32_t decomp_dict_size = ProbeGetDecompDictSize(probe);
    return GenericPointerAdd(probe->buffer, kHeaderSize + decomp_dict_size);
}

static int LoadBlocks(DbProbe *probe, Position position) {
    // Calculate the address of probe's bit stream blocks.
    int32_t decomp_dict_size = ProbeGetDecompDictSize(probe);
    int32_t lookup_table_size = ProbeGetLookupTableSize(probe);
    int bits_per_entry = ProbeGetBitsPerEntry(probe);
    void *buffer_blocks = ProbeGetBitStream(probe);

    char *full_path = GetFullPathToFile(probe->tier);
    if (full_path == NULL) return 1;

    // Get offset into the compressed bit stream.
    int64_t compressed_offset =
        ReadCompressedOffset(probe, position, full_path);
    if (compressed_offset == -1) {
        free(full_path);
        return -1;
    }
    compressed_offset += kHeaderSize + decomp_dict_size + lookup_table_size;

    // Open db_file
    int db_fd = GuardedOpen(full_path, O_RDONLY);
    free(full_path);
    full_path = NULL;
    if (db_fd == -1) return BailOutClose(db_fd, -1);

    // lseek() to the beginning of the first block that contains POSITION.
    int error = GuardedLseek(db_fd, compressed_offset, SEEK_SET);
    if (error != 0) return BailOutClose(db_fd, error);

    // Wrap db_fd in a gzFile container.
    gzFile compressed_stream = GuardedGzdopen(db_fd, "rb");
    if (compressed_stream == Z_NULL) return BailOutClose(db_fd, -1);

    // Since the cursor is already at the beginning of the first block,
    // we can start reading from there.
    int block_size = ProbeGetBlockSize(probe);
    error = GuardedGzread(compressed_stream, buffer_blocks,
                          kBlocksPerBuffer * block_size, true);
    if (error != 0) return BailOutGzclose(compressed_stream, error);

    // This will also close db_fd.
    error = GuardedGzclose(compressed_stream);
    if (error != Z_OK) return -1;

    // Set PROBE->begin
    int64_t block_offset = GetBlockOffset(position, bits_per_entry, block_size);
    probe->begin = block_offset * block_size;

    return 0;
}

// Loads the record of POSITION, assuming the requested record is in PROBE's
// buffer.
static uint64_t ProbeLoadRecord(const DbProbe *probe, Position position) {
    const int32_t *decomp_dict = ProbeGetDecompDict(probe);
    int bits_per_entry = ProbeGetBitsPerEntry(probe);

    int64_t bit_offset = GetBitOffset(position, bits_per_entry);
    int64_t byte_offset = bit_offset / kBitsPerByte - probe->begin;
    int local_bit_offset = bit_offset % kBitsPerByte;
    uint64_t mask = (((uint64_t)1 << bits_per_entry) - 1) << local_bit_offset;

    // Get encoded entry from bit stream.
    const void *bit_stream = ProbeGetBitStream(probe);
    const void *byte_address = GenericPointerAdd(bit_stream, byte_offset);
    uint64_t segment = *((uint64_t *)byte_address);
    uint64_t entry = (segment & mask) >> local_bit_offset;

    // Decode entry using decompression dictionary.
    return decomp_dict[entry];
}

static uint64_t ProbeRecord(DbProbe *probe, TierPosition tier_position) {
    if (probe->tier != tier_position.tier) {
        int error = ReloadProbeHeader(probe, tier_position.tier);
        if (error != 0) {
            printf(
                "ProbeRecord: failed to reload header and decompression "
                "dictionary into probe, code %d\n",
                error);
            return UINT64_MAX;
        }
    }
    if (ProbeCacheMiss(probe, tier_position.position)) {
        LoadBlocks(probe, tier_position.position);
    }
    return ProbeLoadRecord(probe, tier_position.position);
}

const BpdbFileHeader *ProbeGetHeader(const DbProbe *probe) {
    return (const BpdbFileHeader *)probe->buffer;
}

const int32_t *ProbeGetDecompDict(const DbProbe *probe) {
    return (const int32_t *)GenericPointerAdd(probe->buffer, kHeaderSize);
}

int ProbeGetBlockSize(const DbProbe *probe) {
    const BpdbFileHeader *header = ProbeGetHeader(probe);
    return header->lookup_table_metadata.block_size;
}

int32_t ProbeGetDecompDictSize(const DbProbe *probe) {
    const BpdbFileHeader *header = ProbeGetHeader(probe);
    return header->decomp_dict_metadata.size;
}

int32_t ProbeGetLookupTableSize(const DbProbe *probe) {
    const BpdbFileHeader *header = ProbeGetHeader(probe);
    return header->lookup_table_metadata.size;
}

int ProbeGetBitsPerEntry(const DbProbe *probe) {
    const BpdbFileHeader *header = ProbeGetHeader(probe);
    return header->stream_metadata.bits_per_entry;
}
