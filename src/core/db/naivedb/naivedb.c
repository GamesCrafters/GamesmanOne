/**
 * @file naivedb.c
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Implementation of a naive database which stores Values and
 * Remotenesses in uncompressed raw bytes.
 * @version 1.2.5
 * @date 2025-06-10
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

#include <assert.h>  // assert
#include <stddef.h>  // NULL
#include <stdio.h>   // fprintf, stderr, fopen, fseek, fclose
#include <string.h>  // strcpy, memset

#include "core/constants.h"
#include "core/gamesman_memory.h"
#include "core/misc.h"
#include "core/types/gamesman_types.h"

// Database API.

static int NaiveDbInit(ReadOnlyString game_name, int variant,
                       ReadOnlyString path, GetTierNameFunc GetTierName,
                       void *aux);
static void NaiveDbFinalize(void);

static int NaiveDbCreateSolvingTier(Tier tier, int64_t size);
static int NaiveDbFlushSolvingTier(void *aux);
static int NaiveDbFreeSolvingTier(void);

static int NaiveDbSetGameSolved(void);
static int NaiveDbSetValue(Position position, Value value);
static int NaiveDbSetRemoteness(Position position, int remoteness);
static Value NaiveDbGetValue(Position position);
static int NaiveDbGetRemoteness(Position position);

static int NaiveDbLoadTier(Tier tier, int64_t size);
static int NaiveDbUnloadTier(Tier tier);
static bool NaiveDbIsTierLoaded(Tier tier);
static Value NaiveDbGetValueFromLoaded(Tier tier, Position position);
static int NaiveDbGetRemotenessFromLoaded(Tier tier, Position position);

static int NaiveDbProbeInit(DbProbe *probe);
static int NaiveDbProbeDestroy(DbProbe *probe);
static Value NaiveDbProbeValue(DbProbe *probe, TierPosition tier_position);
static int NaiveDbProbeRemoteness(DbProbe *probe, TierPosition tier_position);
static int NaiveDbTierStatus(Tier tier);
static int NaiveDbGameStatus(void);

const Database kNaiveDb = {
    .name = "naivedb",
    .formal_name = "Naive DB",

    .Init = &NaiveDbInit,
    .Finalize = &NaiveDbFinalize,

    // Solving
    .CreateSolvingTier = &NaiveDbCreateSolvingTier,
    .FlushSolvingTier = &NaiveDbFlushSolvingTier,
    .FreeSolvingTier = &NaiveDbFreeSolvingTier,

    .SetGameSolved = &NaiveDbSetGameSolved,
    .SetValue = &NaiveDbSetValue,
    .SetRemoteness = &NaiveDbSetRemoteness,
    .GetValue = &NaiveDbGetValue,
    .GetRemoteness = &NaiveDbGetRemoteness,

    // Loading
    .LoadTier = &NaiveDbLoadTier,
    .UnloadTier = &NaiveDbUnloadTier,
    .IsTierLoaded = &NaiveDbIsTierLoaded,
    .GetValueFromLoaded = &NaiveDbGetValueFromLoaded,
    .GetRemotenessFromLoaded = &NaiveDbGetRemotenessFromLoaded,

    // Probing
    .ProbeInit = &NaiveDbProbeInit,
    .ProbeDestroy = &NaiveDbProbeDestroy,
    .ProbeValue = &NaiveDbProbeValue,
    .ProbeRemoteness = &NaiveDbProbeRemoteness,
    .TierStatus = &NaiveDbTierStatus,
    .GameStatus = &NaiveDbGameStatus,
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
static const int kBufferSize = (1 << 17) * sizeof(NaiveDbEntry);

static char current_game_name[kGameNameLengthMax + 1];
static int current_variant;
static GetTierNameFunc CurrentGetTierName;
static char *sandbox_path;
static Tier current_tier;
static int64_t current_tier_size;
static NaiveDbEntry *records;
static TierToPtrChainedHashMap loaded_tiers;

/**
 * @brief Returns the full path to the DB file for the given tier. The user is
 * responsible for freeing the pointer returned by this function. Returns
 * NULL on failure.
 */
static char *GetFullPathToFile(Tier tier, GetTierNameFunc GetTierName) {
    // Full path: "<path>/<file_name>", +2 for '/' and '\0'.
    char *full_path = (char *)GamesmanCallocWhole(
        (strlen(sandbox_path) + kDbFileNameLengthMax + 2), sizeof(char));
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

    return full_path;
}

static char *GetFullPathToFinishFlag(void) {
    // Full path: "<path>/.finish", +2 for '/' and '\0'.
    ConstantReadOnlyString kFinishFlagFilename = ".finish";
    char *full_path = (char *)GamesmanCallocWhole(
        (strlen(sandbox_path) + strlen(kFinishFlagFilename) + 2), sizeof(char));
    if (full_path == NULL) {
        fprintf(stderr,
                "GetFullPathToFinishFlag: failed to calloc full_path.\n");
        return NULL;
    }

    sprintf(full_path, "%s/%s", sandbox_path, kFinishFlagFilename);
    return full_path;
}

static int ReadFromFile(TierPosition tier_position, void *buffer) {
    char *full_path = GetFullPathToFile(tier_position.tier, CurrentGetTierName);
    if (full_path == NULL) return kMallocFailureError;  // OOM.

    FILE *file = fopen(full_path, "rb");
    GamesmanFree(full_path);

    if (file == NULL) {
        perror("fopen");
        return kFileSystemError;
    }

    int64_t offset = tier_position.position * (int64_t)sizeof(NaiveDbEntry);
    if (fseek(file, offset, SEEK_SET) != 0) {
        perror("fseek");
        fclose(file);
        return kFileSystemError;
    }

    size_t num_entries = kBufferSize / sizeof(NaiveDbEntry);
    if (fread(buffer, sizeof(NaiveDbEntry), num_entries, file) != num_entries &&
        !feof(file)) {
        perror("fread");
        fclose(file);
        return kFileSystemError;
    }

    if (fclose(file) != 0) {
        perror("fclose");
    }

    return kNoError;
}

static int NaiveDbInit(ReadOnlyString game_name, int variant,
                       ReadOnlyString path, GetTierNameFunc GetTierName,
                       void *aux) {
    (void)aux;  // Unused.
    assert(sandbox_path == NULL);

    sandbox_path = (char *)GamesmanMalloc((strlen(path) + 1) * sizeof(char));
    if (sandbox_path == NULL) {
        fprintf(stderr, "NaiveDbInit: failed to malloc path.\n");
        return kMallocFailureError;
    }
    strcpy(sandbox_path, path);

    SafeStrncpy(current_game_name, game_name, kGameNameLengthMax + 1);
    current_game_name[kGameNameLengthMax] = '\0';
    current_variant = variant;
    CurrentGetTierName = GetTierName;
    current_tier = kIllegalTier;
    current_tier_size = kIllegalSize;
    assert(records == NULL);
    TierToPtrChainedHashMapInit(&loaded_tiers, 0.5);

    return kNoError;
}

static void NaiveDbFinalize(void) {
    GamesmanFree(sandbox_path);
    sandbox_path = NULL;

    // Free all loaded records, including the solving tier.
    TierToPtrChainedHashMapIterator it =
        TierToPtrChainedHashMapBegin(&loaded_tiers);
    while (TierToPtrChainedHashMapIteratorIsValid(&it)) {
        GamesmanFree(TierToPtrChainedHashMapIteratorValue(&it));
    }
    TierToPtrChainedHashMapDestroy(&loaded_tiers);
    records = NULL;
}

static int NaiveDbCreateSolvingTier(Tier tier, int64_t size) {
    GamesmanFree(records);
    records = (NaiveDbEntry *)GamesmanCallocWhole(size, sizeof(NaiveDbEntry));
    if (records == NULL) {
        fprintf(stderr,
                "NaiveDbCreateSolvingTier: failed to calloc records.\n");
        return kMallocFailureError;
    }
    if (!TierToPtrChainedHashMapSet(&loaded_tiers, tier, records)) {
        GamesmanFree(records);
        records = NULL;
        return kMallocFailureError;
    }
    current_tier = tier;
    current_tier_size = size;

    return kNoError;
}

static int NaiveDbFlushSolvingTier(void *aux) {
    (void)aux;  // Unused.

    // Create a file <tier> at the given path
    char *full_path = GetFullPathToFile(current_tier, CurrentGetTierName);
    if (full_path == NULL) return kMallocFailureError;

    FILE *file = fopen(full_path, "wb");
    GamesmanFree(full_path);

    if (file == NULL) {
        perror("fopen");
        return kFileSystemError;
    }

    // Write records
    int64_t n =
        (int64_t)fwrite(records, sizeof(records[0]), current_tier_size, file);
    if (n != current_tier_size) {
        perror("fwrite");
        fclose(file);
        return kFileSystemError;
    }

    fclose(file);
    return kNoError;
}

static int NaiveDbFreeSolvingTier(void) {
    GamesmanFree(records);
    records = NULL;
    TierToPtrChainedHashMapRemove(&loaded_tiers, current_tier);
    current_tier = kIllegalTier;
    current_tier_size = kIllegalSize;

    return kNoError;
}

static int NaiveDbSetGameSolved(void) {
    char *full_path = GetFullPathToFinishFlag();
    if (full_path == NULL) return kMallocFailureError;

    FILE *f = fopen(full_path, "w");
    GamesmanFree(full_path);
    if (f == NULL) return kFileSystemError;

    if (fclose(f) != 0) return kFileSystemError;

    return kNoError;
}

static int NaiveDbSetValue(Position position, Value value) {
    records[position].value = value;
    return kNoError;
}

static int NaiveDbSetRemoteness(Position position, int remoteness) {
    records[position].remoteness = remoteness;
    return kNoError;
}

static Value NaiveDbGetValue(Position position) {
    return records[position].value;
}

static int NaiveDbGetRemoteness(Position position) {
    return records[position].remoteness;
}

static int NaiveDbLoadTier(Tier tier, int64_t size) {
    char *full_path = GetFullPathToFile(tier, CurrentGetTierName);
    NaiveDbEntry *load =
        (NaiveDbEntry *)GamesmanCallocWhole(size, sizeof(NaiveDbEntry));
    if (full_path == NULL || load == NULL) {
        GamesmanFree(full_path);
        GamesmanFree(load);
        return kMallocFailureError;
    }

    // Open db file
    FILE *db_file = fopen(full_path, "rb");
    GamesmanFree(full_path);
    if (db_file == NULL) {
        GamesmanFree(load);
        return kFileSystemError;
    }

    // Read db file into "load"
    size_t n = fread(load, sizeof(NaiveDbEntry), size, db_file);
    fclose(db_file);

    // Set loaded tier in map
    if (!TierToPtrChainedHashMapSet(&loaded_tiers, tier, load)) {
        GamesmanFree(load);
        load = NULL;
        return kMallocFailureError;
    }

    return n == (size_t)size ? kNoError : kFileSystemError;
}

static int NaiveDbUnloadTier(Tier tier) {
    TierToPtrChainedHashMapIterator it =
        TierToPtrChainedHashMapGet(&loaded_tiers, tier);
    if (TierToPtrChainedHashMapIteratorIsValid(&it)) {
        GamesmanFree(TierToPtrChainedHashMapIteratorValue(&it));
        TierToPtrChainedHashMapRemove(&loaded_tiers, tier);
    }

    return kNoError;
}

static bool NaiveDbIsTierLoaded(Tier tier) {
    TierToPtrChainedHashMapIterator it =
        TierToPtrChainedHashMapGet(&loaded_tiers, tier);

    return TierToPtrChainedHashMapIteratorIsValid(&it);
}

static NaiveDbEntry GetEntryFromLoaded(Tier tier, Position position) {
    TierToPtrChainedHashMapIterator it =
        TierToPtrChainedHashMapGet(&loaded_tiers, tier);
    if (!TierToPtrChainedHashMapIteratorIsValid(&it)) {
        return (NaiveDbEntry){.value = kErrorValue, .remoteness = -1};
    }

    NaiveDbEntry *records =
        (NaiveDbEntry *)TierToPtrChainedHashMapIteratorValue(&it);

    return records[position];
}

static Value NaiveDbGetValueFromLoaded(Tier tier, Position position) {
    return GetEntryFromLoaded(tier, position).value;
}

static int NaiveDbGetRemotenessFromLoaded(Tier tier, Position position) {
    return GetEntryFromLoaded(tier, position).remoteness;
}

static int NaiveDbProbeInit(DbProbe *probe) {
    probe->buffer = GamesmanMalloc(kBufferSize);
    if (probe->buffer == NULL) return kMallocFailureError;

    probe->tier = kIllegalTier;
    probe->begin = 0;
    probe->size = kBufferSize;

    return kNoError;
}

static int NaiveDbProbeDestroy(DbProbe *probe) {
    GamesmanFree(probe->buffer);
    memset(probe, 0, sizeof(*probe));

    return kNoError;
}

static bool ProbeBufferHit(DbProbe *probe, TierPosition tier_position) {
    if (probe->tier != tier_position.tier) return false;
    int64_t record_offset =
        tier_position.position * (int64_t)sizeof(NaiveDbEntry);
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
        probe->begin = tier_position.position * (int64_t)sizeof(NaiveDbEntry);
    }
    return true;
}

static NaiveDbEntry ProbeGetRecord(DbProbe *probe, Position position) {
    int64_t offset = position * (int64_t)sizeof(NaiveDbEntry) - probe->begin;
    assert(offset >= 0);
    NaiveDbEntry entry;
    memcpy(&entry, GenericPointerAdd(probe->buffer, offset),
           sizeof(NaiveDbEntry));
    return entry;
}

static Value NaiveDbProbeValue(DbProbe *probe, TierPosition tier_position) {
    if (!ProbeFillBuffer(probe, tier_position)) return kErrorValue;
    NaiveDbEntry record = ProbeGetRecord(probe, tier_position.position);
    return record.value;
}

static int NaiveDbProbeRemoteness(DbProbe *probe, TierPosition tier_position) {
    if (!ProbeFillBuffer(probe, tier_position)) return kErrorRemoteness;
    NaiveDbEntry record = ProbeGetRecord(probe, tier_position.position);
    return record.remoteness;
}

static int NaiveDbTierStatus(Tier tier) {
    char *full_path = GetFullPathToFile(tier, CurrentGetTierName);
    if (full_path == NULL) return kDbTierStatusCheckError;

    FILE *db_file = fopen(full_path, "rb");
    GamesmanFree(full_path);
    if (db_file == NULL) return kDbTierStatusMissing;

    int error = GuardedFclose(db_file);
    if (error != 0) return kDbTierStatusCheckError;

    return kDbTierStatusSolved;
}

static int NaiveDbGameStatus(void) {
    char *full_path = GetFullPathToFinishFlag();
    if (full_path == NULL) return kDbGameStatusCheckError;

    bool exists = FileExists(full_path);
    GamesmanFree(full_path);

    return exists ? kDbGameStatusSolved : kDbGameStatusIncomplete;
}
