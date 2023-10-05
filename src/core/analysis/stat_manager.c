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
#include "core/gamesman_types.h"
#include "core/misc.h"
#include "libs/mgz/mgz.h"

static char *sandbox_path;

static char *SetupStatPath(ReadOnlyString game_name, int variant);

static char *GetPathToTierAnalysis(Tier tier);
static char *GetPathToTierDiscoveryMap(Tier tier);
static char *GetPathTo(Tier tier, ReadOnlyString extension);

int StatManagerInit(ReadOnlyString game_name, int variant) {
    if (sandbox_path != NULL) StatManagerFinalize();

    sandbox_path = SetupStatPath(game_name, variant);
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
    static const BitStream kInvalidStream;
    BitStream ret;

    char *filename = GetPathToTierDiscoveryMap(tier);
    if (filename == NULL) return kInvalidStream;

    FILE *file = fopen(filename, "rb");
    if (file == NULL) {
        free(filename);
        return kInvalidStream;
    }

    int fd = GuardedOpen(filename, O_RDONLY);
    free(filename);
    if (fd < 0) return kInvalidStream;

    // Read BitStream size.
    int error = GuardedFread(&ret.size, sizeof(ret.size), 1, file);
    if (error != 0) return kInvalidStream;

    error = GuardedFclose(file);
    if (error != 0) return kInvalidStream;

    // Initialize BitStream with size.
    error = BitStreamInit(&ret, ret.size);
    if (error != 0) return kInvalidStream;

    // Read compressed stream.
    error = GuardedLseek(fd, sizeof(ret.size), SEEK_SET);
    if (error != 0) return kInvalidStream;

    gzFile gzfile = GuardedGzdopen(fd, "rb");
    if (gzfile == Z_NULL) return kInvalidStream;

    error = GuardedGz64Read(gzfile, ret.stream, ret.num_bytes, false);
    if (error != 0) {
        BitStreamDestroy(&ret);
        GuardedGzclose(gzfile);
        return kInvalidStream;
    }

    GuardedGzclose(gzfile);
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

static char *SetupStatPath(ReadOnlyString game_name, int variant) {
    // path = "data/<game_name>/<variant>/analysis/"
    static ConstantReadOnlyString kDataPath = "data";
    static ConstantReadOnlyString kAnalysisDirName = "analysis";
    char *path = NULL;

    int path_length = strlen(kDataPath) + 1;  // +1 for '/'.
    path_length += strlen(game_name) + 1;
    path_length += kInt32Base10StringLengthMax + 1;
    path_length += strlen(kAnalysisDirName) + 1;
    path = (char *)calloc((path_length + 1), sizeof(char));
    if (path == NULL) {
        fprintf(stderr, "SetupStatPath: failed to calloc path.\n");
        return NULL;
    }
    int actual_length = snprintf(path, path_length, "%s/%s/%d/%s/", kDataPath,
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
