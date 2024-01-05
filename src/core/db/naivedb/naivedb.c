/**
 * @file naivedb.c
 * @author Robert Shi (robertyishi@berkeley.edu)
 *         GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Implementation of a naive database which stores Values and
 * Remotenesses in uncompressed raw bytes.
 * @version 1.0
 * @date 2023-08-19
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

#include "core/db/naivedb/naivedb.h"

#include <assert.h>    // assert
#include <inttypes.h>  // PRId64
#include <stddef.h>    // NULL
#include <stdio.h>     // fprintf, stderr
#include <stdlib.h>    // malloc, calloc, free
#include <string.h>    // memset

#include "core/constants.h"
#include "core/types/gamesman_types.h"
#include "core/misc.h"

// Database API.

static int NaiveDbInit(ReadOnlyString game_name, int variant,
                       ReadOnlyString path, void *aux);
static void NaiveDbFinalize(void);

static int NaiveDbCreateSolvingTier(Tier tier, int64_t size);
static int NaiveDbFlushSolvingTier(void *aux);
static int NaiveDbFreeSolvingTier(void);

static int NaiveDbSetValue(Position position, Value value);
static int NaiveDbSetRemoteness(Position position, int remoteness);
static Value NaiveDbGetValue(Position position);
static int NaiveDbGetRemoteness(Position position);

static int NaiveDbProbeInit(DbProbe *probe);
static int NaiveDbProbeDestroy(DbProbe *probe);
static Value NaiveDbProbeValue(DbProbe *probe, TierPosition tier_position);
static int NaiveDbProbeRemoteness(DbProbe *probe, TierPosition tier_position);

static int NaiveDbTierStatus(Tier tier);

const Database kNaiveDb = {
    .name = "naivedb",
    .formal_name = "Naive DB",

    .Init = &NaiveDbInit,
    .Finalize = &NaiveDbFinalize,

    // Solving
    .CreateSolvingTier = &NaiveDbCreateSolvingTier,
    .FlushSolvingTier = &NaiveDbFlushSolvingTier,
    .FreeSolvingTier = &NaiveDbFreeSolvingTier,
    .SetValue = &NaiveDbSetValue,
    .SetRemoteness = &NaiveDbSetRemoteness,
    .GetValue = &NaiveDbGetValue,
    .GetRemoteness = &NaiveDbGetRemoteness,

    // Probing
    .ProbeInit = &NaiveDbProbeInit,
    .ProbeDestroy = &NaiveDbProbeDestroy,
    .ProbeValue = &NaiveDbProbeValue,
    .ProbeRemoteness = &NaiveDbProbeRemoteness,
    .TierStatus = &NaiveDbTierStatus,
};

// -----------------------------------------------------------------------------

/**
 * @brief Each entry is a simple structure containing the value and remoteness
 * of the position. Currently 8 bytes in size.
 *
 */
typedef struct NaiveDbEntry {
    Value value;
    int remoteness;
} NaiveDbEntry;

// Probe buffer size, fixed at 1 MiB.
static const int kBufferSize = (2 << 17) * sizeof(NaiveDbEntry);

static char current_game_name[kGameNameLengthMax + 1];
static int current_variant;
static char *sandbox_path;
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
        (strlen(sandbox_path) + kInt64Base10StringLengthMax + 2), sizeof(char));
    if (full_path == NULL) {
        fprintf(stderr, "GetFullPathToFile: failed to calloc full_path.\n");
        return NULL;
    }

    char file_name[kInt64Base10StringLengthMax + 2];
    snprintf(file_name, kInt64Base10StringLengthMax, "/%" PRId64, tier);

    strcat(full_path, sandbox_path);
    strcat(full_path, file_name);
    return full_path;
}

static int ReadFromFile(TierPosition tier_position, void *buffer) {
    char *full_path = GetFullPathToFile(tier_position.tier);
    if (full_path == NULL) return 1;  // OOM.

    FILE *file = fopen(full_path, "rb");
    free(full_path);

    if (file == NULL) {
        perror("fopen");
        return 2;
    }

    int64_t offset = tier_position.position * sizeof(NaiveDbEntry);
    if (fseek(file, offset, SEEK_SET) != 0) {
        perror("fseek");
        fclose(file);
        return 2;
    }

    size_t num_entries = kBufferSize / sizeof(NaiveDbEntry);
    if (fread(buffer, sizeof(NaiveDbEntry), num_entries, file) != num_entries &&
        !feof(file)) {
        perror("fread");
        fclose(file);
        return 2;
    }

    if (fclose(file) != 0) {
        perror("fclose");
    }

    return 0;
}

static int NaiveDbInit(ReadOnlyString game_name, int variant,
                       ReadOnlyString path, void *aux) {
    (void)aux;  // Unused.
    assert(sandbox_path == NULL);

    sandbox_path = (char *)malloc((strlen(path) + 1) * sizeof(char));
    if (sandbox_path == NULL) {
        fprintf(stderr, "NaiveDbInit: failed to malloc path.\n");
        return 1;
    }
    strcpy(sandbox_path, path);

    SafeStrncpy(current_game_name, game_name, kGameNameLengthMax + 1);
    current_game_name[kGameNameLengthMax] = '\0';
    current_variant = variant;
    current_tier = kIllegalTier;
    current_tier_size = kIllegalSize;
    assert(records == NULL);

    return 0;
}

static void NaiveDbFinalize(void) {
    free(sandbox_path);
    sandbox_path = NULL;
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
        return 1;
    }
    return 0;
}

static int NaiveDbFlushSolvingTier(void *aux) {
    (void)aux;  // Unused.

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
    current_tier = kIllegalTier;
    current_tier_size = kIllegalSize;
    return 0;
}

static int NaiveDbSetValue(Position position, Value value) {
    records[position].value = value;
    return 0;
}

static int NaiveDbSetRemoteness(Position position, int remoteness) {
    records[position].remoteness = remoteness;
    return 0;
}

static Value NaiveDbGetValue(Position position) {
    return records[position].value;
}

static int NaiveDbGetRemoteness(Position position) {
    return records[position].remoteness;
}

static int NaiveDbProbeInit(DbProbe *probe) {
    probe->buffer = malloc(kBufferSize);
    if (probe->buffer == NULL) return 1;

    probe->tier = kIllegalTier;
    probe->begin = 0;
    probe->size = kBufferSize;

    return 0;
}

static int NaiveDbProbeDestroy(DbProbe *probe) {
    free(probe->buffer);
    memset(probe, 0, sizeof(*probe));
    return 0;
}

static bool ProbeBufferHit(DbProbe *probe, TierPosition tier_position) {
    if (probe->tier != tier_position.tier) return false;
    int64_t record_offset = tier_position.position * sizeof(NaiveDbEntry);
    if (record_offset < probe->begin) return false;
    if (record_offset >= probe->begin + probe->size) return false;
    return true;
}

static bool ProbeFillBuffer(DbProbe *probe, TierPosition tier_position) {
    if (!ProbeBufferHit(probe, tier_position)) {
        if (ReadFromFile(tier_position, probe->buffer) != 0) {
            fprintf(stderr, "ProbeFillBuffer: failed to read from file.\n");
            return false;
        }
        probe->tier = tier_position.tier;
        probe->begin = tier_position.position * sizeof(NaiveDbEntry);
    }
    return true;
}

static void *VoidPointerShift(void *pointer, int64_t offset) {
    return (void *)((int8_t *)pointer + offset);
}

static NaiveDbEntry ProbeGetRecord(DbProbe *probe, Position position) {
    int64_t offset = position * sizeof(NaiveDbEntry) - probe->begin;
    assert(offset >= 0);
    NaiveDbEntry entry;
    memcpy(&entry, VoidPointerShift(probe->buffer, offset),
           sizeof(NaiveDbEntry));
    return entry;
}

static Value NaiveDbProbeValue(DbProbe *probe, TierPosition tier_position) {
    if (!ProbeFillBuffer(probe, tier_position)) return kErrorValue;
    NaiveDbEntry record = ProbeGetRecord(probe, tier_position.position);
    return record.value;
}

static int NaiveDbProbeRemoteness(DbProbe *probe, TierPosition tier_position) {
    if (!ProbeFillBuffer(probe, tier_position)) return kIllegalRemoteness;
    NaiveDbEntry record = ProbeGetRecord(probe, tier_position.position);
    return record.remoteness;
}

static int NaiveDbTierStatus(Tier tier) {
    char *full_path = GetFullPathToFile(tier);
    if (full_path == NULL) return kDbTierCheckError;

    FILE *db_file = fopen(full_path, "rb");
    free(full_path);
    if (db_file == NULL) return kDbTierMissing;

    int error = GuardedFclose(db_file);
    if (error != 0) return kDbTierCheckError;

    return kDbTierSolved;
}
