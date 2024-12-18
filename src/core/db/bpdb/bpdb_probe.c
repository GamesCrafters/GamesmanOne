/**
 * @file bpdb_probe.c
 * @author Dan Garcia: designed the "lookup table" compression algorithm
 * @author Max Fierro: improved the algorithm for BpArray compression
 * @author Sameer Nayyar: improved the algorithm for BpArray compression
 * @author Robert Shi (robertyishi@berkeley.edu): improved and implemented
 * BpDict.
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Implementation of the probe for the Bit-Perfect Database.
 * @version 1.1.2
 * @date 2024-12-10
 *
 * @copyright This file is part of GAMESMAN, The Finite, Two-person
 * Perfect-Information Game Generator released under the GPL:
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "core/db/bpdb/bpdb_probe.h"

#include <fcntl.h>   // O_RDONLY
#include <stddef.h>  // NULL
#include <stdint.h>  // int32_t, int64_t, uint64_t
#include <stdio.h>   // fprintf, stderr, FILE, SEEK_SET
#include <stdlib.h>  // malloc, free, realloc
#include <string.h>  // memset
#include <zlib.h>

#include "core/constants.h"
#include "core/db/bpdb/bpdb_file.h"
#include "core/misc.h"
#include "core/types/gamesman_types.h"

static const int kBlocksPerBuffer = 2;
static const int kHeaderSize = sizeof(BpdbFileHeader);

// (kNumValues - 2) because undecided and draw have no remoteness definition.
static const int kDefaultDecompDictSize =
    (kNumValues - 2) * kNumRemotenesses * sizeof(uint32_t);
static const int kDefaultBitsPerEntry = 8;

static int GetBufferSize(int32_t decomp_dict_size, int bits_per_entry);

static int ProbeRecordStep0ReloadHeader(ConstantReadOnlyString sandbox_path,
                                        DbProbe *probe, Tier tier,
                                        GetTierNameFunc GetTierName);
const BpdbFileHeader *ProbeGetHeader(const DbProbe *probe);
const int32_t *ProbeGetDecompDict(const DbProbe *probe);
int ProbeGetBlockSize(const DbProbe *probe);
int ProbeGetBitsPerEntry(const DbProbe *probe);
int32_t ProbeGetDecompDictSize(const DbProbe *probe);
int32_t ProbeGetLookupTableSize(const DbProbe *probe);
static bool ExpandProbeBuffer(DbProbe *probe, int target_size);

static bool ProbeRecordStep1CacheMiss(const DbProbe *probe, Position position);

static int ProbeRecordStep2LoadBlocks(ConstantReadOnlyString sandbox_path,
                                      DbProbe *probe, Position position,
                                      GetTierNameFunc GetTierName);
static int64_t GetBitOffset(Position position, int bits_per_entry);
static int64_t GetByteOffset(Position position, int bits_per_entry);
static int64_t GetBlockOffset(Position position, int bits_per_entry,
                              int block_size);
static void *ProbeGetBitStream(const DbProbe *probe);
static int64_t ProbeRecordStep2_0ReadCompressedOffset(const DbProbe *probe,
                                                      Position position,
                                                      const char *full_path);
static int ProbeRecordStep2_1HandleFile(const DbProbe *probe,
                                        const char *full_path,
                                        int64_t compressed_offset);

static uint64_t ProbeRecordStep3LoadRecord(const DbProbe *probe,
                                           Position position);

// -----------------------------------------------------------------------------

int BpdbProbeInit(DbProbe *probe) {
    int default_buffer_size =
        GetBufferSize(kDefaultDecompDictSize, kDefaultBitsPerEntry);
    probe->buffer = malloc(default_buffer_size);
    if (probe->buffer == NULL) return kMallocFailureError;

    probe->tier = kIllegalTier;
    probe->begin = -1;
    probe->size = default_buffer_size;

    return kNoError;
}

int BpdbProbeDestroy(DbProbe *probe) {
    free(probe->buffer);
    memset(probe, 0, sizeof(*probe));
    return kNoError;
}

uint64_t BpdbProbeRecord(ConstantReadOnlyString sandbox_path, DbProbe *probe,
                         TierPosition tier_position,
                         GetTierNameFunc GetTierName) {
    if (probe->tier != tier_position.tier) {
        int error = ProbeRecordStep0ReloadHeader(
            sandbox_path, probe, tier_position.tier, GetTierName);
        if (error != 0) {
            printf(
                "BpdbProbeRecord: failed to reload header and decompression "
                "dictionary into probe, code %d\n",
                error);
            return UINT64_MAX;
        }
    }
    if (ProbeRecordStep1CacheMiss(probe, tier_position.position)) {
        ProbeRecordStep2LoadBlocks(sandbox_path, probe, tier_position.position,
                                   GetTierName);
    }
    return ProbeRecordStep3LoadRecord(probe, tier_position.position);
}

// -----------------------------------------------------------------------------

static int GetBufferSize(int32_t decomp_dict_size, int bits_per_entry) {
    // Header format:
    // [header][decomp_dict][bit_stream_block_0][bit_stream_block_1][...]
    // plus 8 bytes of additional space for safe uint64_t masking.
    int block_size = BpdbFileGetBlockSize(bits_per_entry);
    return kHeaderSize + decomp_dict_size + kBlocksPerBuffer * block_size +
           sizeof(uint64_t);
}

// Reloads tier bpdb file header and decomp dict into probe's cache.
static int ProbeRecordStep0ReloadHeader(ConstantReadOnlyString sandbox_path,
                                        DbProbe *probe, Tier tier,
                                        GetTierNameFunc GetTierName) {
    char *full_path = BpdbFileGetFullPath(sandbox_path, tier, GetTierName);
    if (full_path == NULL) return kMallocFailureError;

    FILE *db_file = GuardedFopen(full_path, "rb");
    free(full_path);
    full_path = NULL;
    if (db_file == NULL) return kFileSystemError;

    // Read header. Assumes probe->buffer has enough space to store the header.
    int error = GuardedFread(probe->buffer, kHeaderSize, 1, db_file, false);
    if (error != 0) return BailOutFclose(db_file, error);

    // Make sure probe->buffer has enough space for the next read.
    int bits_per_entry = ProbeGetBitsPerEntry(probe);
    int32_t decomp_dict_size = ProbeGetDecompDictSize(probe);
    int target_size = GetBufferSize(decomp_dict_size, bits_per_entry);
    if (!ExpandProbeBuffer(probe, target_size)) {
        fprintf(
            stderr,
            "ProbeRecordStep0ReloadHeader: failed to expand probe buffer\n");
        return BailOutFclose(db_file, 1);
    }

    // Read decompression dictionary.
    error = GuardedFread(GenericPointerAdd(probe->buffer, kHeaderSize),
                         decomp_dict_size, 1, db_file, false);
    if (error != 0) return BailOutFclose(db_file, error);

    // Uninitialize probe->begin so that a cache miss is triggered.
    probe->begin = -1;
    probe->tier = tier;

    return GuardedFclose(db_file);
}

const BpdbFileHeader *ProbeGetHeader(const DbProbe *probe) {
    return (const BpdbFileHeader *)probe->buffer;
}

const int32_t *ProbeGetDecompDict(const DbProbe *probe) {
    return (const int32_t *)GenericPointerAdd(probe->buffer, kHeaderSize);
}

int ProbeGetBlockSize(const DbProbe *probe) {
    const BpdbFileHeader *header = ProbeGetHeader(probe);
    return header->lookup_meta.block_size;
}

int32_t ProbeGetDecompDictSize(const DbProbe *probe) {
    const BpdbFileHeader *header = ProbeGetHeader(probe);
    return header->decomp_dict_meta.size;
}

int32_t ProbeGetLookupTableSize(const DbProbe *probe) {
    const BpdbFileHeader *header = ProbeGetHeader(probe);
    return header->lookup_meta.size;
}

int ProbeGetBitsPerEntry(const DbProbe *probe) {
    const BpdbFileHeader *header = ProbeGetHeader(probe);
    return header->stream_meta.bits_per_entry;
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

static bool ProbeRecordStep1CacheMiss(const DbProbe *probe, Position position) {
    if (probe->begin < 0) return true;

    // Read header from probe
    int bits_per_entry = ProbeGetBitsPerEntry(probe);
    int block_size = ProbeGetBlockSize(probe);

    int64_t entry_bit_begin = position * bits_per_entry;
    int64_t entry_bit_end = entry_bit_begin + bits_per_entry;

    int64_t buffer_bit_begin = probe->begin * kBitsPerByte;
    int64_t buffer_bit_end =
        buffer_bit_begin + kBlocksPerBuffer * block_size * kBitsPerByte;

    if (entry_bit_begin < buffer_bit_begin) return true;
    if (entry_bit_end > buffer_bit_end) return true;
    return false;
}

static int ProbeRecordStep2LoadBlocks(ConstantReadOnlyString sandbox_path,
                                      DbProbe *probe, Position position,
                                      GetTierNameFunc GetTierName) {
    int ret = kRuntimeError;
    char *full_path =
        BpdbFileGetFullPath(sandbox_path, probe->tier, GetTierName);
    if (full_path == NULL) return kMallocFailureError;

    // Get offset into the compressed bit stream.
    int64_t compressed_offset =
        ProbeRecordStep2_0ReadCompressedOffset(probe, position, full_path);
    if (compressed_offset == -1) goto _bailout;

    ret = ProbeRecordStep2_1HandleFile(probe, full_path, compressed_offset);
    if (ret != kNoError) goto _bailout;

    // Set PROBE->begin
    int block_size = ProbeGetBlockSize(probe);
    int bits_per_entry = ProbeGetBitsPerEntry(probe);
    int64_t block_offset = GetBlockOffset(position, bits_per_entry, block_size);
    probe->begin = block_offset * block_size;

    // Success.
    ret = kNoError;

_bailout:
    free(full_path);
    return ret;
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

static void *ProbeGetBitStream(const DbProbe *probe) {
    int32_t decomp_dict_size = ProbeGetDecompDictSize(probe);
    return GenericPointerAdd(probe->buffer, kHeaderSize + decomp_dict_size);
}

static int64_t ProbeRecordStep2_0ReadCompressedOffset(const DbProbe *probe,
                                                      Position position,
                                                      const char *full_path) {
    FILE *db_file = GuardedFopen(full_path, "rb");
    if (db_file == NULL) return kFileSystemError;

    int bits_per_entry = ProbeGetBitsPerEntry(probe);
    int block_size = ProbeGetBlockSize(probe);
    int64_t block_offset = GetBlockOffset(position, bits_per_entry, block_size);

    // Seek to the entry in lookup that corresponds to block_offset.
    int32_t decomp_dict_size = ProbeGetDecompDictSize(probe);
    int64_t seek_length =
        kHeaderSize + decomp_dict_size + block_offset * sizeof(int64_t);
    int error = GuardedFseek(db_file, seek_length, SEEK_SET);
    if (error) return BailOutFclose(db_file, -1);

    // Read offset into the compressed bit stream.
    int64_t compressed_offset;
    error =
        GuardedFread(&compressed_offset, sizeof(int64_t), 1, db_file, false);
    if (error) return BailOutFclose(db_file, -1);

    // Finalize.
    error = GuardedFclose(db_file);
    if (error != 0) return kFileSystemError;

    int32_t lookup_table_size = ProbeGetLookupTableSize(probe);
    return compressed_offset + kHeaderSize + decomp_dict_size +
           lookup_table_size;
}

static int ProbeRecordStep2_1HandleFile(const DbProbe *probe,
                                        const char *full_path,
                                        int64_t compressed_offset) {
    // Open db_file
    int db_fd = GuardedOpen(full_path, O_RDONLY);
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
    void *buffer_blocks = ProbeGetBitStream(probe);
    error = GuardedGzread(compressed_stream, buffer_blocks,
                          kBlocksPerBuffer * block_size, true);
    if (error != 0) return BailOutGzclose(compressed_stream, error);

    // This will also close db_fd.
    error = GuardedGzclose(compressed_stream);
    return error;
}

// Loads the record of POSITION, assuming the requested record is in PROBE's
// buffer.
static uint64_t ProbeRecordStep3LoadRecord(const DbProbe *probe,
                                           Position position) {
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
