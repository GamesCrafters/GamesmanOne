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

#include <fcntl.h>  // IWYU pragma: no_include <sys/types.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include "core/analysis/analysis.h"
#include "core/concurrency.h"
#include "core/constants.h"
#include "core/data_structures/concurrent_bitset.h"
#include "core/gamesman_memory.h"
#include "core/misc.h"
#include "core/types/base.h"
#include "core/types/gamesman_error.h"
#include "libs/lz4_utils/lz4_utils.h"

static char *sandbox_path;

static char *SetupStatPath(ReadOnlyString game_name, int variant,
                           ReadOnlyString data_path);

static char *GetPathToTierAnalysis(Tier tier);
static char *GetPathToTierDiscoveryMap(Tier tier);
static char *GetPathTo(Tier tier, ReadOnlyString extension);

static int ReportLz4UtilsError(Lz4UtilsStatus status);

// -----------------------------------------------------------------------------

int StatManagerInit(ReadOnlyString game_name, int variant,
                    ReadOnlyString data_path) {
    if (sandbox_path != NULL) StatManagerFinalize();

    sandbox_path = SetupStatPath(game_name, variant, data_path);
    if (sandbox_path == NULL) return kMallocFailureError;

    return kNoError;
}

void StatManagerFinalize(void) {
    GamesmanFree(sandbox_path);
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
    GamesmanFree(filename);
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
    GamesmanFree(filename);
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
    GamesmanFree(filename);
    if (stat_fd < 0) return kFileSystemError;

    int error = AnalysisRead(dest, stat_fd);
    if (error != 0) return error;

    error = GuardedClose(stat_fd);
    return error;
}

int StatManagerLoadDiscoveryMap(Tier tier, int64_t size,
                                GamesmanAllocator *allocator,
                                ConcurrentBitset **dest) {
    int error = kNoError;
    char buf[2][BUFSIZ];  // Double-buffer
    char *filename = GetPathToTierDiscoveryMap(tier);
    ConcurrentBitset *s = ConcurrentBitsetCreateAllocator(size, allocator);
    Lz4UtilsInStream *lz4_istream = NULL;
    if (filename == NULL || s == NULL) {
        error = kMallocFailureError;
        goto _bailout;
    }
    if (!FileExists(filename)) {
        error = kFileSystemError;
        goto _bailout;
    }

    lz4_istream = Lz4UtilsInStreamCreate(filename);
    if (!lz4_istream) {
        error = kRuntimeError;
        goto _bailout;
    }

    size_t total_bytes = ConcurrentBitsetGetSerializedSize(s);
    size_t deserialized = 0;
    int current_buf = 0;

    // Prime the pipeline by reading the first chunk
    size_t bytes_read = 0;
    Lz4UtilsInStreamRun(lz4_istream, buf[current_buf], BUFSIZ, &bytes_read);
    // TODO: add LZ4 status checks

    PRAGMA_OMP(parallel) {
        PRAGMA_OMP(single) {
            // Loop until deserialized all expected bytes or hit an EOF/error
            while (deserialized < total_bytes && bytes_read > 0) {
                size_t deserialize_step = 0;
                size_t next_bytes_read = 0;

                // Task A: Deserialize the buffer we just read
                PRAGMA_OMP(task shared(deserialize_step) firstprivate(
                    current_buf, bytes_read, deserialized)) {
                    deserialize_step = ConcurrentBitsetDeserializeStreaming(
                        s, deserialized, buf[current_buf], bytes_read);
                }

                // Task B: Eagerly decompress the next chunk into the alternate
                // buffer
                PRAGMA_OMP(task shared(next_bytes_read)
                               firstprivate(current_buf)) {
                    Lz4UtilsInStreamRun(lz4_istream, buf[1 - current_buf],
                                        BUFSIZ, &next_bytes_read);
                }

                // Both tasks must complete before moving forward
                PRAGMA_OMP(taskwait)

                if (deserialize_step == 0) {
                    error = kRuntimeError;
                    break;
                }

                deserialized += deserialize_step;
                bytes_read = next_bytes_read;
                current_buf = 1 - current_buf;
            }
        }
    }

    // Handle a sudden failure in the deserialize step
    if (error == kRuntimeError) {
        fprintf(stderr,
                "StatManagerLoadDiscoveryMap: "
                "ConcurrentBitsetDeserializeStreaming unexpectedly "
                "returned 0\n");
        goto _bailout;
    }

    // Check for premature End-Of-File
    if (deserialized < total_bytes) {
        fprintf(stderr,
                "StatManagerLoadDiscoveryMap: premature end of file "
                "for tier %" PRITier "\n",
                tier);
        error = kRuntimeError;
        goto _bailout;
    }

    // Success.
    *dest = s;

_bailout:
    GamesmanFree(filename);
    if (error != kNoError) ConcurrentBitsetDestroy(s);
    Lz4UtilsInStreamClose(lz4_istream);

    return error;
}

int StatManagerSaveDiscoveryMap(const ConcurrentBitset *s, Tier tier) {
    char *filename = GetPathToTierDiscoveryMap(tier);
    if (filename == NULL) return kMallocFailureError;

    Lz4UtilsOutStream *const lz4_ostream = Lz4UtilsOutStreamCreate(filename, 0);
    GamesmanFree(filename);
    if (lz4_ostream == NULL) return kRuntimeError;

    char buf[2][BUFSIZ];
    int current_buf = 0;
    size_t bytes_serialized = 0;
    Lz4UtilsStatus status = LZ4_UTILS_SUCCESS;
    size_t compressed_bytes = 0;
    size_t step = ConcurrentBitsetSerializeStreaming(s, bytes_serialized,
                                                     buf[current_buf], BUFSIZ);
    PRAGMA_OMP(parallel) {
        PRAGMA_OMP(single) {
            while (step > 0) {
                bytes_serialized += step;
                // Task A: Compress the buffer we just filled
                PRAGMA_OMP(task shared(status, compressed_bytes)
                               firstprivate(current_buf, step))
                status = Lz4UtilsOutStreamRun(lz4_ostream, buf[current_buf],
                                              step, &compressed_bytes);

                // Task B: Eagerly serialize the next chunk into the alternate
                // buffer
                size_t next_step = 0;
                PRAGMA_OMP(task shared(next_step)
                               firstprivate(current_buf, bytes_serialized)) {
                    next_step = ConcurrentBitsetSerializeStreaming(
                        s, bytes_serialized, buf[1 - current_buf], BUFSIZ);
                }

                // Both tasks must complete before moving forward
                PRAGMA_OMP(taskwait)
                if (status != LZ4_UTILS_SUCCESS)
                    break;  // Break if compression failed
                step = next_step;
                current_buf = 1 - current_buf;
            }
        }
    }

    Lz4UtilsStatus close_status = Lz4UtilsOutStreamClose(lz4_ostream, NULL);
    int error = ReportLz4UtilsError(close_status);
    if (error != kNoError) return error;

    return ReportLz4UtilsError(status);
}

int StatManagerRemoveDiscoveryMap(Tier tier) {
    char *filename = GetPathToTierDiscoveryMap(tier);
    if (filename == NULL) return kMallocFailureError;

    int error = GuardedRemove(filename);
    GamesmanFree(filename);
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
    path = (char *)GamesmanCallocWhole((path_length + 1), sizeof(char));
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
        GamesmanFree(path);
        return NULL;
    }
    if (MkdirRecursive(path) != 0) {
        fprintf(stderr,
                "SetupStatPath: failed to create path in the file system.\n");
        GamesmanFree(path);
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
    char *path = (char *)GamesmanCallocWhole(path_length, sizeof(char));
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

static int ReportLz4UtilsError(Lz4UtilsStatus status) {
    switch (status) {
        case LZ4_UTILS_SUCCESS:
            return kNoError;
        case LZ4_UTILS_ERR_INVALID_PARAM:
        case LZ4_UTILS_ERR_INSUFFICIENT_BUF:
            return kIllegalArgumentError;
        case LZ4_UTILS_ERR_OOM:
            return kMallocFailureError;
        case LZ4_UTILS_ERR_IO:
            return kFileSystemError;
        default:
            return kRuntimeError;
    }
}
