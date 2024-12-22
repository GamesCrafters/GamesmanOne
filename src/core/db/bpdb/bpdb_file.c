/**
 * @file bpdb_file.h
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Bit-Perfect Database file utilities.
 * @version 1.2.2
 * @date 2024-12-22
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

#include "core/db/bpdb/bpdb_file.h"

#include <stddef.h>  // NULL
#include <stdio.h>   // fprintf, stderr, sprintf
#include <stdlib.h>  // calloc, free
#include <string.h>  // strlen, strcat

#include "core/constants.h"
#include "core/db/bpdb/bparray.h"
#include "core/misc.h"
#include "core/types/gamesman_types.h"
#include "libs/mgz/mgz.h"

static const int kMgzMinBlockSize = 1 << 14;  // 16 KiB as specified by mgz.
static const int kMgzCompressionLevel = 9;    // Maximum compression.
static const bool kMgzLookupNeeded = true;

static mgz_res_t FlushStep0MgzCompress(BpdbFileHeader *header,
                                       const BpArray *stream);
static int FlushStep1WriteToFile(ReadOnlyString full_path,
                                 const BpdbFileHeader *header,
                                 const int32_t *decomp_dict, mgz_res_t result);

// -----------------------------------------------------------------------------

char *BpdbFileGetFullPath(ConstantReadOnlyString sandbox_path, Tier tier,
                          GetTierNameFunc GetTierName) {
    // Full path: "<path>/<file_name>.bpdb".
    static ConstantReadOnlyString kBpdbExtension = ".bpdb";
    int path_length = (int)(strlen(sandbox_path) + 1 + kDbFileNameLengthMax +
                            strlen(kBpdbExtension) + 1);
    char *full_path = (char *)malloc(path_length * sizeof(char));
    if (full_path == NULL) {
        fprintf(stderr, "GetFullPathToFile: failed to malloc full_path.\n");
        return NULL;
    }

    int count = sprintf(full_path, "%s/", sandbox_path);
    if (GetTierName != NULL) {
        GetTierName(tier, full_path + count);
    } else {
        sprintf(full_path + count, "%" PRITier, tier);
    }
    strcat(full_path, kBpdbExtension);

    return full_path;
}

char *BpdbFileGetFullPathToFinishFlag(ConstantReadOnlyString sandbox_path) {
    // Full path: "<path>/.finish".
    static ConstantReadOnlyString kFinishFlagFileName = ".finish";
    int path_length =
        (int)(strlen(sandbox_path) + strlen(kFinishFlagFileName) + 2);
    char *full_path = (char *)malloc(path_length * sizeof(char));
    if (full_path == NULL) {
        fprintf(
            stderr,
            "BpdbFileGetFullPathToFinishFlag: failed to malloc full_path.\n");
        return NULL;
    }

    sprintf(full_path, "%s/%s", sandbox_path, kFinishFlagFileName);
    return full_path;
}

int BpdbFileFlush(ReadOnlyString full_path, const BpArray *records) {
    int32_t num_unique_values = BpArrayGetNumUniqueValues(records);

    BpdbFileHeader header;
    header.decomp_dict_meta.size = num_unique_values * (int32_t)sizeof(int32_t);
    header.stream_meta = records->meta;

    // Compress stream using mgz
    mgz_res_t result = FlushStep0MgzCompress(&header, records);
    if (result.out == NULL) {
        fprintf(stderr,
                "BpdbLiteFlushSolvingTier: failed to compress records using "
                "mgz.\n");
        return kMallocFailureError;
    }

    // Write compressed data to file.
    const int32_t *decomp_dict = BpArrayGetDecompDict(records);
    return FlushStep1WriteToFile(full_path, &header, decomp_dict, result);
}

int64_t BpdbFileGetBlockSize(int bits_per_entry) {
    // Makes sure that each block contains an integer number of entries.
    return NextMultiple(kMgzMinBlockSize,
                        bits_per_entry * (int64_t)sizeof(uint64_t));
}

int BpdbFileGetTierStatus(ConstantReadOnlyString sandbox_path, Tier tier,
                          GetTierNameFunc GetTierName) {
    char *filename = BpdbFileGetFullPath(sandbox_path, tier, GetTierName);
    if (filename == NULL) return kDbTierStatusCheckError;

    FILE *db_file = fopen(filename, "rb");
    free(filename);
    if (db_file == NULL) return kDbTierStatusMissing;

    int error = GuardedFclose(db_file);
    if (error != 0) return kDbTierStatusCheckError;

    return kDbTierStatusSolved;
}

// -----------------------------------------------------------------------------

static mgz_res_t FlushStep0MgzCompress(BpdbFileHeader *header,
                                       const BpArray *stream) {
    int64_t block_size = BpdbFileGetBlockSize(stream->meta.bits_per_entry);
    header->lookup_meta.block_size = block_size;
    mgz_res_t result =
        MgzParallelDeflate(stream->stream, stream->meta.stream_length,
                           kMgzCompressionLevel, block_size, kMgzLookupNeeded);
    if (result.out == NULL) return result;

    header->lookup_meta.size = result.num_blocks * (int64_t)sizeof(int64_t);
    return result;
}

static int FlushStep1WriteToFile(ReadOnlyString full_path,
                                 const BpdbFileHeader *header,
                                 const int32_t *decomp_dict, mgz_res_t result) {
    // Write header to file.
    FILE *db_file = GuardedFopen(full_path, "wb");
    if (db_file == NULL) return kFileSystemError;

    int error = GuardedFwrite(header, sizeof(*header), 1, db_file);
    if (error != 0) return BailOutFclose(db_file, error);

    // Write decomp dict.
    error =
        GuardedFwrite(decomp_dict, 1, header->decomp_dict_meta.size, db_file);
    if (error != 0) return BailOutFclose(db_file, error);

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
