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
static const int64_t kNumHashes = kNumValues * kNumRemotenesses;

static char current_game_name[kGameNameLengthMax + 1];
static int current_variant;
static char *current_path;
static Tier current_tier;
static int64_t current_tier_size;
static BpArray records;
static int32_t *compression_dict;
static int32_t *decomp_dict;
static int32_t decomp_dict_size;
static int32_t num_unique_values;

static uint64_t BuildRecord(Value value, int remoteness);
static Value GetValueFromRecord(uint64_t record);
static int GetRemotenessFromRecord(uint64_t record);
static uint64_t CompressRecord(uint64_t record);
static int DecompDictExpand(void);
static uint64_t DecompressRecord(uint64_t compressed);

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
    compression_dict = NULL;
    decomp_dict = NULL;
    decomp_dict_size = 0;
    num_unique_values = 0;

    return 0;
}

static void BpdbLiteFinalize(void) {
    free(current_path);
    current_path = NULL;
    BpArrayDestroy(&records);
    free(compression_dict);
    compression_dict = NULL;
    free(decomp_dict);
    decomp_dict = NULL;
    decomp_dict_size = 0;
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

    compression_dict = (int32_t *)malloc(kNumHashes * sizeof(int32_t));
    decomp_dict = (int32_t *)malloc(2 * sizeof(int32_t));
    if (compression_dict == NULL || decomp_dict == NULL) {
        fprintf(stderr,
                "BpdbLiteCreateSolvingTier: failed to malloc dictionary\n");
        BpArrayDestroy(&records);
        return 1;
    }

    // Initialize dictionaries; not worth parallelizing.
    compression_dict[0] = 0;
    for (int64_t i = 1; i < kNumHashes; ++i) {
        compression_dict[i] = -1;
    }
    decomp_dict[0] = 0;
    decomp_dict_size = 2;
    num_unique_values = 1;

    return 0;
}

static int BpdbLiteFlushSolvingTier(void *aux) {
    (void)aux;  // Unused.

    char *full_path = BpdbFileGetFullPath(current_path, current_tier);
    if (full_path == NULL) return 1;

    int error =
        BpdbFileFlush(full_path, &records, decomp_dict, num_unique_values);
    if (error != 0) fprintf(stderr, "BpdbLiteFlushSolvingTier: code %d", error);

    free(full_path);
    return error;
}

static int BpdbLiteFreeSolvingTier(void) {
    BpArrayDestroy(&records);
    free(compression_dict);
    compression_dict = NULL;
    free(decomp_dict);
    decomp_dict = NULL;
    decomp_dict_size = 0;
    num_unique_values = 0;
    current_tier = -1;
    current_tier_size = -1;
    return 0;
}

static int BpdbLiteSetValue(Position position, Value value) {
    uint64_t record = BpArrayAt(&records, position);
    record = DecompressRecord(record);

    int remoteness = GetRemotenessFromRecord(record);
    record = BuildRecord(value, remoteness);
    record = CompressRecord(record);
    BpArraySetTs(&records, position, record);

    return 0;
}

static int BpdbLiteSetRemoteness(Position position, int remoteness) {
    uint64_t record = BpArrayAt(&records, position);
    record = DecompressRecord(record);

    Value value = GetValueFromRecord(record);
    record = BuildRecord(value, remoteness);
    record = CompressRecord(record);
    BpArraySetTs(&records, position, record);

    return 0;
}

static Value BpdbLiteGetValue(Position position) {
    uint64_t record = BpArrayAt(&records, position);
    record = DecompressRecord(record);
    return GetValueFromRecord(record);
}

static int BpdbLiteGetRemoteness(Position position) {
    uint64_t record = BpArrayAt(&records, position);
    record = DecompressRecord(record);
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

static uint64_t CompressRecord(uint64_t record) {
    PRAGMA_OMP_CRITICAL(BpdbCompression) {
        if (compression_dict[record] < 0) {
            compression_dict[record] = num_unique_values;

            if (decomp_dict_size == num_unique_values) {
                int error = DecompDictExpand();
                if (error != 0) exit(1);  // TODO: replace this
            }
            assert(decomp_dict_size > num_unique_values);
            decomp_dict[num_unique_values++] = record;
        }
    }
    return compression_dict[record];
}

static int DecompDictExpand(void) {
    int32_t new_size = decomp_dict_size * 2;
    int32_t *new_decomp_dict =
        (int32_t *)realloc(decomp_dict, new_size * sizeof(int32_t));
    if (new_decomp_dict == NULL) return 1;

    decomp_dict = new_decomp_dict;
    decomp_dict_size = new_size;
    return 0;
}

static uint64_t DecompressRecord(uint64_t compressed) {
    uint64_t record;
    PRAGMA_OMP_CRITICAL(BpdbCompression) { record = decomp_dict[compressed]; }
    return record;
}
