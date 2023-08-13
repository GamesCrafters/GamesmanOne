#include "core/db/naivedb/naivedb.h"

#include <assert.h>    // assert
#include <inttypes.h>  // PRId64
#include <malloc.h>    // malloc, calloc, free
#include <stddef.h>    // NULL
#include <stdio.h>     // fprintf, stderr
#include <string.h>    // strncpy, memset

#include "core/gamesman_types.h"

static int NaiveDbInit(const char *game_name, int variant, const char *path,
                       void *aux);
static void NaiveDbFinalize(void);

static int NaiveDbCreateSolvingTier(Tier tier, int64_t size);
static int NaiveDbFlushSolvingTier(void *aux);
static int NaiveDbFreeSolvingTier(void);

static int NaiveDbSetValue(Position position, Value value);
static int NaiveDbSetRemoteness(Position position, int remoteness);
static Value NaiveDbGetValue(TierPosition tier_position);
static int NaiveDbGetRemoteness(TierPosition tier_position);

const Database kNaiveDb = {.name = "Naive DB",

                           .Init = &NaiveDbInit,
                           .Finalize = &NaiveDbFinalize,

                           // Solving
                           .CreateSolvingTier = &NaiveDbCreateSolvingTier,
                           .FlushSolvingTier = &NaiveDbFlushSolvingTier,
                           .FreeSolvingTier = &NaiveDbFreeSolvingTier,
                           .SetValue = &NaiveDbSetValue,
                           .SetRemoteness = &NaiveDbSetRemoteness,

                           // Probing
                           .GetValue = &NaiveDbGetValue,
                           .GetRemoteness = &NaiveDbGetRemoteness};

// -----------------------------------------------------------------------------

typedef struct NaiveDbEntry {
    Value value;
    int remoteness;
} NaiveDbEntry;

static char current_game_name[kGameNameLengthMax + 1];
static int current_variant;
static char *current_path;
static Tier current_tier;
static int64_t current_tier_size;
static NaiveDbEntry *records;

/**
 * @brief Returns the full path to the DB file for the given tier. The user is
 * responsible for free()ing the pointer returned by this function. Returns
 * NULL on failure.
 */
static char *GetFullPathToFile(Tier tier) {
    // Full path: "<path>/<tier>", +2 for '/' and '\0'.
    char *full_path = (char *)calloc(
        (strlen(current_path) + kInt64Base10StringLengthMax + 2), sizeof(char));
    if (full_path == NULL) {
        fprintf(stderr, "GetFullPathToFile: failed to calloc full_path.\n");
        return NULL;
    }
    char file_name[kInt64Base10StringLengthMax + 2];
    snprintf(file_name, kInt64Base10StringLengthMax, "/%" PRId64, tier);
    strcat(full_path, current_path);
    strcat(full_path, file_name);
    return full_path;
}

static NaiveDbEntry ReadEntryFromFile(TierPosition tier_position) {
    NaiveDbEntry entry = {.value = kUndecided, .remoteness = -1};

    char *full_path = GetFullPathToFile(tier_position.tier);
    if (full_path == NULL) return entry;

    FILE *file = fopen(full_path, "rb");
    free(full_path);

    if (file == NULL) {
        perror("fopen");
        fclose(file);
        return entry;
    }

    if (fseek(file, tier_position.position * sizeof(NaiveDbEntry), SEEK_SET) !=
        0) {
        perror("fseek");
        fclose(file);
        return entry;
    }

    if (fread(&entry, sizeof(entry), 1, file) != 1) {
        perror("fread");
        fclose(file);
        return entry;
    }

    if (fclose(file) != 0) {
        perror("fclose");
    }

    return entry;
}

static int NaiveDbInit(const char *game_name, int variant, const char *path,
                       void *aux) {
    assert(current_path == NULL);

    current_path = (char *)malloc((strlen(path) + 1) * sizeof(char));
    if (current_path == NULL) {
        fprintf(stderr, "NaiveDbInit: failed to malloc path.\n");
        return 1;
    }

    strncpy(current_game_name, game_name, kGameNameLengthMax + 1);
    current_variant = variant;
    current_tier = -1;
    current_tier_size = -1;
    assert(records == NULL);

    return 0;
}

static void NaiveDbFinalize(void) {
    free(current_path);
    current_path = NULL;
    free(records);
    records = NULL;
}

static int NaiveDbCreateSolvingTier(Tier tier, int64_t size) {
    current_tier = tier;
    current_tier_size = size;

    free(records);
    records = (NaiveDbEntry *)calloc(size, sizeof(NaiveDbEntry));
    if (records == NULL) {
        fprintf(stderr,
                "NaiveDbCreateSolvingTier: failed to calloc records.\n");
        return false;
    }
    return true;
}

static int NaiveDbFlushSolvingTier(void *aux) {
    // Create a file <tier> at the given path
    char *full_path = GetFullPathToFile(current_tier);
    if (full_path == NULL) return 1;

    FILE *file = fopen(full_path, "wb");
    free(full_path);

    if (file == NULL) {
        perror("fopen");
        return 1;
    }

    // Write records
    int64_t n = fwrite(records, sizeof(records[0]), current_tier_size, file);
    if (n != current_tier_size) {
        perror("fwrite");
        return 1;
    }

    fclose(file);
    return 0;
}

static int NaiveDbFreeSolvingTier(void) {
    free(records);
    records = NULL;
    current_tier = -1;
    current_tier_size = -1;
}

static int NaiveDbSetValue(Position position, Value value) {
    records[position].value = value;
    return 0;
}

static int NaiveDbSetRemoteness(Position position, int remoteness) {
    records[position].remoteness = remoteness;
    return 0;
}

static Value NaiveDbGetValue(TierPosition tier_position) {
    if (tier_position.tier == current_tier) {
        return records[tier_position.position].value;
    }
    NaiveDbEntry entry = ReadEntryFromFile(tier_position);
    return entry.value;
}

static int NaiveDbGetRemoteness(TierPosition tier_position) {
    if (tier_position.tier == current_tier) {
        return records[tier_position.position].remoteness;
    }
    NaiveDbEntry entry = ReadEntryFromFile(tier_position);
    return entry.remoteness;}
