/**
 * @file tier_solver.c
 * @author Max Delgadillo: designed and implemented the original version
 * of tier solver (solveretrograde.c in GamesmanClassic.)
 * @author Robert Shi (robertyishi@berkeley.edu): Modularized, multithreaded
 * tier solver with various optimizations.
 *         GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Implementation of the generic tier solver.
 * @version 1.6.1
 * @date 2024-09-13
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

#include "core/solvers/tier_solver/tier_solver.h"

#include <assert.h>  // assert
#include <stddef.h>  // NULL
#include <stdint.h>  // int64_t, intptr_t
#include <stdio.h>   // fprintf, stderr
#include <stdlib.h>  // strtoll
#include <string.h>  // memset, memcpy, strncmp
#ifdef USE_MPI
#include <mpi.h>
#endif  // USE_MPI

#include "core/analysis/stat_manager.h"
#include "core/db/arraydb/arraydb.h"
#include "core/db/bpdb/bpdb_lite.h"
#include "core/db/db_manager.h"
#include "core/db/naivedb/naivedb.h"
#include "core/misc.h"
#include "core/solvers/tier_solver/tier_manager.h"
#include "core/solvers/tier_solver/tier_worker.h"
#include "core/types/gamesman_types.h"

// Solver API functions.

static int TierSolverInit(ReadOnlyString game_name, int variant,
                          const void *solver_api, ReadOnlyString data_path);
static int TierSolverFinalize(void);

static int TierSolverTest(long seed);
static ReadOnlyString TierSolverExplainTestError(int error);

static int TierSolverSolve(void *aux);
static int TierSolverAnalyze(void *aux);
static int TierSolverGetStatus(void);
static const SolverConfig *TierSolverGetCurrentConfig(void);
static int TierSolverSetOption(int option, int selection);
static Value TierSolverGetValue(TierPosition tier_position);
static int TierSolverGetRemoteness(TierPosition tier_position);

/** @brief Tier Solver definition. */
const Solver kTierSolver = {
    .name = "Tier Solver",
    .supports_mpi = 1,

    .Init = &TierSolverInit,
    .Finalize = &TierSolverFinalize,

    .Test = &TierSolverTest,
    .ExplainTestError = &TierSolverExplainTestError,

    .Solve = &TierSolverSolve,
    .Analyze = &TierSolverAnalyze,
    .GetStatus = &TierSolverGetStatus,

    .GetCurrentConfig = &TierSolverGetCurrentConfig,
    .SetOption = &TierSolverSetOption,

    .GetValue = &TierSolverGetValue,
    .GetRemoteness = &TierSolverGetRemoteness,
};

// Size of each uncompressed XZ block for ArrayDb compression. Smaller block
// sizes allows faster read of random tier positions at the cost of lower
// compression ratio.
static const int64_t kArrayDbBlockSize = 1 << 20;  // 1 MiB.

// Number of ArrayDb records in each uncompressed XZ block. This should be
// treated as a constant, although its value is calculated at runtime.
static int64_t kArrayDbRecordsPerBlock;

static ConstantReadOnlyString kChoices[] = {"On", "Off"};
static const SolverOption kTierSymmetryRemoval = {
    .name = "Tier Symmetry Removal",
    .num_choices = 2,
    .choices = kChoices,
};

static const SolverOption kPositionSymmetryRemoval = {
    .name = "Position Symmetry Removal",
    .num_choices = 2,
    .choices = kChoices,
};

static const SolverOption kUseRetrograde = {
    .name = "Use Retrograde Analysis",
    .num_choices = 2,
    .choices = kChoices,
};

// Backup of the original API functions. If the user decides to turn off some
// settings and then turn them back on, those functions will be read from this
// object.
static TierSolverApi default_api;

// The API currently being used.
static TierSolverApi current_api;

// Solver settings for external use
static SolverConfig current_config;
static int num_options;
#define NUM_OPTIONS_MAX 4  // At most 3 options and 1 zero-terminator.
static SolverOption current_options[NUM_OPTIONS_MAX];
static int current_selections[NUM_OPTIONS_MAX];
#undef NUM_OPTIONS_MAX

// Whether the current game was solved and stored with an older database
// version. Solving, including force re-solving, are disabled to prevent
// damage to the old database.
static bool read_only_db;

// Solver status: 0 if not solved, 1 if solved.
static int solver_status;

// Helper Functions

static bool RequiredApiFunctionsImplemented(const TierSolverApi *api);
static bool TierSymmetryRemovalImplemented(const TierSolverApi *api);
static bool PositionSymmetryRemovalImplemented(const TierSolverApi *api);
static bool RetrogradeAnalysisImplemented(const TierSolverApi *api);

static void ToggleTierSymmetryRemoval(bool on);
static void TogglePositionSymmetryRemoval(bool on);
static void ToggleRetrogradeAnalysis(bool on);

static bool SetCurrentApi(const TierSolverApi *api);

static int SetDb(ReadOnlyString game_name, int variant,
                 ReadOnlyString data_path);

static TierPosition GetCanonicalTierPosition(TierPosition tier_position);

// Default API functions

static TierType DefaultGetTierType(Tier tier);
static Tier DefaultGetCanonicalTier(Tier tier);
static Position DefaultGetPositionInSymmetricTier(TierPosition tier_position,
                                                  Tier symmetric);
static Position DefaultGetCanonicalPosition(TierPosition tier_position);
static int DefaultGetNumberOfCanonicalChildPositions(
    TierPosition tier_position);
static TierPositionArray DefaultGetCanonicalChildPositions(
    TierPosition tier_position);
static int DefaultGetTierName(Tier tier,
                              char name[static kDbFileNameLengthMax + 1]);

// -----------------------------------------------------------------------------

static int TierSolverInit(ReadOnlyString game_name, int variant,
                          const void *solver_api, ReadOnlyString data_path) {
    kArrayDbRecordsPerBlock = kArrayDbBlockSize / kArrayDbRecordSize;
    read_only_db = false;
    solver_status = kTierSolverSolveStatusNotSolved;

    int error = -1;
    bool success = SetCurrentApi((const TierSolverApi *)solver_api);
    if (!success) goto _bailout;
    error = SetDb(game_name, variant, data_path);
    if (error != kNoError) goto _bailout;

    error = StatManagerInit(game_name, variant, data_path);
    if (error != kNoError) goto _bailout;

    // Success.
    error = 0;

_bailout:
    if (error != kNoError) {
        DbManagerFinalizeDb();
        StatManagerFinalize();
        TierSolverFinalize();
    }
    return error;
}

static int TierSolverFinalize(void) {
    read_only_db = false;
    solver_status = kTierSolverSolveStatusNotSolved;
    DbManagerFinalizeDb();
    memset(&default_api, 0, sizeof(default_api));
    memset(&current_api, 0, sizeof(current_api));
    memset(&current_config, 0, sizeof(current_config));
    memset(&current_options, 0, sizeof(current_options));
    memset(&current_selections, 0, sizeof(current_selections));
    num_options = 0;

    return kNoError;
}

static int TierSolverTest(long seed) {
    TierWorkerInit(&current_api, kArrayDbRecordsPerBlock);
    printf(
        "Enter the number of positions to test in each tier [Default: 1000]: ");
    char input[kInt64Base10StringLengthMax + 1];
    int64_t test_size = 1000;
    if (fgets(input, sizeof(input), stdin) != NULL) {
        // Check if the user pressed Enter without entering a number
        if (input[0] != '\n') {
            test_size = strtoll(input, NULL, 10);
            if (test_size < 0) {
                printf("Invalid input. Using default test size [1000]\n");
                test_size = 1000;
            }
        }
    }

    return TierManagerTest(&current_api, seed, test_size);
}

static ReadOnlyString TierSolverExplainTestError(int error) {
    switch (error) {
        case kTierSolverTestNoError:
            return "no error";
        case kTierSolverTestDependencyError:
            return "another error occurred before the test begins";
        case kTierSolverTestGetTierNameError:
            return "error reported from game-specific GetTierName function";
        case kTierSolverTestIllegalChildTierError:
            return "an child tier position was found to be in a tier that is "
                   "not in the list of child tiers generated by the "
                   "TierSolverApi::GetChildTiers function";
        case kTierSolverTestIllegalChildPosError:
            return "an illegal position was found to be a child position of "
                   "some legal position";
        case kTierSolverTestGetCanonicalChildPositionsMismatch:
            return "the canonical child positions returned by the "
                   "game-specific GetCanonicalChildPositions did not match "
                   "those returned by the default function which calls "
                   "GenerateMoves and DoMove";
        case kTierSolverTestGetNumberOfCanonicalChildPositionsMismatch:
            return "the number of canonical positions returned by the "
                   "game-specific GetNumberOfCanonicalChildPositions did not "
                   "match the value returned by the default function which "
                   "calls GenerateMoves and DoMove.";
        case kTierSolverTestTierSymmetrySelfMappingError:
            return "applying tier symmetry within the same tier returned a "
                   "different position";
        case kTierSolverTestTierSymmetryInconsistentError:
            return "applying tier symmetry twice - first using a symmetric "
                   "tier, then using the original tier - returned a different "
                   "position";
        case kTierSolverTestChildParentMismatchError:
            return "one of the canonical child positions of a legal canonical "
                   "position was found not to have that legal position as its "
                   "parent";
        case kTierSolverTestParentChildMismatchError:
            return "one of the canonical parent positions of a legal canonical "
                   "position was found not to have that legal position as its "
                   "child";
    }

    return "unknown error, which usually indicates a bug in the tier solver "
           "test code";
}

static ConstantReadOnlyString kTierSolverSolveSkipReadOnlyMsg =
    "TierSolverSolve: the current game was solved with a database of a "
    "previous version that is no longer supported. The solver has "
    "skipped the solving process to prevent damage to the existing "
    "database. To re-solve the current game, remove the old database "
    "or use a different data path and try again.";

static ConstantReadOnlyString kTierSolverAnalyzeSkipReadOnlyMsg =
    "TierSolverAnalyze: the current game was solved with a database of a "
    "previous version that is no longer supported. The solver has skipped the "
    "analysis because some functions are missing from the original database "
    "implementation. To analyze the current game, remove the old database "
    "or use a different data path to resolve the game and try again.";

static ConstantReadOnlyString kTierSolverSolveSkipSolvedMsg =
    "TierSolverSolve: the current game variant has already been solved. Use -f "
    "in headless mode to force re-solve the game variant.";

static int TierSolverSolve(void *aux) {
    if (read_only_db) {  // Skip solving if database is in read-only mode.
        printf("%s\n", kTierSolverSolveSkipReadOnlyMsg);
        return kNoError;
    }

    TierSolverSolveOptions default_options = {
        .force = false,
        .verbose = 1,
        .memlimit = 0,  // Use default memory limit.
    };
    const TierSolverSolveOptions *options = (TierSolverSolveOptions *)aux;
    if (options == NULL) options = &default_options;
    if (!options->force && solver_status == kTierSolverSolveStatusSolved) {
        printf("%s\n", kTierSolverSolveSkipSolvedMsg);
        return kNoError;
    }
#ifndef USE_MPI  // If not using MPI
    TierWorkerInit(&current_api, kArrayDbRecordsPerBlock, options->memlimit);
    return TierManagerSolve(&current_api, options->force, options->verbose);
#else   // Using MPI
    // Assumes MPI_Init or MPI_Init_thread has been called.
    int process_id, cluster_size;
    cluster_size = SafeMpiCommSize(MPI_COMM_WORLD);
    process_id = SafeMpiCommRank(MPI_COMM_WORLD);
    if (cluster_size < 1) {
        NotReached("TierSolverSolve: cluster size smaller than 1");
    } else if (cluster_size == 1) {  // Only one node is allocated.
        TierWorkerInit(&current_api, kArrayDbRecordsPerBlock,
                       options->memlimit);
        return TierManagerSolve(&current_api, options->force, options->verbose);
    } else {                    // cluster_size > 1
        if (process_id == 0) {  // This is the manager node.
            return TierManagerSolve(&current_api, options->force,
                                    options->verbose);
        } else {  // This is a worker node.
            TierWorkerInit(&current_api, kArrayDbRecordsPerBlock,
                           options->memlimit);
            return TierWorkerMpiServe();
        }
    }

    return kNotReachedError;
#endif  // USE_MPI
}

static int TierSolverAnalyze(void *aux) {
    // Now allowing analysis on old databases now for simplicity.
    // Need to work on old db implementation to support new analyzer API calls.
    if (read_only_db) {
        printf("%s\n", kTierSolverAnalyzeSkipReadOnlyMsg);
        return kNoError;
    }

    static const TierSolverAnalyzeOptions kDefaultAnalyzeOptions = {
        .force = false,
        .verbose = 1,
    };
    const TierSolverAnalyzeOptions *options = (TierSolverAnalyzeOptions *)aux;
    if (options == NULL) options = &kDefaultAnalyzeOptions;
    return TierManagerAnalyze(&current_api, options->force, options->verbose);
}

static int TierSolverGetStatus(void) { return solver_status; }

static const SolverConfig *TierSolverGetCurrentConfig(void) {
    return &current_config;
}

static int TierSolverSetOption(int option, int selection) {
    if (option >= num_options) {
        fprintf(stderr,
                "TierSolverSetOption: (BUG) option index out of bounds. "
                "Aborting...\n");
        return kIllegalSolverOptionError;
    }
    if (selection < 0 || selection > 1) {
        fprintf(stderr,
                "TierSolverSetOption: (BUG) selection index out of bounds. "
                "Aborting...\n");
        return kIllegalSolverOptionError;
    }

    current_selections[option] = selection;
    if (strncmp(current_options[option].name, "Tier Symmetry Removal",
                kSolverOptionNameLengthMax) == 0) {
        ToggleTierSymmetryRemoval(!selection);
    } else if (strncmp(current_options[option].name,
                       "Position Symmetry Removal",
                       kSolverOptionNameLengthMax) == 0) {
        TogglePositionSymmetryRemoval(!selection);
    } else {
        ToggleRetrogradeAnalysis(!selection);
    }

    return kNoError;
}

static Value TierSolverGetValue(TierPosition tier_position) {
    TierPosition canonical = GetCanonicalTierPosition(tier_position);
    DbProbe probe;
    int error = DbManagerProbeInit(&probe);
    if (error != 0) {
        NotReached(
            "RegularSolverGetRemoteness: failed to initialize DbProbe, most "
            "likely ran out of memory");
    }
    Value ret = DbManagerProbeValue(&probe, canonical);
    DbManagerProbeDestroy(&probe);
    return ret;
}

static int TierSolverGetRemoteness(TierPosition tier_position) {
    TierPosition canonical = GetCanonicalTierPosition(tier_position);
    DbProbe probe;
    int error = DbManagerProbeInit(&probe);
    if (error != 0) {
        NotReached(
            "RegularSolverGetRemoteness: failed to initialize DbProbe, most "
            "likely ran out of memory");
    }
    int ret = DbManagerProbeRemoteness(&probe, canonical);
    DbManagerProbeDestroy(&probe);
    return ret;
}

// Helper functions

static bool RequiredApiFunctionsImplemented(const TierSolverApi *api) {
    if (api->GetInitialTier == NULL) return false;
    if (api->GetInitialTier() < 0) return false;
    if (api->GetInitialPosition == NULL) return false;
    if (api->GetInitialPosition() < 0) return false;

    if (api->GetTierSize == NULL) return false;
    if (api->GenerateMoves == NULL) return false;
    if (api->Primitive == NULL) return false;
    if (api->DoMove == NULL) return false;
    if (api->IsLegalPosition == NULL) return false;

    if (api->GetChildTiers == NULL) return false;

    return true;
}

static bool TierSymmetryRemovalImplemented(const TierSolverApi *api) {
    if (api->GetCanonicalTier == NULL) return false;
    if (api->GetPositionInSymmetricTier == NULL) return false;

    return true;
}

static bool PositionSymmetryRemovalImplemented(const TierSolverApi *api) {
    return (api->GetCanonicalPosition != NULL);
}

static bool RetrogradeAnalysisImplemented(const TierSolverApi *api) {
    return (api->GetCanonicalParentPositions != NULL);
}

static void ToggleTierSymmetryRemoval(bool on) {
    if (on) {
        // This function should not be used to turn on Tier Symmetry Removal if
        // it's not an option.
        assert(default_api.GetCanonicalTier != NULL);
        assert(default_api.GetPositionInSymmetricTier != NULL);
        current_api.GetCanonicalTier = default_api.GetCanonicalTier;
        current_api.GetPositionInSymmetricTier =
            default_api.GetPositionInSymmetricTier;
    } else {
        current_api.GetCanonicalTier = &DefaultGetCanonicalTier;
        current_api.GetPositionInSymmetricTier =
            &DefaultGetPositionInSymmetricTier;
    }
}

static void TogglePositionSymmetryRemoval(bool on) {
    if (on) {
        // This function should not be used to turn on Position Symmetry Removal
        // if it's not an option.
        assert(default_api.GetCanonicalPosition != NULL);
        current_api.GetCanonicalPosition = default_api.GetCanonicalPosition;
    } else {
        current_api.GetCanonicalPosition = &DefaultGetCanonicalPosition;
    }
}

static void ToggleRetrogradeAnalysis(bool on) {
    if (on) {
        // This function should not be used to turn on Retrograde Analysis
        // if it's not an option.
        assert(default_api.GetCanonicalParentPositions != NULL);
        current_api.GetCanonicalParentPositions =
            default_api.GetCanonicalParentPositions;
    } else {
        current_api.GetCanonicalParentPositions = NULL;
    }
}

static bool SetCurrentApi(const TierSolverApi *api) {
    if (!RequiredApiFunctionsImplemented(api)) return false;
    memcpy(&default_api, api, sizeof(default_api));
    memcpy(&current_api, api, sizeof(current_api));

    // Assuming solver options and configuration objects, both external and
    // internal, are all zero-initialized before this function is called.
    if (TierSymmetryRemovalImplemented(&current_api)) {
        current_options[num_options++] = kTierSymmetryRemoval;
    } else {
        ToggleTierSymmetryRemoval(false);
    }

    if (PositionSymmetryRemovalImplemented(&current_api)) {
        current_options[num_options++] = kPositionSymmetryRemoval;
    } else {
        TogglePositionSymmetryRemoval(false);
    }

    if (RetrogradeAnalysisImplemented(&current_api)) {
        current_options[num_options++] = kUseRetrograde;
    }  // else, current_api.GetCanonicalParentPositions remains NULL.

    if (current_api.GetNumberOfCanonicalChildPositions == NULL) {
        current_api.GetNumberOfCanonicalChildPositions =
            &DefaultGetNumberOfCanonicalChildPositions;
    }

    if (current_api.GetTierType == NULL) {
        current_api.GetTierType = &DefaultGetTierType;
    }

    if (current_api.GetCanonicalChildPositions == NULL) {
        current_api.GetCanonicalChildPositions =
            &DefaultGetCanonicalChildPositions;
    }

    if (current_api.GetTierName == NULL) {
        current_api.GetTierName = &DefaultGetTierName;
    }

    return true;
}

static int SetDb(ReadOnlyString game_name, int variant,
                 ReadOnlyString data_path) {
    // Look for existing bpdb_lite database.
    int error = DbManagerInitDb(&kBpdbLite, true, game_name, variant, data_path,
                                current_api.GetTierName, NULL);
    if (error != kNoError) return error;
    int bpdb_status = DbManagerGameStatus();
    if (bpdb_status == kDbGameStatusCheckError) return kRuntimeError;
    if (bpdb_status == kDbGameStatusSolved) {
        read_only_db = true;
        solver_status = kTierSolverSolveStatusSolved;
        return kNoError;
    }
    DbManagerFinalizeDb();

    // Initialize a R/W array database.
    error = DbManagerInitDb(&kArrayDb, false, game_name, variant, data_path,
                            current_api.GetTierName, NULL);
    if (error != kNoError) return error;
    int arraydb_status = DbManagerGameStatus();
    if (arraydb_status == kDbGameStatusCheckError) return kRuntimeError;
    solver_status = (arraydb_status == kDbGameStatusSolved);

    return kNoError;
}

static TierPosition GetCanonicalTierPosition(TierPosition tier_position) {
    TierPosition canonical;

    // Convert to the tier position inside the canonical tier.
    canonical.tier = current_api.GetCanonicalTier(tier_position.tier);
    if (canonical.tier == tier_position.tier) {  // Original tier is canonical.
        canonical.position = tier_position.position;
    } else {  // Original tier is not canonical.
        canonical.position = current_api.GetPositionInSymmetricTier(
            tier_position, canonical.tier);
    }

    // Find the canonical position inside the canonical tier.
    canonical.position = current_api.GetCanonicalPosition(canonical);

    return canonical;
}

// Default API functions

static TierType DefaultGetTierType(Tier tier) {
    (void)tier;  // Unused.
    return kTierTypeLoopy;
}

static Tier DefaultGetCanonicalTier(Tier tier) { return tier; }

static Position DefaultGetPositionInSymmetricTier(TierPosition tier_position,
                                                  Tier symmetric) {
    (void)symmetric;  // Unused.
    assert(tier_position.tier == symmetric);

    return tier_position.position;
}

static Position DefaultGetCanonicalPosition(TierPosition tier_position) {
    return tier_position.position;
}

static int DefaultGetNumberOfCanonicalChildPositions(
    TierPosition tier_position) {
    //
    TierPositionHashSet children;
    TierPositionHashSetInit(&children, 0.5);

    MoveArray moves = current_api.GenerateMoves(tier_position);
    for (int64_t i = 0; i < moves.size; ++i) {
        TierPosition child = current_api.DoMove(tier_position, moves.array[i]);
        child = GetCanonicalTierPosition(child);
        if (!TierPositionHashSetContains(&children, child)) {
            TierPositionHashSetAdd(&children, child);
        }
    }
    MoveArrayDestroy(&moves);
    int num_children = (int)children.size;
    TierPositionHashSetDestroy(&children);

    return num_children;
}

static TierPositionArray DefaultGetCanonicalChildPositions(
    TierPosition tier_position) {
    //
    TierPositionHashSet deduplication_set;
    TierPositionHashSetInit(&deduplication_set, 0.5);

    TierPositionArray children;
    TierPositionArrayInit(&children);

    MoveArray moves = current_api.GenerateMoves(tier_position);
    for (int64_t i = 0; i < moves.size; ++i) {
        TierPosition child = current_api.DoMove(tier_position, moves.array[i]);
        child = GetCanonicalTierPosition(child);
        if (!TierPositionHashSetContains(&deduplication_set, child)) {
            TierPositionHashSetAdd(&deduplication_set, child);
            TierPositionArrayAppend(&children, child);
        }
    }
    MoveArrayDestroy(&moves);
    TierPositionHashSetDestroy(&deduplication_set);

    return children;
}

static int DefaultGetTierName(Tier tier,
                              char name[static kDbFileNameLengthMax + 1]) {
    sprintf(name, "%" PRITier, tier);

    return kNoError;
}
