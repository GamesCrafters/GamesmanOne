/**
 * @file stat_manager.c
 * @author Robert Shi (robertyishi@berkeley.edu)
 *         GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Implementation of the Statistics Manager Module for game analysis.
 * @version 1.1
 * @date 2023-10-22
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
#include <inttypes.h>  // PRId64
#include <stddef.h>    // NULL
#include <stdio.h>     // fprintf, stderr, SEEK_SET, fopen
#include <stdlib.h>    // calloc, free
#include <string.h>    // strlen
#include <sys/stat.h>  // S_IRWXU, S_IRWXG, S_IRWXO
#include <zlib.h>      // gzread, gzFile, Z_NULL

#include "core/analysis/analysis.h"
#include "core/constants.h"
#include "core/gamesman_types.h"
#include "core/misc.h"
#include "libs/mgz/mgz.h"

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
    if (sandbox_path == NULL) return 1;

    return 0;
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
        return -1;
    }

    char *filename = GetPathToTierAnalysis(tier);
    if (filename == NULL) return 1;
    mode_t mode = S_IRWXU | S_IRWXG | S_IRWXO;  // This sets permissions to 0777
    int stat_fd = open(filename, O_CREAT | O_WRONLY, mode);
    free(filename);
    if (stat_fd < 0) return 2;

    int error = AnalysisWrite(analysis, stat_fd);
    if (error != 0) return error;

    error = GuardedClose(stat_fd);
    return error;
}

int StatManagerLoadAnalysis(Analysis *dest, Tier tier) {
    if (sandbox_path == NULL) {
        fprintf(stderr, "StatManagerLoadAnalysis: StatManager uninitialized\n");
        return -1;
    }

    char *filename = GetPathToTierAnalysis(tier);
    if (filename == NULL) return 1;

    int stat_fd = GuardedOpen(filename, O_RDONLY);
    free(filename);
    if (stat_fd < 0) return 2;

    int error = AnalysisRead(dest, stat_fd);
    if (error != 0) return error;

    error = GuardedClose(stat_fd);
    return error;
}

BitStream StatManagerLoadDiscoveryMap(Tier tier) {
    BitStream ret = {0};
    int error = -1;
    FILE *file = NULL;
    int fd = -1;
    gzFile gzfile = Z_NULL;

    char *filename = GetPathToTierDiscoveryMap(tier);
    if (filename == NULL) goto _bailout;

    file = fopen(filename, "rb");
    if (file == NULL) goto _bailout;

    fd = GuardedOpen(filename, O_RDONLY);
    if (fd < 0) goto _bailout;

    // Read BitStream size.
    error = GuardedFread(&ret.size, sizeof(ret.size), 1, file);
    if (error != 0) goto _bailout;

    // Initialize BitStream with size.
    error = BitStreamInit(&ret, ret.size);
    if (error != 0) goto _bailout;

    // Read compressed stream.
    error = GuardedLseek(fd, sizeof(ret.size), SEEK_SET);
    if (error != 0) goto _bailout;

    gzfile = GuardedGzdopen(fd, "rb");
    if (gzfile == Z_NULL) goto _bailout;
    fd = -1;  // Prevent double closing.

    error = GuardedGz64Read(gzfile, ret.stream, ret.num_bytes, false);
    if (error != 0) goto _bailout;

    // Success.
    error = 0;

_bailout:
    free(filename);
    if (file != NULL) GuardedFclose(file);
    if (fd >= 0) GuardedClose(fd);
    if (gzfile != Z_NULL) GuardedGzclose(gzfile);
    if (error != 0) {
        BitStreamDestroy(&ret);
        ret = (BitStream){0};
    }
    return ret;
}

int StatManagerSaveDiscoveryMap(const BitStream *stream, Tier tier) {
    mgz_res_t res =
        MgzParallelDeflate(stream->stream, stream->num_bytes, 9, 0, false);
    if (res.out == NULL) return 1;

    char *filename = GetPathToTierDiscoveryMap(tier);
    if (filename == NULL) return 1;

    FILE *file = GuardedFopen(filename, "wb");
    free(filename);
    if (file == NULL) return -1;

    int error = GuardedFwrite(&stream->size, sizeof(stream->size), 1, file);
    if (error != 0) return error;

    error = GuardedFwrite(res.out, 1, res.size, file);
    if (error != 0) return error;
    free(res.out);
    assert(res.lookup == NULL);

    error = GuardedFclose(file);
    if (error != 0) return error;

    return 0;
}

// -----------------------------------------------------------------------------

static char *SetupStatPath(ReadOnlyString game_name, int variant,
                           ReadOnlyString data_path) {
    // path = "<data_path>/<game_name>/<variant>/analysis/"
    if (data_path == NULL) data_path = "data";
    static ConstantReadOnlyString kAnalysisDirName = "analysis";
    char *path = NULL;

    int path_length = strlen(data_path) + 1;  // +1 for '/'.
    path_length += strlen(game_name) + 1;
    path_length += kInt32Base10StringLengthMax + 1;
    path_length += strlen(kAnalysisDirName) + 1;
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
    static ConstantReadOnlyString kMapExtension = ".map";
    return GetPathTo(tier, kMapExtension);
}

static char *GetPathTo(Tier tier, ReadOnlyString extension) {
    // path = "<sandbox_path>/<tier><extension>"
    // file_name = "<tier><extension>"
    int file_name_length = kInt64Base10StringLengthMax + strlen(extension);

    // +1 for '/', and +1 for '\0'.
    int path_length = strlen(sandbox_path) + 1 + file_name_length + 1;
    char *path = (char *)calloc(path_length, sizeof(char));
    if (path == NULL) {
        fprintf(stderr, "GetPathToTierAnalysis: failed to calloc path.\n");
        return NULL;
    }

    char file_name[file_name_length + 1];  // +1 for '\0'.
    sprintf(file_name, "%" PRId64 "%s", tier, extension);

    strcat(path, sandbox_path);
    strcat(path, file_name);
    return path;
}
