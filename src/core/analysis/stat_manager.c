/**
 * @file stat_manager.c
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Implementation of the Statistics Manager Module for game analysis.
 * @version 2.0.0
 * @date 2025-03-17
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

#include "core/analysis/stat_manager.h"

#include <assert.h>    // assert
#include <fcntl.h>     // open, O_RDONLY, O_WRONLY, O_CREAT
#include <stddef.h>    // NULL, size_t
#include <stdio.h>     // fprintf, stderr, SEEK_SET, fopen
#include <stdlib.h>    // calloc, free
#include <string.h>    // strlen, memset
#include <sys/stat.h>  // S_IRWXU, S_IRWXG, S_IRWXO
#include <zlib.h>      // gzread, gzFile, Z_NULL

#include "core/analysis/analysis.h"
#include "core/constants.h"
#include "core/data_structures/concurrent_bitset.h"
#include "core/misc.h"
#include "core/types/gamesman_types.h"
#include "libs/lz4_utils/lz4_utils.h"

static char *sandbox_path;

static char *SetupStatPath(ReadOnlyString game_name, int variant,
                           ReadOnlyString data_path);

static char *GetPathToTierAnalysis(Tier tier);
static char *GetPathToTierDiscoveryMap(Tier tier);
static char *GetPathTo(Tier tier, ReadOnlyString extension);

// -----------------------------------------------------------------------------

int StatManagerInit(ReadOnlyString game_name, int variant,
                    ReadOnlyString data_path) {
    if (sandbox_path != NULL) StatManagerFinalize();

    sandbox_path = SetupStatPath(game_name, variant, data_path);
    if (sandbox_path == NULL) return kMallocFailureError;

    return kNoError;
}

void StatManagerFinalize(void) {
    free(sandbox_path);
    sandbox_path = NULL;
}

int StatManagerGetStatus(Tier tier) {
    if (sandbox_path == NULL) {
        fprintf(stderr, "StatManagerSaveAnalysis: StatManager uninitialized\n");
        return kAnalysisTierCheckError;
    }

    char *filename = GetPathToTierAnalysis(tier);
    if (filename == NULL) return kAnalysisTierCheckError;

    FILE *stat_file = fopen(filename, "rb");
    free(filename);
    if (stat_file == NULL) return kAnalysisTierUnanalyzed;

    int error = GuardedFclose(stat_file);
    if (error != 0) return kAnalysisTierCheckError;

    return kAnalysisTierAnalyzed;
}

int StatManagerSaveAnalysis(Tier tier, const Analysis *analysis) {
    if (sandbox_path == NULL) {
        fprintf(stderr, "StatManagerSaveAnalysis: StatManager uninitialized\n");
        return kUseBeforeInitializationError;
    }

    char *filename = GetPathToTierAnalysis(tier);
    if (filename == NULL) return kMallocFailureError;
    mode_t mode = S_IRWXU | S_IRWXG | S_IRWXO;  // This sets permissions to 0777
    int stat_fd = open(filename, O_CREAT | O_WRONLY, mode);
    free(filename);
    if (stat_fd < 0) return kFileSystemError;

    int error = AnalysisWrite(analysis, stat_fd);
    if (error != 0) return error;

    error = GuardedClose(stat_fd);
    return error;
}

int StatManagerLoadAnalysis(Analysis *dest, Tier tier) {
    if (sandbox_path == NULL) {
        fprintf(stderr, "StatManagerLoadAnalysis: StatManager uninitialized\n");
        return kUseBeforeInitializationError;
    }

    char *filename = GetPathToTierAnalysis(tier);
    if (filename == NULL) return kMallocFailureError;

    int stat_fd = GuardedOpen(filename, O_RDONLY);
    free(filename);
    if (stat_fd < 0) return kFileSystemError;

    int error = AnalysisRead(dest, stat_fd);
    if (error != 0) return error;

    error = GuardedClose(stat_fd);
    return error;
}

int StatManagerLoadDiscoveryMap(Tier tier, int64_t size,
                                ConcurrentBitset **dest) {
    char *filename = GetPathToTierDiscoveryMap(tier);
    if (filename == NULL) return kMallocFailureError;

    ConcurrentBitset *s = ConcurrentBitsetCreate(size);
    if (s == NULL) {
        free(filename);
        return kMallocFailureError;
    }

    // Allocate deserialization buffer
    size_t buf_size = ConcurrentBitsetGetSerializedSize(s);
    void *buf = malloc(buf_size);
    if (buf == NULL) {
        free(filename);
        ConcurrentBitsetDestroy(s);
        return kMallocFailureError;
    }

    int64_t res = Lz4UtilsDecompressFile(filename, buf, buf_size);
    free(filename);

    // Handle error
    switch (res) {
        case -1:
            free(buf);
            return kFileSystemError;
        case -2:
            free(buf);
            return kMallocFailureError;
        case -3:
            free(buf);
            fprintf(stderr,
                    "StatManagerLoadDiscoveryMap: discovery map appears to be "
                    "corrupt for tier %" PRITier "\n",
                    tier);
            return kRuntimeError;
        case -4:
            free(buf);
            NotReached(
                "StatManagerLoadDiscoveryMap: not enough space for "
                "destination bit stream allocated, likely a bug\n");
            return kRuntimeError;
        default:
            break;
    }

    // Deserialize
    ConcurrentBitsetDeserialize(s, buf);
    free(buf);
    *dest = s;

    return kNoError;
}

int StatManagerSaveDiscoveryMap(const ConcurrentBitset *s, Tier tier) {
    char *filename = GetPathToTierDiscoveryMap(tier);
    if (filename == NULL) return kMallocFailureError;

    // Serialize the bitset
    size_t buf_size = ConcurrentBitsetGetSerializedSize(s);
    void *buf = malloc(buf_size);
    if (buf == NULL) return kMallocFailureError;
    ConcurrentBitsetSerialize(s, buf);

    int64_t res = Lz4UtilsCompressStream(buf, buf_size, 0, filename);
    free(buf);
    free(filename);
    switch (res) {
        case -1:
            return kIllegalArgumentError;
        case -2:
            return kMallocFailureError;
        case -3:
            return kFileSystemError;
        default:
            break;
    }

    return kNoError;
}

int StatManagerRemoveDiscoveryMap(Tier tier) {
    char *filename = GetPathToTierDiscoveryMap(tier);
    if (filename == NULL) return kMallocFailureError;

    int error = GuardedRemove(filename);
    free(filename);
    if (error != 0) return kFileSystemError;

    return kNoError;
}

// -----------------------------------------------------------------------------

static char *SetupStatPath(ReadOnlyString game_name, int variant,
                           ReadOnlyString data_path) {
    // path = "<data_path>/<game_name>/<variant>/analysis/"
    if (data_path == NULL) data_path = "data";
    static ConstantReadOnlyString kAnalysisDirName = "analysis";
    char *path = NULL;

    int path_length = (int)strlen(data_path) + 1;  // +1 for '/'.
    path_length += (int)strlen(game_name) + 1;
    path_length += kInt32Base10StringLengthMax + 1;
    path_length += (int)strlen(kAnalysisDirName) + 1;
    path = (char *)calloc((path_length + 1), sizeof(char));
    if (path == NULL) {
        fprintf(stderr, "SetupStatPath: failed to calloc path.\n");
        return NULL;
    }
    int actual_length = snprintf(path, path_length, "%s/%s/%d/%s/", data_path,
                                 game_name, variant, kAnalysisDirName);
    if (actual_length >= path_length) {
        fprintf(stderr,
                "SetupStatPath: (BUG) not enough space was allocated for "
                "path. Please check the implementation of this function.\n");
        free(path);
        return NULL;
    }
    if (MkdirRecursive(path) != 0) {
        fprintf(stderr,
                "SetupStatPath: failed to create path in the file system.\n");
        free(path);
        return NULL;
    }
    return path;
}

static char *GetPathToTierAnalysis(Tier tier) {
    // path = "<path>/<tier>.stat"
    static ConstantReadOnlyString kAnalysisExtension = ".stat";
    return GetPathTo(tier, kAnalysisExtension);
}

static char *GetPathToTierDiscoveryMap(Tier tier) {
    // path = "<path>/<tier>.map"
    static ConstantReadOnlyString kMapExtension = ".map.lz4";
    return GetPathTo(tier, kMapExtension);
}

static char *GetPathTo(Tier tier, ReadOnlyString extension) {
    // path = "<sandbox_path>/<tier><extension>"
    // file_name = "<tier><extension>"
    int file_name_length = kInt64Base10StringLengthMax + (int)strlen(extension);

    // +1 for '/', and +1 for '\0'.
    int path_length = (int)strlen(sandbox_path) + 1 + file_name_length + 1;
    char *path = (char *)calloc(path_length, sizeof(char));
    if (path == NULL) {
        fprintf(stderr, "GetPathToTierAnalysis: failed to calloc path.\n");
        return NULL;
    }

    char file_name[file_name_length + 1];  // +1 for '\0'.
    sprintf(file_name, "%" PRITier "%s", tier, extension);

    strcat(path, sandbox_path);
    strcat(path, file_name);
    return path;
}
