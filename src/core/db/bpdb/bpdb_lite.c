/**
 * @file bpdb_lite.h
 * @author Dan Garcia: designed the "lookup table" compression algorithm
 * @author Max Fierro: improved the algorithm for BpArray compression
 * @author Sameer Nayyar: improved the algorithm for BpArray compression
 * @author Robert Shi (robertyishi@berkeley.edu): improved and implemented
 *         compression algorithms and bpdb.
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Bit-Perfect Database Lite.
 * @version 1.2.1
 * @date 2024-12-10
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

#include "core/db/bpdb/bpdb_lite.h"

#include <assert.h>  // assert
#include <stddef.h>  // NULL
#include <stdint.h>  // int64_t, uint64_t, int32_t
#include <stdio.h>   // fprintf, stderr, FILE
#include <stdlib.h>  // malloc, free, realloc
#include <string.h>  // strlen

#include "core/concurrency.h"
#include "core/constants.h"
#include "core/db/bpdb/bparray.h"
#include "core/db/bpdb/bpdb_file.h"
#include "core/db/bpdb/bpdb_probe.h"
#include "core/misc.h"
#include "core/types/gamesman_types.h"

// Include and use OpenMP if the _OPENMP flag is set.
#ifdef _OPENMP
#include <omp.h>
#endif  // _OPENMP

// Database API.

static int BpdbLiteInit(ReadOnlyString game_name, int variant,
                        ReadOnlyString path, GetTierNameFunc GetTierName,
                        void *aux);
static void BpdbLiteFinalize(void);

static int BpdbLiteCreateSolvingTier(Tier tier, int64_t size);
static int BpdbLiteFlushSolvingTier(void *aux);
static int BpdbLiteFreeSolvingTier(void);

static int BpdbLiteSetGameSolved(void);
static int BpdbLiteSetValue(Position position, Value value);
static int BpdbLiteSetRemoteness(Position position, int remoteness);
static Value BpdbLiteGetValue(Position position);
static int BpdbLiteGetRemoteness(Position position);

static Value BpdbLiteProbeValue(DbProbe *probe, TierPosition tier_position);
static int BpdbLiteProbeRemoteness(DbProbe *probe, TierPosition tier_position);

static int BpdbLiteTierStatus(Tier tier);
static int BpdbLiteGameStatus(void);

const Database kBpdbLite = {
    .name = "bpdb_lite",
    .formal_name = "Bit-Perfect Database Lite",

    .Init = &BpdbLiteInit,
    .Finalize = &BpdbLiteFinalize,

    // Solving
    .CreateSolvingTier = &BpdbLiteCreateSolvingTier,
    .FlushSolvingTier = &BpdbLiteFlushSolvingTier,
    .FreeSolvingTier = &BpdbLiteFreeSolvingTier,

    .SetGameSolved = &BpdbLiteSetGameSolved,
    .SetValue = &BpdbLiteSetValue,
    .SetRemoteness = &BpdbLiteSetRemoteness,
    .GetValue = &BpdbLiteGetValue,
    .GetRemoteness = &BpdbLiteGetRemoteness,

    // Probing
    .ProbeInit = &BpdbProbeInit,        // Generic version
    .ProbeDestroy = &BpdbProbeDestroy,  // Generic version
    .ProbeValue = &BpdbLiteProbeValue,
    .ProbeRemoteness = &BpdbLiteProbeRemoteness,
    .TierStatus = &BpdbLiteTierStatus,
    .GameStatus = &BpdbLiteGameStatus,
};

static char current_game_name[kGameNameLengthMax + 1];
static int current_variant;
static GetTierNameFunc CurrentGetTierName;
static char *sandbox_path;
static Tier current_tier;
static int64_t current_tier_size;
static BpArray records;

static uint64_t BuildRecord(Value value, int remoteness);
static Value GetValueFromRecord(uint64_t record);
static int GetRemotenessFromRecord(uint64_t record);

// -----------------------------------------------------------------------------

static int BpdbLiteInit(ReadOnlyString game_name, int variant,
                        ReadOnlyString path, GetTierNameFunc GetTierName,
                        void *aux) {
    (void)aux;  // Unused.
    assert(sandbox_path == NULL);

    sandbox_path = (char *)malloc((strlen(path) + 1) * sizeof(char));
    if (sandbox_path == NULL) {
        fprintf(stderr, "BpdbLiteInit: failed to malloc path.\n");
        return kMallocFailureError;
    }
    strcpy(sandbox_path, path);

    SafeStrncpy(current_game_name, game_name, kGameNameLengthMax + 1);
    current_game_name[kGameNameLengthMax] = '\0';
    current_variant = variant;
    CurrentGetTierName = GetTierName;
    current_tier = kIllegalTier;
    current_tier_size = kIllegalSize;

    return kNoError;
}

static void BpdbLiteFinalize(void) {
    free(sandbox_path);
    sandbox_path = NULL;
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

    return kNoError;
}

static int BpdbLiteFlushSolvingTier(void *aux) {
    (void)aux;  // Unused.

    char *full_path =
        BpdbFileGetFullPath(sandbox_path, current_tier, CurrentGetTierName);
    if (full_path == NULL) return kMallocFailureError;

    int error = BpdbFileFlush(full_path, &records);
    if (error != 0) fprintf(stderr, "BpdbLiteFlushSolvingTier: code %d", error);
    free(full_path);

    return error;
}

static int BpdbLiteFreeSolvingTier(void) {
    BpArrayDestroy(&records);
    current_tier = kIllegalTier;
    current_tier_size = -kIllegalSize;

    return kNoError;
}

static uint64_t GetRecord(Position position) {
    uint64_t record;
    PRAGMA_OMP_CRITICAL(records) { record = BpArrayGet(&records, position); }

    return record;
}

static int BpdbLiteSetGameSolved(void) {
    char *flag_filename = BpdbFileGetFullPathToFinishFlag(sandbox_path);
    if (flag_filename == NULL) return kMallocFailureError;

    FILE *flag_file = GuardedFopen(flag_filename, "w");
    free(flag_filename);
    if (flag_file == NULL) return kFileSystemError;

    int error = GuardedFclose(flag_file);
    if (error != 0) return kFileSystemError;

    return kNoError;
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
    uint64_t record =
        BpdbProbeRecord(sandbox_path, probe, tier_position, CurrentGetTierName);
    return GetValueFromRecord(record);
}

static int BpdbLiteProbeRemoteness(DbProbe *probe, TierPosition tier_position) {
    uint64_t record =
        BpdbProbeRecord(sandbox_path, probe, tier_position, CurrentGetTierName);
    return GetRemotenessFromRecord(record);
}

static int BpdbLiteTierStatus(Tier tier) {
    return BpdbFileGetTierStatus(sandbox_path, tier, CurrentGetTierName);
}

static int BpdbLiteGameStatus(void) {
    static ConstantReadOnlyString kIndicatorFileName = ".finish";
    char *indicator =
        (char *)malloc(strlen(sandbox_path) + strlen(kIndicatorFileName) + 1);
    if (indicator == NULL) return kDbGameStatusCheckError;

    sprintf(indicator, "%s%s", sandbox_path, kIndicatorFileName);
    bool solved = FileExists(indicator);
    free(indicator);
    if (solved) return kDbGameStatusSolved;

    return kDbGameStatusIncomplete;
}

// -----------------------------------------------------------------------------

static uint64_t BuildRecord(Value value, int remoteness) {
    return remoteness * kNumValues + value;
}

static Value GetValueFromRecord(uint64_t record) { return record % kNumValues; }

static int GetRemotenessFromRecord(uint64_t record) {
    return record / kNumValues;
}
