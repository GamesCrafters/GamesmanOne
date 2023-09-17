#include "core/db/bpdb/bpdb_lite.h"

#include <assert.h>  // assert
#include <stddef.h>  // NULL
#include <stdint.h>  // int64_t, uint64_t, int32_t
#include <stdio.h>   // fprintf, stderr, FILE
#include <stdlib.h>  // malloc, free, realloc
#include <string.h>  // strlen

#include "core/db/bpdb/bparray.h"
#include "core/db/bpdb/bpdb_file.h"
#include "core/db/bpdb/bpdb_probe.h"
#include "core/gamesman_types.h"
#include "core/misc.h"

// Include and use OpenMP if the _OPENMP flag is set.
#ifdef _OPENMP
#include <omp.h>
#define PRAGMA(X) _Pragma(#X)
#define PRAGMA_OMP_CRITICAL(name) PRAGMA(omp critical(name))

// Otherwise, the following macros do nothing.
#else
#define PRAGMA
#define PRAGMA_OMP_CRITICAL(name)
#endif

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
    .ProbeInit = &BpdbProbeInit,        // Generic version
    .ProbeDestroy = &BpdbProbeDestroy,  // Generic version
    .ProbeValue = &BpdbLiteProbeValue,
    .ProbeRemoteness = &BpdbLiteProbeRemoteness,
};

static const int kNumValues = 5;  // undecided, lose, draw, tie, win.

static char current_game_name[kGameNameLengthMax + 1];
static int current_variant;
static char *current_path;
static Tier current_tier;
static int64_t current_tier_size;
static BpArray records;

static uint64_t BuildRecord(Value value, int remoteness);
static Value GetValueFromRecord(uint64_t record);
static int GetRemotenessFromRecord(uint64_t record);

// -----------------------------------------------------------------------------

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
    int error = BpArrayInit(&records, current_tier_size);
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

    char *full_path = BpdbFileGetFullPath(current_path, current_tier);
    if (full_path == NULL) return 1;

    int error = BpdbFileFlush(full_path, &records);
    if (error != 0) fprintf(stderr, "BpdbLiteFlushSolvingTier: code %d", error);

    free(full_path);
    return error;
}

static int BpdbLiteFreeSolvingTier(void) {
    BpArrayDestroy(&records);
    current_tier = -1;
    current_tier_size = -1;
    return 0;
}

static uint64_t GetRecord(Position position) {
    uint64_t record;
    PRAGMA_OMP_CRITICAL(records) { record = BpArrayGet(&records, position); }

    return record;
}

static int BpdbLiteSetValue(Position position, Value value) {
    uint64_t record = GetRecord(position);
    int remoteness = GetRemotenessFromRecord(record);
    record = BuildRecord(value, remoteness);

    int error;
    PRAGMA_OMP_CRITICAL(records) {
        error = BpArraySet(&records, position, record);
    }

    return error;
}

static int BpdbLiteSetRemoteness(Position position, int remoteness) {
    uint64_t record = GetRecord(position);
    Value value = GetValueFromRecord(record);
    record = BuildRecord(value, remoteness);

    int error;
    PRAGMA_OMP_CRITICAL(records) {
        error = BpArraySet(&records, position, record);
    }

    return error;
}

static Value BpdbLiteGetValue(Position position) {
    uint64_t record = GetRecord(position);
    return GetValueFromRecord(record);
}

static int BpdbLiteGetRemoteness(Position position) {
    uint64_t record = GetRecord(position);
    return GetRemotenessFromRecord(record);
}

static Value BpdbLiteProbeValue(DbProbe *probe, TierPosition tier_position) {
    uint64_t record = BpdbProbeRecord(current_path, probe, tier_position);
    return GetValueFromRecord(record);
}

static int BpdbLiteProbeRemoteness(DbProbe *probe, TierPosition tier_position) {
    uint64_t record = BpdbProbeRecord(current_path, probe, tier_position);
    return GetRemotenessFromRecord(record);
}

// -----------------------------------------------------------------------------

static uint64_t BuildRecord(Value value, int remoteness) {
    return remoteness * kNumValues + value;
}

static Value GetValueFromRecord(uint64_t record) { return record % kNumValues; }

static int GetRemotenessFromRecord(uint64_t record) {
    return record / kNumValues;
}

#undef PRAGMA
#undef PRAGMA_OMP_CRITICAL
