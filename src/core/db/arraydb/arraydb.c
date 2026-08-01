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
 * @version 1.2.0
 * @date 2025-06-23
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

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "core/concurrency.h"
#include "core/constants.h"
#include "core/db/arraydb/atomic_record.h"
#include "core/db/arraydb/atomic_record_array.h"
#include "core/db/arraydb/record.h"
#include "core/db/arraydb/record_array.h"
#include "core/gamesman_memory.h"
#include "core/misc.h"
#include "core/types/base.h"
#include "core/types/database/database.h"
#include "core/types/database/db_probe.h"
#include "core/types/game/game.h"
#include "core/types/gamesman_status.h"
#include "core/types/tier_to_ptr_chained_hash_map.h"
#include "libs/io/xfile.h"
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
static bool ArrayDbMaximizeValueRemoteness(Position position, Value value,
                                           int remoteness,
                                           int (*compare)(Value v1, int r1,
                                                          Value v2, int r2));
static int ArrayDbDecrementNumUndecidedChildren(Position position);
static int ArrayDbClearNumUndecidedChildren(Position position);
static Value ArrayDbGetValue(Position position);
static int ArrayDbGetRemoteness(Position position);
static int ArrayDbGetNumUndecidedChildren(Position position);

static int ArrayDbSegmentationMaxNumBuffers(void);
static int ArrayDbSegmentationCreateBuffers(Tier tier, int num_segments,
                                            int64_t size);
static int ArrayDbSegmentationLoad(int buf_idx, int seg_idx);
static int ArrayDbSegmentationFlush(int buf_idx, int seg_idx);
static int ArrayDbSegmentationFreeBuffers(void);
static Value ArrayDbSegmentationGetValue(int buf_idx, int64_t offset);
static int ArrayDbSegmentationGetRemoteness(int buf_idx, int64_t offset);
static void ArrayDbSegmentationSetValueRemoteness(int buf_idx, int64_t offset,
                                                  Value value, int remoteness);
static int ArrayDbSegmentationConsolidate(int64_t tier_size, int num_segments);

static bool ArrayDbCheckpointExists(Tier tier);
static int ArrayDbCheckpointSave(const void *status, size_t status_size);
static int ArrayDbCheckpointLoad(Tier tier, int64_t size, void *status,
                                 size_t status_size);
static int ArrayDbCheckpointRemove(Tier tier);

static size_t ArrayDbTierMemUsage(Tier tier, int64_t size);
static size_t ArrayDbConcurrentTierMemUsage(Tier tier, int64_t size);
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

const char *ArrayDbGetPath(void);

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
    .DecrementNumUndecidedChildren = ArrayDbDecrementNumUndecidedChildren,
    .ClearNumUndecidedChildren = ArrayDbClearNumUndecidedChildren,
    .GetValue = ArrayDbGetValue,
    .GetRemoteness = ArrayDbGetRemoteness,
    .GetNumUndecidedChildren = ArrayDbGetNumUndecidedChildren,

    .segmentation =
        {
            .MaxNumBuffers = ArrayDbSegmentationMaxNumBuffers,
            .CreateBuffers = ArrayDbSegmentationCreateBuffers,
            .Load = ArrayDbSegmentationLoad,
            .Flush = ArrayDbSegmentationFlush,
            .FreeBuffers = ArrayDbSegmentationFreeBuffers,
            .GetValue = ArrayDbSegmentationGetValue,
            .GetRemoteness = ArrayDbSegmentationGetRemoteness,
            .SetValueRemoteness = ArrayDbSegmentationSetValueRemoteness,
            .Consolidate = ArrayDbSegmentationConsolidate,
        },

    .CheckpointExists = ArrayDbCheckpointExists,
    .CheckpointSave = ArrayDbCheckpointSave,
    .CheckpointLoad = ArrayDbCheckpointLoad,

    // Loading
    .TierMemUsage = ArrayDbTierMemUsage,
    .ConcurrentTierMemUsage = ArrayDbConcurrentTierMemUsage,
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

    .GetPath = ArrayDbGetPath,
};

// Extern constants (see arraydb.h for comments)

const int kArrayDbRecordSize = sizeof(Record);

const ArrayDbOptions kArrayDbOptionsInit = {
    .block_size = 1 << 20,  // 1 MiB.
    .lzma_level = 6,        // LZMA level 6.
    .lzma_extreme = false,  // Extreme compression disabled.
};

// Types

/** Goes inside of the buffer of an ArrayDb's DbProbe. */
typedef struct {
    // Compressed DB archive of the tier currently loaded.
    // XzraFile is buffered so we don't need to provide our own.
    XzraFile *file;
} AdbProbeInternal;

// Constants

/** Default LZ4 fast compression level. */
enum { kDefaultLz4Level = 0 };

/** Maximum number of solving segments to activate. */
enum { kNumSolvingSegmentsMax = 8 };

// Global state variables

/** Options for LZMA compression */
static struct {
    int block_size;  // Number of bytes in each independently compressed block.
    int level;       // LZMA compression level.
    bool extreme;    // Whether to use extreme compression mode.
} lzma_options;

/** Current game information */
static struct {
    char name[kGameNameLengthMax + 1];  // Name of the game.
    int variant;                        // Variant of the game.
    GetTierNameFunc GetTierName;        // Function to get the name of a tier.
    Tier tier;                          // Current tier.
} current_game;

/** Path to a sandbox directory reserved for ArrayDb to use. */
static char *sandbox_path;

/** Records for the solving tier. */
static struct {
    // Non-atomic records.
    RecordArray *records;

    // Atomic records.
    AtomicRecordArray *atomic_records;

    // True -> using atomic records; False -> using non-atomic records.
    bool is_concurrent;
} solving;

/** Map from loaded tiers to their RecordArray pointers. */
static TierToPtrChainedHashMap loaded_tiers;

/** Solving segments for algorithms that support streaming. */
static struct {
    RecordArray *array[kNumSolvingSegmentsMax];  // Records in the segment.
    int num_active;  // Number of segments initialized and usable.
} segments;

static int ArrayDbInit(ReadOnlyString game_name, int variant,
                       ReadOnlyString path, GetTierNameFunc GetTierName,
                       void *aux) {
    const ArrayDbOptions *options = (ArrayDbOptions *)aux;
    if (options == NULL) options = &kArrayDbOptionsInit;
    lzma_options.block_size = options->block_size;
    lzma_options.level = options->lzma_level;
    lzma_options.extreme = options->lzma_extreme;

    assert(sandbox_path == NULL);
    sandbox_path = (char *)GamesmanMalloc((strlen(path) + 1) * sizeof(char));
    if (sandbox_path == NULL) {
        fprintf(stderr, "ArrayDbInit: failed to malloc path.\n");
        return kMallocFailureError;
    }

    strcpy(sandbox_path, path);
    strcpy(current_game.name, game_name);
    current_game.variant = variant;
    current_game.GetTierName = GetTierName;
    current_game.tier = kIllegalTier;
    TierToPtrChainedHashMapInit(&loaded_tiers, 0.75);
    memset(&segments, 0, sizeof(segments));

    return kSuccess;
}

static void ArrayDbFinalize(void) {
    GamesmanFree(sandbox_path);
    sandbox_path = NULL;

    // Free the current solving tier, if exists.
    ArrayDbFreeSolvingTier();

    // Free all other loaded records.
    TierToPtrChainedHashMapIterator it =
        TierToPtrChainedHashMapBegin(&loaded_tiers);
    while (TierToPtrChainedHashMapIteratorIsValid(&it)) {
        RecordArrayDestroy(
            (RecordArray *)TierToPtrChainedHashMapIteratorValue(&it));
        TierToPtrChainedHashMapIteratorNext(&it);
    }
    TierToPtrChainedHashMapDestroy(&loaded_tiers);
    ArrayDbSegmentationFreeBuffers();
}

static int CheckExistingSolvingTier(const char *caller) {
    if (current_game.tier != kIllegalTier) {
        fprintf(stderr,
                "%s: failed to create solving tier due "
                "to an existing solving tier\n",
                caller);
        return kRuntimeError;
    }

    return kSuccess;
}

static int ArrayDbCreateSolvingTier(Tier tier, int64_t size) {
    int error = CheckExistingSolvingTier("ArrayDbCreateSolvingTier");
    if (error) return error;

    // Initialize the solving tier's record array.
    solving.records = RecordArrayCreate(size);
    if (solving.records == NULL) return kMallocFailureError;
    if (!TierToPtrChainedHashMapSet(&loaded_tiers, tier, solving.records)) {
        RecordArrayDestroy(solving.records);
        solving.records = NULL;
        return kMallocFailureError;
    }
    current_game.tier = tier;
    solving.is_concurrent = false;

    return kSuccess;
}

static int ArrayDbCreateConcurrentSolvingTier(Tier tier, int64_t size) {
#ifdef _OPENMP
    int error = CheckExistingSolvingTier("ArrayDbCreateConcurrentSolvingTier");
    if (error) return error;

    solving.atomic_records = AtomicRecordArrayCreate(size);
    if (solving.atomic_records == NULL) return kMallocFailureError;
    if (!TierToPtrChainedHashMapSet(&loaded_tiers, tier,
                                    solving.atomic_records)) {
        AtomicRecordArrayDestroy(solving.atomic_records);
        solving.atomic_records = NULL;
        return kMallocFailureError;
    }
    current_game.tier = tier;
    solving.is_concurrent = true;

    return kSuccess;
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

static char *GetFullPathToTempSegment(Tier tier, GetTierNameFunc GetTierName,
                                      int seg_idx) {
    char postfix[14 + kInt32Base10StringLengthMax];
    sprintf(postfix, "_seg_%d.lz4.tmp", seg_idx);

    return GetFullPathPlusExtension(tier, GetTierName, postfix);
}

static char *GetFullPathToSegment(Tier tier, GetTierNameFunc GetTierName,
                                  int seg_idx) {
    char postfix[10 + kInt32Base10StringLengthMax];
    sprintf(postfix, "_seg_%d.lz4", seg_idx);

    return GetFullPathPlusExtension(tier, GetTierName, postfix);
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

static int FlushSolvingTierConcurrent(void) {
    // Allocate memory and create db file.
    int error = kSuccess;
    char *full_path =
        GetFullPathToFile(current_game.tier, current_game.GetTierName);
    char *tmp_full_path =
        GetFullPathToTempFile(current_game.tier, current_game.GetTierName);
    XzraCodecOptions xzra_options = {
        .block_size = lzma_options.block_size,
        .level = lzma_options.level,
        .extreme = lzma_options.extreme,
        .num_threads = ConcurrencyGetOmpNumThreads(),
    };
    XzraOutStream *xout = XzraOutStreamCreate(tmp_full_path, &xzra_options);
    void *buf = GamesmanMalloc(1ULL << 20);
    if (full_path == NULL || tmp_full_path == NULL || xout == NULL ||
        buf == NULL) {
        error = kMallocFailureError;
        goto _bailout;
    }

    // First compress to a temp file using streaming.
    size_t total = 0;
    size_t serialized = AtomicRecordArraySerializeStreaming(
        solving.atomic_records, 0, buf, sizeof(buf));
    while (serialized) {
        XzraStatus status = XzraOutStreamRun(xout, buf, serialized, NULL);
        if (status != XZRA_SUCCESS) {
            error = kRuntimeError;
            goto _bailout;
        }
        total += serialized;
        serialized = AtomicRecordArraySerializeStreaming(
            solving.atomic_records, total, buf, sizeof(buf));
    }
    if (XzraOutStreamClose(xout, NULL) != XZRA_SUCCESS) {
        error = kRuntimeError;
        goto _bailout;
    }
    xout = NULL;

    // If successful, rename the temp file into the desired tier DB name.
    int rename_error = GuardedRename(tmp_full_path, full_path);
    if (rename_error) {
        error = kFileSystemError;
        goto _bailout;
    }

_bailout:
    GamesmanFree(full_path);
    GamesmanFree(tmp_full_path);
    XzraOutStreamClose(xout, NULL);
    GamesmanFree(buf);

    return error;
}

static int FlushSolvingTierNormal(void) {
    // Create db file.
    int error = kSuccess;
    char *full_path =
        GetFullPathToFile(current_game.tier, current_game.GetTierName);
    char *tmp_full_path =
        GetFullPathToTempFile(current_game.tier, current_game.GetTierName);
    if (full_path == NULL || tmp_full_path == NULL) {
        error = kMallocFailureError;
        goto _bailout;
    }

    // First compress to a temp file.
    XzraCodecOptions xzra_options = {
        .block_size = lzma_options.block_size,
        .level = lzma_options.level,
        .extreme = lzma_options.extreme,
        .num_threads = ConcurrencyGetOmpNumThreads(),
    };
    XzraStatus status = XzraCompressMem(
        tmp_full_path, RecordArrayGetReadOnlyData(solving.records),
        RecordArrayGetRawSize(solving.records), &xzra_options, NULL);
    switch (status) {
        case XZRA_ERR_OUT_FILE:
            error = kFileSystemError;
            goto _bailout;
        case XZRA_ERR_CODEC:
            error = kRuntimeError;
            goto _bailout;
        default:
            break;
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

static int ArrayDbFlushSolvingTier(void *aux) {
    (void)aux;  // Unused.
    if (solving.is_concurrent) return FlushSolvingTierConcurrent();

    return FlushSolvingTierNormal();
}

static int ArrayDbFreeSolvingTier(void) {
    if (current_game.tier == kIllegalTier) return kSuccess;

    if (solving.is_concurrent) {
        AtomicRecordArrayDestroy(solving.atomic_records);
        solving.atomic_records = NULL;
    } else {
        RecordArrayDestroy(solving.records);
        solving.records = NULL;
    }
    TierToPtrChainedHashMapRemove(&loaded_tiers, current_game.tier);
    current_game.tier = kIllegalTier;

    return kSuccess;
}

static int ArrayDbSetGameSolved(void) {
    char *flag_filename = GetFullPathToFinishFlag();
    if (flag_filename == NULL) return kMallocFailureError;

    FILE *flag_file = GuardedFopen(flag_filename, "w");
    GamesmanFree(flag_filename);
    if (flag_file == NULL) return kFileSystemError;

    int error = GuardedFclose(flag_file);
    if (error != 0) return kFileSystemError;

    return kSuccess;
}

static int ArrayDbSetValue(Position position, Value value) {
    if (solving.is_concurrent) {
        AtomicRecordArraySetValue(solving.atomic_records, position, value);
    } else {
        RecordArraySetValue(solving.records, position, value);
    }

    return kSuccess;
}

static int ArrayDbSetRemoteness(Position position, int remoteness) {
    if (solving.is_concurrent) {
        AtomicRecordArraySetRemoteness(solving.atomic_records, position,
                                       remoteness);
    } else {
        RecordArraySetRemoteness(solving.records, position, remoteness);
    }

    return kSuccess;
}

static int ArrayDbSetValueRemoteness(Position position, Value value,
                                     int remoteness) {
    if (solving.is_concurrent) {
        AtomicRecordArraySetValueRemoteness(solving.atomic_records, position,
                                            value, remoteness);
    } else {
        RecordArraySetValueRemoteness(solving.records, position, value,
                                      remoteness);
    }

    return kSuccess;
}

static bool ArrayDbMaximizeValueRemoteness(Position position, Value value,
                                           int remoteness,
                                           int (*compare)(Value v1, int r1,
                                                          Value v2, int r2)) {
    if (solving.is_concurrent) {
        return AtomicRecordArrayMaximize(solving.atomic_records, position,
                                         value, remoteness, compare);
    }

    return RecordArrayMaximize(solving.records, position, value, remoteness,
                               compare);
}

static int ArrayDbDecrementNumUndecidedChildren(Position position) {
    if (solving.is_concurrent) {
        return AtomicRecordArrayDecrementNumUndecidedChildren(
            solving.atomic_records, position);
    }

    return RecordArrayDecrementNumUndecidedChildren(solving.records, position);
}

static int ArrayDbClearNumUndecidedChildren(Position position) {
    if (solving.is_concurrent) {
        return AtomicRecordArrayClearNumUndecidedChildren(
            solving.atomic_records, position);
    }

    return RecordArrayClearNumUndecidedChildren(solving.records, position);
}

static Value ArrayDbGetValue(Position position) {
    if (solving.is_concurrent) {
        return AtomicRecordArrayGetValue(solving.atomic_records, position);
    }

    return RecordArrayGetValue(solving.records, position);
}

static int ArrayDbGetRemoteness(Position position) {
    if (solving.is_concurrent) {
        return AtomicRecordArrayGetRemoteness(solving.atomic_records, position);
    }

    return RecordArrayGetRemoteness(solving.records, position);
}

static int ArrayDbGetNumUndecidedChildren(Position position) {
    if (solving.is_concurrent) {
        return AtomicRecordArrayGetNumUndecidedChildren(solving.atomic_records,
                                                        position);
    }

    return RecordArrayGetNumUndecidedChildren(solving.records, position);
}

static int ArrayDbSegmentationMaxNumBuffers(void) {
    return kNumSolvingSegmentsMax;
}

int ArrayDbSegmentationCreateBuffers(Tier tier, int num_segments,
                                     int64_t size) {
    if (num_segments < 0 || num_segments > kNumSolvingSegmentsMax) {
        return kIllegalArgumentError;
    }
    for (int i = 0; i < num_segments; ++i) {
        segments.array[i] = RecordArrayCreate(size);
        if (segments.array[i] == NULL) {
            ArrayDbSegmentationFreeBuffers();
            return kMallocFailureError;
        }
    }
    current_game.tier = tier;
    segments.num_active = num_segments;

    return kSuccess;
}

static int ConvertLz4UtilsDecompressFileError(Lz4UtilsStatus status) {
    switch (status) {
        case LZ4_UTILS_SUCCESS:
            return kSuccess;
        case LZ4_UTILS_ERR_INVALID_PARAM:
        case LZ4_UTILS_ERR_INSUFFICIENT_BUF:
            return kIllegalArgumentError;
        case LZ4_UTILS_ERR_OOM:
            return kMallocFailureError;
        case LZ4_UTILS_ERR_IO:
            return kFileSystemError;
        default:
            return kRuntimeError;
    }
}

static int ArrayDbSegmentationLoad(int buf_idx, int seg_idx) {
    if (buf_idx >= segments.num_active) return kIllegalArgumentError;

    char *filename = GetFullPathToSegment(current_game.tier,
                                          current_game.GetTierName, seg_idx);
    if (!filename) return kMallocFailureError;

    Lz4UtilsStatus lz4_utils_status = Lz4UtilsDecompressFileToBuffer(
        filename, segments.array[buf_idx]->records,
        segments.array[buf_idx]->size * sizeof(Record), NULL);
    GamesmanFree(filename);

    return ConvertLz4UtilsDecompressFileError(lz4_utils_status);
}

static int ArrayDbSegmentationFlush(int buf_idx, int seg_idx) {
    if (buf_idx >= segments.num_active) return kIllegalArgumentError;

    int error = kSuccess;
    char *tmp_name = GetFullPathToTempSegment(
        current_game.tier, current_game.GetTierName, seg_idx);
    char *name = GetFullPathToSegment(current_game.tier,
                                      current_game.GetTierName, seg_idx);
    if (tmp_name == NULL || name == NULL) {
        error = kMallocFailureError;
        goto _bailout;
    }

    Lz4UtilsStatus lz4_utils_status = Lz4UtilsCompressBufferToFile(
        segments.array[buf_idx]->records,
        segments.array[buf_idx]->size * sizeof(Record), kDefaultLz4Level,
        tmp_name, NULL);
    switch (lz4_utils_status) {
        case LZ4_UTILS_SUCCESS:
            break;
        case LZ4_UTILS_ERR_INVALID_PARAM:
            NotReached(
                "ArrayDbSegmentationFlush: (BUG) malformed input array(s)");
            break;
        case LZ4_UTILS_ERR_OOM:
            error = kMallocFailureError;
            goto _bailout;
        case LZ4_UTILS_ERR_IO:
            error = kFileSystemError;
            goto _bailout;
        default:
            error = kRuntimeError;
            goto _bailout;
    }
    int rename_error = GuardedRename(tmp_name, name);
    if (rename_error) {
        error = kFileSystemError;
        goto _bailout;
    }

_bailout:
    GamesmanFree(tmp_name);
    GamesmanFree(name);

    return error;
}

static int ArrayDbSegmentationFreeBuffers(void) {
    for (int i = 0; i < segments.num_active; ++i) {
        RecordArrayDestroy(segments.array[i]);
    }
    segments.num_active = 0;

    return kSuccess;
}

static Value ArrayDbSegmentationGetValue(int buf_idx, int64_t offset) {
    return RecordArrayGetValue(segments.array[buf_idx], offset);
}

static int ArrayDbSegmentationGetRemoteness(int buf_idx, int64_t offset) {
    return RecordArrayGetRemoteness(segments.array[buf_idx], offset);
}

static void ArrayDbSegmentationSetValueRemoteness(int buf_idx, int64_t offset,
                                                  Value value, int remoteness) {
    RecordArraySetValueRemoteness(segments.array[buf_idx], offset, value,
                                  remoteness);
}

static int64_t I64Min(int64_t a, int64_t b) { return a < b ? a : b; }

static bool RecompressDbChunk(int64_t tier_size, int slot, int chunk,
                              XzraOutStream *xzra_out) {
    Position begin_pos = chunk * segments.array[slot]->size;
    Position end_pos =
        I64Min(begin_pos + segments.array[slot]->size, tier_size);
    XzraStatus status = XzraOutStreamRun(
        xzra_out,
        (const uint8_t *)RecordArrayGetReadOnlyData(segments.array[slot]),
        (end_pos - begin_pos) * sizeof(Record), NULL);

    return status == XZRA_SUCCESS;
}

int ArrayDbSegmentationConsolidate(int64_t tier_size, int num_segments) {
    int error = kSuccess;
    char *tmp_full_path =
        GetFullPathToTempFile(current_game.tier, current_game.GetTierName);
    char *full_path =
        GetFullPathToFile(current_game.tier, current_game.GetTierName);
    if (tmp_full_path == NULL || full_path == NULL) {
        error = kMallocFailureError;
        goto _bailout;
    }

    XzraCodecOptions xzra_options = {
        .block_size = lzma_options.block_size,
        .level = lzma_options.level,
        .extreme = lzma_options.extreme,
        .num_threads = ConcurrencyGetOmpNumThreads() - 1,
    };
    XzraOutStream *xzra_out = XzraOutStreamCreate(tmp_full_path, &xzra_options);
    if (xzra_out == NULL) {
        error = kMallocFailureError;
        goto _bailout;
    }

    bool success = true;
    PRAGMA_OMP(parallel reduction(task, && : success))
    PRAGMA_OMP(single)
    for (int64_t i = 0; i < num_segments; ++i) {
        int slot = i % segments.num_active;

        // Read in a DB segment
        PRAGMA_OMP(task depend(inout : segments.array[slot]))
        ArrayDbSegmentationLoad(slot, i);

        // Recompress
        PRAGMA_OMP(task in_reduction(&& : success)
                       depend(inout : segments.array[slot]))
        success &= RecompressDbChunk(tier_size, slot, i, xzra_out);
    }
    if (XzraOutStreamClose(xzra_out, NULL) != XZRA_SUCCESS) {
        error = kFileSystemError;
        goto _bailout;
    } else if (!success) {
        error = kRuntimeError;
        goto _bailout;
    }

    // Rename tmp to regular db file.
    int rename_error = GuardedRename(tmp_full_path, full_path);
    if (rename_error) {
        error = kFileSystemError;
        goto _bailout;
    }

_bailout:
    GamesmanFree(tmp_full_path);
    GamesmanFree(full_path);

    return error;
}

bool ArrayDbCheckpointExists(Tier tier) {
    char *full_path = GetFullPathToCheckpoint(tier, current_game.GetTierName);
    bool ret = full_path && FileExists(full_path);
    GamesmanFree(full_path);

    return ret;
}

int ArrayDbCheckpointSave(const void *status, size_t status_size) {
    int error = kSuccess;
    char *full_path =
        GetFullPathToCheckpoint(current_game.tier, current_game.GetTierName);
    char *tmp_full_path = GetFullPathToTempCheckpoint(current_game.tier,
                                                      current_game.GetTierName);
    if (full_path == NULL || tmp_full_path == NULL) {
        error = kMallocFailureError;
        goto _bailout;
    }

    const void *inputs[] = {RecordArrayGetReadOnlyData(solving.records),
                            status};
    const size_t input_sizes[] = {RecordArrayGetRawSize(solving.records),
                                  status_size};
    Lz4UtilsStatus lz4_utils_status = Lz4UtilsCompressBuffersToFile(
        inputs, input_sizes, 2, kDefaultLz4Level, tmp_full_path, NULL);
    switch (lz4_utils_status) {
        case LZ4_UTILS_SUCCESS:
            break;
        case LZ4_UTILS_ERR_INVALID_PARAM:
            NotReached("ArrayDbCheckpointSave: (BUG) malformed input array(s)");
            break;
        case LZ4_UTILS_ERR_OOM:
            error = kMallocFailureError;
            goto _bailout;
        case LZ4_UTILS_ERR_IO:
            error = kFileSystemError;
            goto _bailout;
        default:
            error = kRuntimeError;
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
    if (current_game.tier != kIllegalTier) {
        fprintf(stderr,
                "ArrayDbCheckpointLoad: failed to load solving tier checkpoint "
                "due to an existing solving tier\n");
        return kRuntimeError;
    }

    // Initialize the solving tier's record array.
    solving.records = RecordArrayCreate(size);
    if (solving.records == NULL) return kMallocFailureError;

    // Get full path to the checkpoint file.
    char *full_path = GetFullPathToCheckpoint(tier, current_game.GetTierName);
    if (full_path == NULL) {
        RecordArrayDestroy(solving.records);
        return kMallocFailureError;
    }

    // Decompress the checkpoint file into the record array and status.
    void *out_buffers[] = {RecordArrayGetData(solving.records), status};
    size_t out_sizes[] = {RecordArrayGetRawSize(solving.records), status_size};
    Lz4UtilsStatus lz4_utils_status = Lz4UtilsDecompressFileToBuffers(
        full_path, out_buffers, out_sizes, 2, NULL);
    GamesmanFree(full_path);
    if (lz4_utils_status != LZ4_UTILS_SUCCESS) {
        RecordArrayDestroy(solving.records);
        return ConvertLz4UtilsDecompressFileError(lz4_utils_status);
    }

    // Add the solving tier's index to the map.
    if (!TierToPtrChainedHashMapSet(&loaded_tiers, tier, solving.records)) {
        RecordArrayDestroy(solving.records);
        return kMallocFailureError;
    }
    current_game.tier = tier;

    return kSuccess;
}

static int ArrayDbCheckpointRemove(Tier tier) {
    char *full_path = GetFullPathToCheckpoint(tier, current_game.GetTierName);
    int error = GuardedRemove(full_path);
    GamesmanFree(full_path);
    if (error != 0) return kFileSystemError;

    return kSuccess;
}

static size_t ArrayDbTierMemUsage(Tier tier, int64_t size) {
    (void)tier;
    return sizeof(Record) * size;
}

static size_t ArrayDbConcurrentTierMemUsage(Tier tier, int64_t size) {
    (void)tier;
    return sizeof(AtomicRecord) * size;
}

static int ArrayDbLoadTier(Tier tier, int64_t size) {
    RecordArray *load = RecordArrayCreate(size);
    if (load == NULL) return kMallocFailureError;

    char *full_path = GetFullPathToFile(tier, current_game.GetTierName);
    if (full_path == NULL) {
        RecordArrayDestroy(load);
        return kMallocFailureError;
    }

    XzraCodecOptions xzra_options = {
        .block_size = lzma_options.block_size,
        .level = lzma_options.level,
        .extreme = lzma_options.extreme,
        .num_threads = ConcurrencyGetOmpNumThreads(),
    };
    uint64_t mem = 0;
    XzraStatus status = XzraDecompressionMemUsage(&xzra_options, &mem);
    if (status != XZRA_SUCCESS) {
        RecordArrayDestroy(load);
        GamesmanFree(full_path);
        return kRuntimeError;
    }

    status = XzraDecompressFile(RecordArrayGetData(load), full_path,
                                size * sizeof(Record),
                                ConcurrencyGetOmpNumThreads(), mem, NULL);
    GamesmanFree(full_path);
    if (status != XZRA_SUCCESS) {
        RecordArrayDestroy(load);
        return kRuntimeError;
    }

    if (!TierToPtrChainedHashMapSet(&loaded_tiers, tier, load)) {
        RecordArrayDestroy(load);
        return kMallocFailureError;
    }

    return kSuccess;
}

static int ArrayDbUnloadTier(Tier tier) {
    TierToPtrChainedHashMapIterator it =
        TierToPtrChainedHashMapGet(&loaded_tiers, tier);
    if (TierToPtrChainedHashMapIteratorIsValid(&it)) {
        RecordArrayDestroy(
            (RecordArray *)TierToPtrChainedHashMapIteratorValue(&it));
        TierToPtrChainedHashMapRemove(&loaded_tiers, tier);
    }

    return kSuccess;
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
    if (tier == current_game.tier && solving.is_concurrent) {
        return AtomicRecordArrayGetValue(solving.atomic_records, position);
    }

    RecordArray *loaded = GetRecordsFromLoaded(tier);
    if (loaded == NULL) return kErrorValue;

    return RecordArrayGetValue(loaded, position);
}

static int ArrayDbGetRemotenessFromLoaded(Tier tier, Position position) {
    if (tier == current_game.tier && solving.is_concurrent) {
        return AtomicRecordArrayGetRemoteness(solving.atomic_records, position);
    }

    RecordArray *loaded = GetRecordsFromLoaded(tier);
    if (loaded == NULL) return kErrorValue;

    return RecordArrayGetRemoteness(loaded, position);
}

static int ArrayDbProbeInit(DbProbe *probe) {
    probe->buffer = GamesmanCallocWhole(1, sizeof(AdbProbeInternal));
    if (probe->buffer == NULL) return kMallocFailureError;

    probe->tier = kIllegalTier;
    // probe->begin and probe->size are unused.

    return kSuccess;
}

static int ArrayDbProbeDestroy(DbProbe *probe) {
    AdbProbeInternal *probe_internal = (AdbProbeInternal *)probe->buffer;
    XzraFileClose(probe_internal->file);
    GamesmanFree(probe->buffer);
    memset(probe, 0, sizeof(*probe));

    return kSuccess;
}

static bool ProbeSameFile(const DbProbe *probe, TierPosition tier_position) {
    return probe->tier == tier_position.tier;
}

static int ProbeLoadNewTier(DbProbe *probe, Tier tier) {
    AdbProbeInternal *probe_internal = (AdbProbeInternal *)probe->buffer;
    if (probe_internal->file) {
        int error = XzraFileClose(probe_internal->file);
        if (error != 0) return kRuntimeError;
        probe_internal->file = NULL;
    }

    char *full_path = GetFullPathToFile(tier, current_game.GetTierName);
    if (full_path == NULL) return kMallocFailureError;

    probe_internal->file = XzraFileOpen(full_path);
    GamesmanFree(full_path);
    if (probe_internal->file == NULL) return kFileSystemError;

    probe->tier = tier;
    return kSuccess;
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
        if (error != kSuccess) {
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
        if (error != kSuccess) {
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
    char *full_path = GetFullPathToFile(tier, current_game.GetTierName);
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

const char *ArrayDbGetPath(void) { return sandbox_path; }
