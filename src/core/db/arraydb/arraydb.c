/**
 * @file arraydb.c
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Simple array database which stores value-remoteness pairs in a 16-bit
 * record array.
 * @details The in-memory database is an uncompressed 16-bit record array of
 * length equal to the size of the given tier. The array is block-compressed
 * using LZMA provided by the XZ Utils library wrapped in the XZRA (XZ with
 * random access) library.
 * @version 1.1.2
 * @date 2025-04-26
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

#include "core/db/arraydb/arraydb.h"

#include <assert.h>   // assert
#include <stdbool.h>  // bool, true, false
#include <stddef.h>   // NULL, size_t
#include <stdint.h>   // uint64_t, int64_t
#include <stdio.h>    // fprintf, stderr
#include <string.h>   // strcpy

#ifdef _OPENMP
#include <omp.h>
#endif  // _OPENMP

#include "core/concurrency.h"
#include "core/constants.h"
#include "core/db/arraydb/record.h"
#include "core/db/arraydb/record_array.h"
#include "core/gamesman_memory.h"
#include "core/misc.h"
#include "core/types/gamesman_types.h"
#include "libs/lz4_utils/lz4_utils.h"
#include "libs/xzra/xzra.h"

// DB API

static int ArrayDbInit(ReadOnlyString game_name, int variant,
                       ReadOnlyString path, GetTierNameFunc GetTierName,
                       void *aux);
static void ArrayDbFinalize(void);

static int ArrayDbCreateSolvingTier(Tier tier, int64_t size);
static int ArrayDbCreateConcurrentSolvingTier(Tier tier, int64_t size);
static int ArrayDbFlushSolvingTier(void *aux);
static int ArrayDbFreeSolvingTier(void);

static int ArrayDbSetGameSolved(void);
static int ArrayDbSetValue(Position position, Value value);
static int ArrayDbSetRemoteness(Position position, int remoteness);
static int ArrayDbSetValueRemoteness(Position position, Value value,
                                     int remoteness);
static int ArrayDbMaximizeValueRemoteness(Position position, Value value,
                                          int remoteness,
                                          int (*compare)(Value v1, int r1,
                                                         Value v2, int r2));
static Value ArrayDbGetValue(Position position);
static int ArrayDbGetRemoteness(Position position);
static bool ArrayDbCheckpointExists(Tier tier);
static int ArrayDbCheckpointSave(const void *status, size_t status_size);
static int ArrayDbCheckpointLoad(Tier tier, int64_t size, void *status,
                                 size_t status_size);
static int ArrayDbCheckpointRemove(Tier tier);

static size_t ArrayDbTierMemUsage(Tier tier, int64_t size);
static int ArrayDbLoadTier(Tier tier, int64_t size);
static int ArrayDbUnloadTier(Tier tier);
static bool ArrayDbIsTierLoaded(Tier tier);
static Value ArrayDbGetValueFromLoaded(Tier tier, Position position);
static int ArrayDbGetRemotenessFromLoaded(Tier tier, Position position);

static int ArrayDbProbeInit(DbProbe *probe);
static int ArrayDbProbeDestroy(DbProbe *probe);
static Value ArrayDbProbeValue(DbProbe *probe, TierPosition tier_position);
static int ArrayDbProbeRemoteness(DbProbe *probe, TierPosition tier_position);
static int ArrayDbTierStatus(Tier tier);
static int ArrayDbGameStatus(void);

const Database kArrayDb = {
    .name = "arraydb",
    .formal_name = "Array Database",

    .Init = ArrayDbInit,
    .Finalize = ArrayDbFinalize,

    // Solving
    .CreateSolvingTier = ArrayDbCreateSolvingTier,
    .CreateConcurrentSolvingTier = ArrayDbCreateConcurrentSolvingTier,
    .FlushSolvingTier = ArrayDbFlushSolvingTier,
    .FreeSolvingTier = ArrayDbFreeSolvingTier,

    .SetGameSolved = ArrayDbSetGameSolved,
    .SetValue = ArrayDbSetValue,
    .SetRemoteness = ArrayDbSetRemoteness,
    .SetValueRemoteness = ArrayDbSetValueRemoteness,
    .MaximizeValueRemoteness = ArrayDbMaximizeValueRemoteness,
    .GetValue = ArrayDbGetValue,
    .GetRemoteness = ArrayDbGetRemoteness,
    .CheckpointExists = ArrayDbCheckpointExists,
    .CheckpointSave = ArrayDbCheckpointSave,
    .CheckpointLoad = ArrayDbCheckpointLoad,

    // Loading
    .TierMemUsage = ArrayDbTierMemUsage,
    .LoadTier = ArrayDbLoadTier,
    .UnloadTier = ArrayDbUnloadTier,
    .IsTierLoaded = ArrayDbIsTierLoaded,
    .GetValueFromLoaded = ArrayDbGetValueFromLoaded,
    .GetRemotenessFromLoaded = ArrayDbGetRemotenessFromLoaded,
    .CheckpointRemove = ArrayDbCheckpointRemove,

    // Probing
    .ProbeInit = ArrayDbProbeInit,
    .ProbeDestroy = ArrayDbProbeDestroy,
    .ProbeValue = ArrayDbProbeValue,
    .ProbeRemoteness = ArrayDbProbeRemoteness,
    .TierStatus = ArrayDbTierStatus,
    .GameStatus = ArrayDbGameStatus,
};

// Types

typedef struct {
    XzraFile *file;
    bool init;
} AdbProbeInternal;

// Constants

const int kArrayDbRecordSize = sizeof(Record);
const ArrayDbOptions kArrayDbOptionsInit = {
    .block_size = 1 << 20,         // 1 MiB.
    .compression_level = 6,        // LZMA level 6.
    .extreme_compression = false,  // Extreme compression disabled.
};
static const int kDefaultLz4Level = 0;  //

// Global options

static int block_size;  // For XZ compression.
static int lzma_level;
static bool enable_extreme_compression;

// Global state variables

static char current_game_name[kGameNameLengthMax + 1];
static int current_variant;
static GetTierNameFunc CurrentGetTierName;
static char *sandbox_path;
static Tier current_tier;
static RecordArray *records;
static TierToPtrChainedHashMap loaded_tiers;

static int ArrayDbInit(ReadOnlyString game_name, int variant,
                       ReadOnlyString path, GetTierNameFunc GetTierName,
                       void *aux) {
    const ArrayDbOptions *options = (ArrayDbOptions *)aux;
    if (options == NULL) options = &kArrayDbOptionsInit;
    block_size = options->block_size;
    lzma_level = options->compression_level;
    enable_extreme_compression = options->extreme_compression;

    assert(sandbox_path == NULL);
    sandbox_path = (char *)GamesmanMalloc((strlen(path) + 1) * sizeof(char));
    if (sandbox_path == NULL) {
        fprintf(stderr, "ArrayDbInit: failed to malloc path.\n");
        return kMallocFailureError;
    }

    strcpy(sandbox_path, path);
    strcpy(current_game_name, game_name);
    current_variant = variant;
    CurrentGetTierName = GetTierName;
    current_tier = kIllegalTier;
    TierToPtrChainedHashMapInit(&loaded_tiers, 0.5);

    return kNoError;
}

static void ArrayDbFinalize(void) {
    GamesmanFree(sandbox_path);
    sandbox_path = NULL;

    // Free all loaded records, including the solving tier.
    TierToPtrChainedHashMapIterator it =
        TierToPtrChainedHashMapBegin(&loaded_tiers);
    while (TierToPtrChainedHashMapIteratorIsValid(&it)) {
        RecordArrayDestroy(
            (RecordArray *)TierToPtrChainedHashMapIteratorValue(&it));
    }
    TierToPtrChainedHashMapDestroy(&loaded_tiers);
    records = NULL;
}

static int ArrayDbCreateSolvingTier(Tier tier, int64_t size) {
    if (current_tier != kIllegalTier) {
        fprintf(stderr,
                "ArrayDbCreateSolvingTier: failed to create solving tier due "
                "to an existing solving tier\n");
        return kRuntimeError;
    }

    // Initialize the solving tier's record array.
    records = RecordArrayCreate(size);
    if (records == NULL) return kMallocFailureError;
    if (!TierToPtrChainedHashMapSet(&loaded_tiers, tier, records)) {
        RecordArrayDestroy(records);
        records = NULL;
        return kMallocFailureError;
    }
    current_tier = tier;

    return kNoError;
}

static int ArrayDbCreateConcurrentSolvingTier(Tier tier, int64_t size) {
#ifdef _OPENMP

#else
    return ArrayDbCreateSolvingTier(tier, size);
#endif
}

/**
 * @brief Returns the full path to the DB file for the given tier. The user is
 * responsible for freeing the pointer returned by this function. Returns
 * NULL on failure.
 */
static char *GetFullPathToFile(Tier tier, GetTierNameFunc GetTierName) {
    // Full path: "<path>/<file_name><ext>", +2 for '/' and '\0'.
    static const char extension[] = ".adb.xz";
    char *full_path = (char *)GamesmanCallocWhole(
        (strlen(sandbox_path) + kDbFileNameLengthMax + sizeof(extension) + 2),
        sizeof(char));
    if (full_path == NULL) {
        fprintf(stderr, "GetFullPathToFile: failed to calloc full_path.\n");
        return NULL;
    }

    int count = sprintf(full_path, "%s/", sandbox_path);
    if (GetTierName != NULL) {
        GetTierName(tier, full_path + count);
    } else {
        sprintf(full_path + count, "%" PRITier, tier);
    }
    strcat(full_path, extension);

    return full_path;
}

static char *GetFullPathPlusExtension(Tier tier, GetTierNameFunc GetTierName,
                                      ReadOnlyString extension) {
    char *full_path_to_tier_file = GetFullPathToFile(tier, GetTierName);
    if (full_path_to_tier_file == NULL) return NULL;

    size_t length = strlen(full_path_to_tier_file);
    char *full_path = (char *)GamesmanCallocWhole(
        length + strlen(extension) + 1, sizeof(char));
    if (full_path == NULL) {
        GamesmanFree(full_path_to_tier_file);
        return NULL;
    }
    strcat(full_path, full_path_to_tier_file);
    GamesmanFree(full_path_to_tier_file);
    strcat(full_path, extension);

    return full_path;
}

static char *GetFullPathToTempFile(Tier tier, GetTierNameFunc GetTierName) {
    return GetFullPathPlusExtension(tier, GetTierName, ".tmp");
}

static char *GetFullPathToCheckpoint(Tier tier, GetTierNameFunc GetTierName) {
    return GetFullPathPlusExtension(tier, GetTierName, ".chk");
}

static char *GetFullPathToTempCheckpoint(Tier tier,
                                         GetTierNameFunc GetTierName) {
    return GetFullPathPlusExtension(tier, GetTierName, ".chk.tmp");
}

static char *GetFullPathToFinishFlag(void) {
    // Full path: "<path>/.finish", +2 for '/' and '\0'.
    static const char finish_flag_name[] = ".finish";
    char *full_path = (char *)GamesmanCallocWhole(
        (strlen(sandbox_path) + sizeof(finish_flag_name) + 2), sizeof(char));
    if (full_path == NULL) {
        fprintf(stderr,
                "GetFullPathToFinishFlag: failed to calloc full_path.\n");
        return NULL;
    }

    sprintf(full_path, "%s/%s", sandbox_path, finish_flag_name);
    return full_path;
}

static int ArrayDbFlushSolvingTier(void *aux) {
    (void)aux;  // Unused.

    // Create db file.
    int error = kNoError;
    char *full_path = GetFullPathToFile(current_tier, CurrentGetTierName);
    char *tmp_full_path =
        GetFullPathToTempFile(current_tier, CurrentGetTierName);
    if (full_path == NULL || tmp_full_path == NULL) {
        error = kMallocFailureError;
        goto _bailout;
    }

    // First compress to a temp file.
    int64_t compressed_size = XzraCompressMem(
        tmp_full_path, block_size, lzma_level, enable_extreme_compression,
        ConcurrencyGetOmpNumThreads(), RecordArrayGetReadOnlyData(records),
        RecordArrayGetRawSize(records));
    switch (compressed_size) {
        case -2:
            error = kFileSystemError;
            goto _bailout;
        case -3:
            error = kRuntimeError;
            goto _bailout;
    }

    // If successful, rename the temp file into the desired tier DB name.
    int rename_error = GuardedRename(tmp_full_path, full_path);
    if (rename_error) {
        error = kFileSystemError;
        goto _bailout;
    }

_bailout:
    GamesmanFree(full_path);
    GamesmanFree(tmp_full_path);

    return error;
}

static int ArrayDbFreeSolvingTier(void) {
    RecordArrayDestroy(records);
    records = NULL;
    TierToPtrChainedHashMapRemove(&loaded_tiers, current_tier);
    current_tier = kIllegalTier;

    return kNoError;
}

static int ArrayDbSetGameSolved(void) {
    char *flag_filename = GetFullPathToFinishFlag();
    if (flag_filename == NULL) return kMallocFailureError;

    FILE *flag_file = GuardedFopen(flag_filename, "w");
    GamesmanFree(flag_filename);
    if (flag_file == NULL) return kFileSystemError;

    int error = GuardedFclose(flag_file);
    if (error != 0) return kFileSystemError;

    return kNoError;
}

static int ArrayDbSetValue(Position position, Value value) {
    RecordArraySetValue(records, position, value);

    return kNoError;
}

static int ArrayDbSetRemoteness(Position position, int remoteness) {
    RecordArraySetRemoteness(records, position, remoteness);

    return kNoError;
}

static int ArrayDbSetValueRemoteness(Position position, Value value,
                                     int remoteness) {
    RecordArraySetValueRemoteness(records, position, value, remoteness);

    return kNoError;
}

static int ArrayDbMaximizeValueRemoteness(Position position, Value value,
                                          int remoteness,
                                          int (*compare)(Value v1, int r1,
                                                         Value v2, int r2)) {
    // TODO
    ;
}

static Value ArrayDbGetValue(Position position) {
    return RecordArrayGetValue(records, position);
}

static int ArrayDbGetRemoteness(Position position) {
    return RecordArrayGetRemoteness(records, position);
}

bool ArrayDbCheckpointExists(Tier tier) {
    char *full_path = GetFullPathToCheckpoint(tier, CurrentGetTierName);
    bool ret = full_path && FileExists(full_path);
    GamesmanFree(full_path);

    return ret;
}

int ArrayDbCheckpointSave(const void *status, size_t status_size) {
    int error = kNoError;
    char *full_path = GetFullPathToCheckpoint(current_tier, CurrentGetTierName);
    char *tmp_full_path =
        GetFullPathToTempCheckpoint(current_tier, CurrentGetTierName);
    if (full_path == NULL || tmp_full_path == NULL) {
        error = kMallocFailureError;
        goto _bailout;
    }

    const void *inputs[] = {RecordArrayGetReadOnlyData(records), status};
    const size_t input_sizes[] = {RecordArrayGetRawSize(records), status_size};
    int64_t compressed_size = Lz4UtilsCompressStreams(
        inputs, input_sizes, 2, kDefaultLz4Level, tmp_full_path);
    switch (compressed_size) {
        case -1:
            NotReached("ArrayDbCheckpointSave: (BUG) malformed input array(s)");
            break;
        case -2:
            error = kMallocFailureError;
            goto _bailout;
        case -3:
            error = kFileSystemError;
            goto _bailout;
    }

    // If successful, rename the temp file into the desired checkpoint filename.
    int rename_error = GuardedRename(tmp_full_path, full_path);
    if (rename_error) {
        error = kFileSystemError;
        goto _bailout;
    }

_bailout:
    GamesmanFree(full_path);
    GamesmanFree(tmp_full_path);

    return error;
}

int ArrayDbCheckpointLoad(Tier tier, int64_t size, void *status,
                          size_t status_size) {
    if (current_tier != kIllegalTier) {
        fprintf(stderr,
                "ArrayDbCheckpointLoad: failed to load solving tier checkpoint "
                "due to an existing solving tier\n");
        return kRuntimeError;
    }

    // Initialize the solving tier's record array.
    records = RecordArrayCreate(size);
    if (records == NULL) return kMallocFailureError;

    // Get full path to the checkpoint file.
    char *full_path = GetFullPathToCheckpoint(tier, CurrentGetTierName);
    if (full_path == NULL) {
        RecordArrayDestroy(records);
        return kMallocFailureError;
    }

    // Decompress the checkpoint file into the record array and status.
    void *out_buffers[] = {RecordArrayGetData(records), status};
    size_t out_sizes[] = {RecordArrayGetRawSize(records), status_size};
    int64_t decomp_size =
        Lz4UtilsDecompressFileMultistream(full_path, out_buffers, out_sizes, 2);
    GamesmanFree(full_path);
    if (decomp_size < 0) {
        RecordArrayDestroy(records);
        return kRuntimeError;
    }

    // Add the solving tier's index to the map.
    if (!TierToPtrChainedHashMapSet(&loaded_tiers, tier, records)) {
        RecordArrayDestroy(records);
        return kMallocFailureError;
    }
    current_tier = tier;

    return kNoError;
}

static int ArrayDbCheckpointRemove(Tier tier) {
    char *full_path = GetFullPathToCheckpoint(tier, CurrentGetTierName);
    int error = GuardedRemove(full_path);
    GamesmanFree(full_path);
    if (error != 0) return kFileSystemError;

    return kNoError;
}

static size_t ArrayDbTierMemUsage(Tier tier, int64_t size) {
    (void)tier;
    return (size_t)size * 2;
}

static int ArrayDbLoadTier(Tier tier, int64_t size) {
    RecordArray *load = RecordArrayCreate(size);
    if (load == NULL) return kMallocFailureError;

    char *full_path = GetFullPathToFile(tier, CurrentGetTierName);
    if (full_path == NULL) {
        RecordArrayDestroy(load);
        return kMallocFailureError;
    }

    uint64_t mem = XzraDecompressionMemUsage(block_size, lzma_level,
                                             enable_extreme_compression,
                                             ConcurrencyGetOmpNumThreads());
    int64_t decomp_size =
        XzraDecompressFile(RecordArrayGetData(load), size * kArrayDbRecordSize,
                           ConcurrencyGetOmpNumThreads(), mem, full_path);
    GamesmanFree(full_path);
    if (decomp_size < 0) {
        RecordArrayDestroy(load);
        return kRuntimeError;
    }

    if (!TierToPtrChainedHashMapSet(&loaded_tiers, tier, load)) {
        RecordArrayDestroy(load);
        return kMallocFailureError;
    }

    return kNoError;
}

static int ArrayDbUnloadTier(Tier tier) {
    TierToPtrChainedHashMapIterator it =
        TierToPtrChainedHashMapGet(&loaded_tiers, tier);
    if (TierToPtrChainedHashMapIteratorIsValid(&it)) {
        RecordArrayDestroy(
            (RecordArray *)TierToPtrChainedHashMapIteratorValue(&it));
        TierToPtrChainedHashMapRemove(&loaded_tiers, tier);
    }

    return kNoError;
}

static bool ArrayDbIsTierLoaded(Tier tier) {
    TierToPtrChainedHashMapIterator it =
        TierToPtrChainedHashMapGet(&loaded_tiers, tier);

    return TierToPtrChainedHashMapIteratorIsValid(&it);
}

static RecordArray *GetRecordsFromLoaded(Tier tier) {
    TierToPtrChainedHashMapIterator it =
        TierToPtrChainedHashMapGet(&loaded_tiers, tier);
    if (!TierToPtrChainedHashMapIteratorIsValid(&it)) {
        return NULL;
    }

    return (RecordArray *)TierToPtrChainedHashMapIteratorValue(&it);
}

static Value ArrayDbGetValueFromLoaded(Tier tier, Position position) {
    RecordArray *loaded = GetRecordsFromLoaded(tier);
    if (loaded == NULL) return kErrorValue;

    return RecordArrayGetValue(loaded, position);
}

static int ArrayDbGetRemotenessFromLoaded(Tier tier, Position position) {
    RecordArray *loaded = GetRecordsFromLoaded(tier);
    if (loaded == NULL) return kErrorValue;

    return RecordArrayGetRemoteness(loaded, position);
}

static int ArrayDbProbeInit(DbProbe *probe) {
    probe->buffer = GamesmanCallocWhole(1, sizeof(AdbProbeInternal));
    if (probe->buffer == NULL) return kMallocFailureError;

    probe->tier = kIllegalTier;
    // probe->begin and probe->size are unused.

    return kNoError;
}

static int ArrayDbProbeDestroy(DbProbe *probe) {
    AdbProbeInternal *probe_internal = (AdbProbeInternal *)probe->buffer;
    XzraFileClose(probe_internal->file);
    GamesmanFree(probe->buffer);
    memset(probe, 0, sizeof(*probe));

    return kNoError;
}

static bool ProbeSameFile(const DbProbe *probe, TierPosition tier_position) {
    return probe->tier == tier_position.tier;
}

static int ProbeLoadNewTier(DbProbe *probe, Tier tier) {
    AdbProbeInternal *probe_internal = (AdbProbeInternal *)probe->buffer;
    if (probe_internal->init) {
        int error = XzraFileClose(probe_internal->file);
        if (error != 0) return kRuntimeError;
    }

    char *full_path = GetFullPathToFile(tier, CurrentGetTierName);
    if (full_path == NULL) return kMallocFailureError;

    probe_internal->file = XzraFileOpen(full_path);
    GamesmanFree(full_path);
    if (probe_internal->file == NULL) return kFileSystemError;

    probe_internal->init = true;
    probe->tier = tier;
    return kNoError;
}

static Record ProbeGetRecord(const DbProbe *probe, Position position) {
    int64_t offset = position * (int64_t)sizeof(Record);
    Record rec;
    AdbProbeInternal *probe_internal = (AdbProbeInternal *)probe->buffer;
    XzraFileSeek(probe_internal->file, offset, XZRA_SEEK_SET);
    size_t bytes_read = XzraFileRead(&rec, sizeof(rec), probe_internal->file);
    if (bytes_read != sizeof(rec)) {
        fprintf(stderr, "ProbeGetRecord: (BUG) corrupt record\n");
    }

    return rec;
}

static Value ArrayDbProbeValue(DbProbe *probe, TierPosition tier_position) {
    if (!ProbeSameFile(probe, tier_position)) {
        int error = ProbeLoadNewTier(probe, tier_position.tier);
        if (error != kNoError) {
            fprintf(stderr,
                    "ArrayDbProbeValue: failed to load tier %" PRITier "\n",
                    tier_position.tier);
            return kErrorValue;
        }
    }

    Record rec = ProbeGetRecord(probe, tier_position.position);
    return RecordGetValue(&rec);
}

static int ArrayDbProbeRemoteness(DbProbe *probe, TierPosition tier_position) {
    if (!ProbeSameFile(probe, tier_position)) {
        int error = ProbeLoadNewTier(probe, tier_position.tier);
        if (error != kNoError) {
            fprintf(stderr,
                    "ArrayDbProbeRemoteness: failed to load tier %" PRITier
                    "\n",
                    tier_position.tier);
            return kErrorRemoteness;
        }
    }

    Record rec = ProbeGetRecord(probe, tier_position.position);
    return RecordGetRemoteness(&rec);
}

static int ArrayDbTierStatus(Tier tier) {
    char *full_path = GetFullPathToFile(tier, CurrentGetTierName);
    if (full_path == NULL) return kDbTierStatusCheckError;

    FILE *db_file = fopen(full_path, "rb");
    GamesmanFree(full_path);
    if (db_file == NULL) return kDbTierStatusMissing;

    int error = GuardedFclose(db_file);
    if (error != 0) return kDbTierStatusCheckError;

    return kDbTierStatusSolved;
}

static int ArrayDbGameStatus(void) {
    char *full_path = GetFullPathToFinishFlag();
    if (full_path == NULL) return kDbGameStatusCheckError;

    bool exists = FileExists(full_path);
    GamesmanFree(full_path);

    return exists ? kDbGameStatusSolved : kDbGameStatusIncomplete;
}
