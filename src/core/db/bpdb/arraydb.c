#include "core/db/bpdb/arraydb.h"

#include <assert.h>   // assert
#include <stdbool.h>  // bool, true, false
#include <stddef.h>   // NULL
#include <stdio.h>    // fprintf, stderr, SEEK_SET
#include <stdlib.h>   // malloc, calloc, free
#include <string.h>   // strcpy

#include "core/constants.h"
#include "core/db/bpdb/record.h"
#include "core/db/bpdb/record_array.h"
#include "core/misc.h"
#include "core/types/gamesman_types.h"

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

    // Probing
    .ProbeInit = ArrayDbProbeInit,
    .ProbeDestroy = ArrayDbProbeDestroy,
    .ProbeValue = ArrayDbProbeValue,
    .ProbeRemoteness = ArrayDbProbeRemoteness,
    .TierStatus = ArrayDbTierStatus,
};

// Probe buffer size.
static const int kBufferSize = (1 << 19) * sizeof(Record);

static char current_game_name[kGameNameLengthMax + 1];
static int current_variant;
static GetTierNameFunc CurrentGetTierName;
static char *sandbox_path;
static Tier current_tier;
static int64_t current_tier_size;
static RecordArray records;

static int ArrayDbInit(ReadOnlyString game_name, int variant,
                       ReadOnlyString path, GetTierNameFunc GetTierName,
                       void *aux) {
    (void)aux;  // Unused.
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
    assert(records.records == NULL);

    return kNoError;
}

static void ArrayDbFinalize(void) {
    free(sandbox_path);
    sandbox_path = NULL;
    RecordArrayDestroy(&records);
}

static int ArrayDbCreateSolvingTier(Tier tier, int64_t size) {
    assert(current_tier == kIllegalTier && current_tier_size == kIllegalSize);
    assert(records.records == NULL);
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
    ConstantReadOnlyString extension = ".adb";
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

static int ArrayDbFlushSolvingTier(void *aux) {
    (void)aux;  // Unused.

    // Create db file.
    char *full_path = GetFullPathToFile(current_tier, CurrentGetTierName);
    if (full_path == NULL) return kMallocFailureError;

    FILE *file = GuardedFopen(full_path, "wb");
    free(full_path);
    if (file == NULL) return kFileSystemError;

    // Write records
    int error = GuardedFwrite(RecordArrayGetData(&records), sizeof(Record),
                              current_tier_size, file);
    if (error != kNoError) return kFileSystemError;

    error = GuardedFclose(file);
    if (error != 0) return kFileSystemError;

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

static int ArrayDbProbeInit(DbProbe *probe) {
    probe->buffer = malloc(kBufferSize);
    if (probe->buffer == NULL) return kMallocFailureError;

    probe->tier = kIllegalTier;
    probe->begin = 0;
    probe->size = kBufferSize;

    return kNoError;
}

static int ArrayDbProbeDestroy(DbProbe *probe) {
    free(probe->buffer);
    memset(probe, 0, sizeof(*probe));

    return kNoError;
}

static bool ProbeBufferHit(const DbProbe *probe, TierPosition tier_position) {
    if (probe->tier != tier_position.tier) return false;
    int64_t record_offset = tier_position.position * sizeof(Record);
    if (record_offset < probe->begin) return false;
    if (record_offset >= probe->begin + probe->size) return false;

    return true;
}

static int ReadFromFile(TierPosition tier_position, void *buffer) {
    char *full_path = GetFullPathToFile(tier_position.tier, CurrentGetTierName);
    if (full_path == NULL) return kMallocFailureError;

    FILE *file = GuardedFopen(full_path, "rb");
    free(full_path);
    if (file == NULL) return kFileSystemError;

    int64_t offset = tier_position.position * sizeof(Record);
    int error = GuardedFseek(file, offset, SEEK_SET);
    if (error != 0) return BailOutFclose(file, kFileSystemError);

    size_t num_entries = kBufferSize / sizeof(Record);
    error = GuardedFread(buffer, sizeof(Record), num_entries, file, true);
    if (error != 0 && error != 2) return BailOutFclose(file, kFileSystemError);

    error = GuardedFclose(file);
    if (error != 0) return kFileSystemError;

    return kNoError;
}

static int ProbeFillBuffer(DbProbe *probe, TierPosition tier_position) {
    int error = ReadFromFile(tier_position, probe->buffer);
    if (error != kNoError) {
        fprintf(stderr, "ProbeFillBuffer: failed to read from file.\n");
        return error;
    }

    probe->tier = tier_position.tier;
    probe->begin = tier_position.position * sizeof(Record);

    return kNoError;
}

static Record ProbeGetRecord(const DbProbe *probe, Position position) {
    int64_t offset = position * sizeof(Record) - probe->begin;
    assert(offset >= 0);
    Record rec;
    memcpy(&rec, GenericPointerAdd(probe->buffer, offset), sizeof(rec));

    return rec;
}

static Value ArrayDbProbeValue(DbProbe *probe, TierPosition tier_position) {
    if (!ProbeBufferHit(probe, tier_position)) {
        int error = ProbeFillBuffer(probe, tier_position);
        if (error != kNoError) {
            fprintf(stderr, "ArrayDbProbeValue: failed to load buffer\n");
            return kErrorValue;
        }
    }

    Record rec = ProbeGetRecord(probe, tier_position.position);
    return RecordGetValue(&rec);
}

static int ArrayDbProbeRemoteness(DbProbe *probe, TierPosition tier_position) {
    if (!ProbeBufferHit(probe, tier_position)) {
        int error = ProbeFillBuffer(probe, tier_position);
        if (error != kNoError) {
            fprintf(stderr, "ArrayDbProbeRemoteness: failed to load buffer\n");
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
