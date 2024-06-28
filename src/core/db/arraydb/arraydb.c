#include "core/db/arraydb/arraydb.h"

#include <assert.h>   // assert
#include <stdbool.h>  // bool, true, false
#include <stddef.h>   // NULL
#include <stdio.h>    // fprintf, stderr
#include <stdlib.h>   // malloc, calloc, free
#include <string.h>   // strcpy

#ifdef _OPENMP
#include <omp.h>
#endif  // _OPENMP

#include "core/constants.h"
#include "core/db/arraydb/record.h"
#include "core/db/arraydb/record_array.h"
#include "core/misc.h"
#include "core/types/gamesman_types.h"
#include "libs/xzra/xzra.h"

// DB API

static int ArrayDbInit(ReadOnlyString game_name, int variant,
                       ReadOnlyString path, GetTierNameFunc GetTierName,
                       void *aux);
static void ArrayDbFinalize(void);

static int ArrayDbCreateSolvingTier(Tier tier, int64_t size);
static int ArrayDbFlushSolvingTier(void *aux);
static int ArrayDbFreeSolvingTier(void);

static int ArrayDbSetValue(Position position, Value value);
static int ArrayDbSetRemoteness(Position position, int remoteness);
static Value ArrayDbGetValue(Position position);
static int ArrayDbGetRemoteness(Position position);

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

const Database kArrayDb = {
    .name = "arraydb",
    .formal_name = "Array Database",

    .Init = ArrayDbInit,
    .Finalize = ArrayDbFinalize,

    // Solving
    .CreateSolvingTier = ArrayDbCreateSolvingTier,
    .FlushSolvingTier = ArrayDbFlushSolvingTier,
    .FreeSolvingTier = ArrayDbFreeSolvingTier,

    .SetValue = ArrayDbSetValue,
    .SetRemoteness = ArrayDbSetRemoteness,
    .GetValue = ArrayDbGetValue,
    .GetRemoteness = ArrayDbGetRemoteness,

    // Loading
    .LoadTier = ArrayDbLoadTier,
    .UnloadTier = ArrayDbUnloadTier,
    .IsTierLoaded = ArrayDbIsTierLoaded,
    .GetValueFromLoaded = ArrayDbGetValueFromLoaded,
    .GetRemotenessFromLoaded = ArrayDbGetRemotenessFromLoaded,

    // Probing
    .ProbeInit = ArrayDbProbeInit,
    .ProbeDestroy = ArrayDbProbeDestroy,
    .ProbeValue = ArrayDbProbeValue,
    .ProbeRemoteness = ArrayDbProbeRemoteness,
    .TierStatus = ArrayDbTierStatus,
};

// Types

typedef struct {
    XzraFile *file;
    bool init;
} AdbProbeInternal;

// Constants

enum { kArrayDbNumLoadedTiersMax = 256 };
const int kArrayDbRecordSize = sizeof(Record);
const ArrayDbOptions kArrayDbOptionsInit = {
    .block_size = 1 << 20,         // 1 MiB.
    .compression_level = 6,        // LZMA level 6.
    .extreme_compression = false,  // Extreme compression disabled.
};

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
static int64_t current_tier_size;
static RecordArray records;
static TierHashMapExt loaded_tier_to_index;
static RecordArray loaded_records[kArrayDbNumLoadedTiersMax];

static int ArrayDbInit(ReadOnlyString game_name, int variant,
                       ReadOnlyString path, GetTierNameFunc GetTierName,
                       void *aux) {
    const ArrayDbOptions *options = (ArrayDbOptions *)aux;
    if (options == NULL) options = &kArrayDbOptionsInit;
    block_size = options->block_size;
    lzma_level = options->compression_level;
    enable_extreme_compression = options->extreme_compression;

    assert(sandbox_path == NULL);
    sandbox_path = (char *)malloc((strlen(path) + 1) * sizeof(char));
    if (sandbox_path == NULL) {
        fprintf(stderr, "ArrayDbInit: failed to malloc path.\n");
        return kMallocFailureError;
    }

    strcpy(sandbox_path, path);
    strcpy(current_game_name, game_name);
    current_variant = variant;
    CurrentGetTierName = GetTierName;
    current_tier = kIllegalTier;
    current_tier_size = kIllegalSize;
    memset(&records, 0, sizeof(records));
    TierHashMapExtInit(&loaded_tier_to_index, 0.5);
    memset(&loaded_records, 0, sizeof(loaded_records));

    return kNoError;
}

static void ArrayDbFinalize(void) {
    free(sandbox_path);
    sandbox_path = NULL;
    RecordArrayDestroy(&records);
    TierHashMapExtDestroy(&loaded_tier_to_index);
    for (int i = 0; i < kArrayDbNumLoadedTiersMax; ++i) {
        RecordArrayDestroy(&loaded_records[i]);
    }
}

static int ArrayDbCreateSolvingTier(Tier tier, int64_t size) {
    assert(current_tier == kIllegalTier && current_tier_size == kIllegalSize);
    current_tier = tier;
    current_tier_size = size;

    return RecordArrayInit(&records, current_tier_size);
}

/**
 * @brief Returns the full path to the DB file for the given tier. The user is
 * responsible for freeing the pointer returned by this function. Returns
 * NULL on failure.
 */
static char *GetFullPathToFile(Tier tier, GetTierNameFunc GetTierName) {
    // Full path: "<path>/<file_name><ext>", +2 for '/' and '\0'.
    ConstantReadOnlyString extension = ".adb.xz";
    char *full_path = (char *)calloc(
        (strlen(sandbox_path) + kDbFileNameLengthMax + strlen(extension) + 2),
        sizeof(char));
    if (full_path == NULL) {
        fprintf(stderr, "GetFullPathToFile: failed to calloc full_path.\n");
        return NULL;
    }

    int count = sprintf(full_path, "%s/", sandbox_path);
    if (GetTierName != NULL) {
        GetTierName(full_path + count, tier);
    } else {
        sprintf(full_path + count, "%" PRITier, tier);
    }
    strcat(full_path, extension);

    return full_path;
}

static int GetNumThreads(void) {
#ifdef _OPENMP
    return omp_get_max_threads();
#else   // _OPENMP not defined
    return 1;
#endif  // _OPENMP
}

static int ArrayDbFlushSolvingTier(void *aux) {
    (void)aux;  // Unused.

    // Create db file.
    char *full_path = GetFullPathToFile(current_tier, CurrentGetTierName);
    if (full_path == NULL) return kMallocFailureError;

    int64_t compressed_size = XzraCompressStream(
        full_path, false, block_size, lzma_level, enable_extreme_compression,
        GetNumThreads(), RecordArrayGetData(&records),
        RecordArrayGetRawSize(&records));
    free(full_path);
    switch (compressed_size) {
        case -2:
            return kFileSystemError;
        case -3:
            return kRuntimeError;
        default:
            break;
    }

    return kNoError;
}

static int ArrayDbFreeSolvingTier(void) {
    RecordArrayDestroy(&records);
    current_tier = kIllegalTier;
    current_tier_size = kIllegalSize;

    return kNoError;
}

static int ArrayDbSetValue(Position position, Value value) {
    RecordArraySetValue(&records, position, value);

    return kNoError;
}

static int ArrayDbSetRemoteness(Position position, int remoteness) {
    RecordArraySetRemoteness(&records, position, remoteness);

    return kNoError;
}

static Value ArrayDbGetValue(Position position) {
    return RecordArrayGetValue(&records, position);
}

static int ArrayDbGetRemoteness(Position position) {
    return RecordArrayGetRemoteness(&records, position);
}

static int ArrayDbLoadTier(Tier tier, int64_t size) {
    // Find the first unused slot in the loaded records array.
    int i;
    for (i = 0; i < kArrayDbNumLoadedTiersMax; ++i) {
        if (loaded_records[i].records == NULL) break;
    }
    if (i == kArrayDbNumLoadedTiersMax) {
        fprintf(stderr,
                "ArrayDbLoadTier: cannot load more than %d tiers at the same "
                "time\n",
                kArrayDbNumLoadedTiersMax);
        return kRuntimeError;
    }

    int error = RecordArrayInit(&loaded_records[i], size);
    if (error != kNoError) return kMallocFailureError;

    char *full_path = GetFullPathToFile(tier, CurrentGetTierName);
    if (full_path == NULL) {
        RecordArrayDestroy(&loaded_records[i]);
        return kMallocFailureError;
    }

    if (!TierHashMapExtSet(&loaded_tier_to_index, tier, i)) {
        RecordArrayDestroy(&loaded_records[i]);
        free(full_path);
        return kMallocFailureError;
    }

    uint64_t mem = XzraDecompressionMemUsage(
        block_size, lzma_level, enable_extreme_compression, GetNumThreads());
    XzraDecompressFile(RecordArrayGetData(&loaded_records[i]),
                       size * kArrayDbRecordSize, GetNumThreads(), mem,
                       full_path);
    free(full_path);

    return kNoError;
}

static int GetLoadedTierIndex(Tier tier) {
    int64_t index;
    if (!TierHashMapExtGet(&loaded_tier_to_index, tier, &index)) return -1;

    assert(index >= 0 && index < kArrayDbNumLoadedTiersMax);
    return (int)index;
}

static int ArrayDbUnloadTier(Tier tier) {
    int index = GetLoadedTierIndex(tier);
    if (index >= 0) {
        RecordArrayDestroy(&loaded_records[index]);
        TierHashMapExtRemove(&loaded_tier_to_index, tier);
    }

    return kNoError;
}

static bool ArrayDbIsTierLoaded(Tier tier) {
    int index = GetLoadedTierIndex(tier);
    if (index < 0) return false;

    return loaded_records[index].records != NULL;
}

static Value ArrayDbGetValueFromLoaded(Tier tier, Position position) {
    int index = GetLoadedTierIndex(tier);
    if (index < 0) return kErrorValue;

    return RecordArrayGetValue(&loaded_records[index], position);
}

static int ArrayDbGetRemotenessFromLoaded(Tier tier, Position position) {
    int index = GetLoadedTierIndex(tier);
    if (index < 0) return kErrorValue;

    return RecordArrayGetRemoteness(&loaded_records[index], position);
}

static int ArrayDbProbeInit(DbProbe *probe) {
    probe->buffer = calloc(1, sizeof(AdbProbeInternal));
    if (probe->buffer == NULL) return kMallocFailureError;

    probe->tier = kIllegalTier;
    // probe->begin and probe->size are unused.

    return kNoError;
}

static int ArrayDbProbeDestroy(DbProbe *probe) {
    AdbProbeInternal *probe_internal = (AdbProbeInternal *)probe->buffer;
    XzraFileClose(probe_internal->file);
    free(probe->buffer);
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
    free(full_path);
    if (probe_internal->file == NULL) return kFileSystemError;

    probe_internal->init = true;
    probe->tier = tier;
    return kNoError;
}

static Record ProbeGetRecord(const DbProbe *probe, Position position) {
    int64_t offset = position * sizeof(Record);
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
            return kIllegalRemoteness;
        }
    }

    Record rec = ProbeGetRecord(probe, tier_position.position);
    return RecordGetRemoteness(&rec);
}

static int ArrayDbTierStatus(Tier tier) {
    char *full_path = GetFullPathToFile(tier, CurrentGetTierName);
    if (full_path == NULL) return kDbTierStatusCheckError;

    FILE *db_file = fopen(full_path, "rb");
    free(full_path);
    if (db_file == NULL) return kDbTierStatusMissing;

    int error = GuardedFclose(db_file);
    if (error != 0) return kDbTierStatusCheckError;

    return kDbTierStatusSolved;
}
