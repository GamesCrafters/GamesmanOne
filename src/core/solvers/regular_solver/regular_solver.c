/**
 * @file regular_solver.c
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Implementation of the Regular Solver.
 * @details The Regular Solver is implemented as a single-tier special case of
 * the Tier Solver, which is why the Tier Solver Worker Module is used in this
 * file.
 * @version 2.2.0
 * @date 2025-05-11
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

#include "core/solvers/regular_solver/regular_solver.h"

#include <assert.h>   // assert
#include <stdbool.h>  // bool, true, false
#include <stddef.h>   // NULL
#include <stdint.h>   // int64_t
#include <stdio.h>    // fprintf, stderr
#include <stdlib.h>   // strtoll
#include <string.h>   // memset

#include "core/analysis/stat_manager.h"
#include "core/constants.h"
#include "core/db/arraydb/arraydb.h"
#include "core/db/db_manager.h"
#include "core/db/naivedb/naivedb.h"
#include "core/gamesman_memory.h"
#include "core/misc.h"
#include "core/solvers/tier_solver/tier_analyzer.h"
#include "core/solvers/tier_solver/tier_solver.h"
#include "core/solvers/tier_solver/tier_worker.h"
#include "core/solvers/tier_solver/tier_worker/test.h"
#include "core/types/gamesman_types.h"

// Solver API functions.

static int RegularSolverInit(ReadOnlyString game_name, int variant,
                             const void *solver_api, ReadOnlyString data_path);
static int RegularSolverFinalize(void);

static int RegularSolverTest(void *aux);
static ReadOnlyString RegularSolverExplainTestError(int error);

static int RegularSolverSolve(void *aux);
static int RegularSolverAnalyze(void *aux);
static int RegularSolverGetStatus(void);
static const SolverConfig *RegularSolverGetCurrentConfig(void);
static int RegularSolverSetOption(int option, int selection);
static Value RegularSolverGetValue(TierPosition tier_position);
static int RegularSolverGetRemoteness(TierPosition tier_position);

/** @brief Regular Solver definition. */
const Solver kRegularSolver = {
    .name = "Regular Solver",
    .supports_mpi = 0,

    .Init = &RegularSolverInit,
    .Finalize = &RegularSolverFinalize,

    .Test = &RegularSolverTest,
    .ExplainTestError = &RegularSolverExplainTestError,

    .Solve = &RegularSolverSolve,
    .Analyze = &RegularSolverAnalyze,
    .GetStatus = &RegularSolverGetStatus,

    .GetCurrentConfig = &RegularSolverGetCurrentConfig,
    .SetOption = &RegularSolverSetOption,

    .GetValue = &RegularSolverGetValue,
    .GetRemoteness = &RegularSolverGetRemoteness,
};

static ConstantReadOnlyString kChoices[] = {"On", "Off"};
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

// Size of each uncompressed XZ block for ArrayDb compression. Smaller block
// sizes allows faster read of random tier positions at the cost of lower
// compression ratio.
static const int64_t kArrayDbBlockSize = 1LL << 20;  // 1 MiB.

// Number of ArrayDb records in each uncompressed XZ block. This should be
// treated as a constant, although its value is calculated at runtime.
static int64_t kArrayDbRecordsPerBlock;

// Copy of the original RegularSolverApi object.
static RegularSolverApi original_api;

// Backup of the default API functions. If the user decides to turn off some
// settings and then turn them back on, those functions will be read from this
// object.
static TierSolverApi default_api;

// The API currently being used.
static TierSolverApi current_api;

// Solver settings for external use
static SolverConfig current_config;
static int num_options;
#define NUM_OPTIONS_MAX 3  // At most 2 options and 1 zero-terminator.
static SolverOption current_options[NUM_OPTIONS_MAX];
static int current_selections[NUM_OPTIONS_MAX];
#undef NUM_OPTIONS_MAX

static ReadOnlyString current_game_name;
static int current_variant_id = kIllegalVariantIndex;

// Helper Functions

static bool RequiredApiFunctionsImplemented(const RegularSolverApi *api);
static bool PositionSymmetryRemovalImplemented(const RegularSolverApi *api);
static bool RetrogradeAnalysisImplemented(const RegularSolverApi *api);

static void ConvertApi(const RegularSolverApi *regular, TierSolverApi *tier);

static void TogglePositionSymmetryRemoval(bool on);
static void ToggleRetrogradeAnalysis(bool on);

static bool SetCurrentApi(const RegularSolverApi *api);

// Converted Tier Solver API Variables and Functions

static Tier GetInitialTier(void);
static Position TierGetInitialPosition(void);

static int64_t GetTierSize(Tier tier);
static int TierGenerateMoves(TierPosition tier_position,
                             Move moves[static kTierSolverNumMovesMax]);
static Value TierPrimitive(TierPosition tier_position);
static TierPosition TierDoMove(TierPosition tier_position, Move move);
static bool TierIsLegalPosition(TierPosition tier_position);
static Position TierGetCanonicalPosition(TierPosition tier_position);
static int TierGetNumberOfCanonicalChildPositions(TierPosition tier_position);
static int TierGetCanonicalChildPositions(
    TierPosition tier_position,
    TierPosition children[static kTierSolverNumChildPositionsMax]);
static int TierGetCanonicalParentPositions(
    TierPosition tier_position, Tier parent_tier,
    Position parents[static kTierSolverNumParentPositionsMax]);
static int GetChildTiers(Tier tier,
                         Tier children[static kTierSolverNumChildTiersMax]);
static Tier GetCanonicalTier(Tier tier);

// Default API Functions.

static Position DefaultGetCanonicalPosition(TierPosition tier_position);
static int DefaultGetNumberOfCanonicalChildPositions(
    TierPosition tier_position);
static int DefaultGetCanonicalChildPositions(
    TierPosition tier_position,
    TierPosition children[static kRegularSolverNumChildPositionsMax]);

static TierType DefaultGetTierType(Tier tier);
static Position DefaultGetPositionInSymmetricTier(TierPosition tier_position,
                                                  Tier symmetric);
static int DefaultGetTierName(Tier tier,
                              char name[static kDbFileNameLengthMax + 1]);

// -----------------------------------------------------------------------------

static int RegularSolverInit(ReadOnlyString game_name, int variant,
                             const void *solver_api, ReadOnlyString data_path) {
    kArrayDbRecordsPerBlock = kArrayDbBlockSize / kArrayDbRecordSize;
    int ret = -1;
    bool success = SetCurrentApi((const RegularSolverApi *)solver_api);
    if (!success) goto _bailout;

    ret = DbManagerInitDb(&kArrayDb, false, game_name, variant, data_path,
                          &DefaultGetTierName, NULL);
    if (ret != 0) goto _bailout;

    ret = StatManagerInit(game_name, variant, data_path);
    if (ret != 0) goto _bailout;

    // Success.
    current_game_name = game_name;
    current_variant_id = variant;
    ret = 0;

_bailout:
    if (ret != 0) {
        DbManagerFinalizeDb();
        StatManagerFinalize();
        RegularSolverFinalize();
    }
    return ret;
}

static int RegularSolverFinalize(void) {
    memset(&original_api, 0, sizeof(original_api));
    memset(&default_api, 0, sizeof(default_api));
    memset(&current_api, 0, sizeof(current_api));
    memset(&current_config, 0, sizeof(current_config));
    memset(&current_options, 0, sizeof(current_options));
    memset(&current_selections, 0, sizeof(current_selections));
    current_game_name = NULL;
    current_variant_id = kIllegalVariantIndex;
    num_options = 0;

    return kNoError;
}

static int RegularSolverTest(void *aux) {
    RegularSolverTestOptions default_options = {
        .seed = (long)time(NULL), .test_size = 1000000, .verbose = 1};
    RegularSolverTestOptions *options = (RegularSolverTestOptions *)aux;
    if (!options) options = &default_options;

    TierWorkerInit(&current_api, kArrayDbRecordsPerBlock, 0);
    TierArray empty;
    TierArrayInit(&empty);
    TierWorkerTestStackBufferStat *stat = TierWorkerTestStackBufferStatCreate();
    int ret = TierWorkerTest(kDefaultTier, &empty, options->seed,
                             options->test_size, stat);
    TierArrayDestroy(&empty);
    TierWorkerTestStackBufferStatPrint(stat);
    TierWorkerTestStackBufferStatDestroy(stat);

    return ret;
}

enum RegularSolverTestErrors {
    /** No error. */
    kRegularSolverTestNoError = kTierSolverTestNoError,
    /** Test failed due to a prior error. */
    kRegularSolverTestDependencyError = kTierSolverTestDependencyError,
    /** Illegal child position detected. */
    kRegularSolverTestIllegalChildPosError =
        kTierSolverTestIllegalChildPosError,
    /** One of the canonical child positions of a legal canonical position was
       found not to have that legal position as its parent. */
    kRegularSolverTestChildParentMismatchError =
        kTierSolverTestChildParentMismatchError,
    /** One of the canonical parent positions of a legal canonical position was
       found not to have that legal position as its child. */
    kRegularSolverTestParentChildMismatchError =
        kTierSolverTestParentChildMismatchError,
};

static ReadOnlyString RegularSolverExplainTestError(int error) {
    switch (error) {
        case kRegularSolverTestNoError:
            return "no error";
        case kRegularSolverTestDependencyError:
            return "another error occurred before the test begins";
        case kRegularSolverTestIllegalChildPosError:
            return "an illegal position was found to be a child position of "
                   "some legal position";
        case kRegularSolverTestChildParentMismatchError:
            return "one of the canonical child positions of a legal canonical "
                   "position was found not to have that legal position as its "
                   "parent";
        case kRegularSolverTestParentChildMismatchError:
            return "one of the canonical parent positions of a legal canonical "
                   "position was found not to have that legal position as its "
                   "child";
    }

    return "unknown error, which usually indicates a bug in the regular "
           "solver's test code";
}

static int RegularSolverSolve(void *aux) {
    RegularSolverSolveOptions default_options = {
        .force = false,
        .verbose = 1,
        .memlimit = 0,  // Use default memory limit.
    };
    const RegularSolverSolveOptions *options =
        (const RegularSolverSolveOptions *)aux;
    if (options == NULL) options = &default_options;
    TierWorkerInit(&current_api, kArrayDbRecordsPerBlock, options->memlimit);

    TierWorkerSolveOptions tier_worker_options = {
        .compare = false,
        .force = options->force,
        .verbose = options->verbose,
    };
    int error = TierWorkerSolve(kTierWorkerSolveMethodValueIteration,
                                kDefaultTier, &tier_worker_options, NULL);
    if (error != kNoError) {
        fprintf(stderr, "RegularSolverSolve: solve failed with code %d\n",
                error);
        return error;
    }

    error = DbManagerSetGameSolved();
    if (error != kNoError) {
        fprintf(stderr,
                "RegularSolverSolve: DB manager failed to set current game as "
                "solved (code %d)\n",
                error);
    }

    return error;
}

static int RegularSolverAnalyze(void *aux) {
    static const RegularSolverAnalyzeOptions kDefaultAnalyzeOptions = {
        .force = false,
        .verbose = 1,
        .memlimit = 0,
    };

    Analysis *analysis = (Analysis *)GamesmanMalloc(sizeof(Analysis));
    if (analysis == NULL) return kMallocFailureError;
    AnalysisInit(analysis);

    const RegularSolverAnalyzeOptions *options =
        (const RegularSolverAnalyzeOptions *)aux;
    if (options == NULL) options = &kDefaultAnalyzeOptions;
    TierAnalyzerInit(&current_api, options->memlimit);
    int error = TierAnalyzerAnalyze(analysis, kDefaultTier, options->force);
    TierAnalyzerFinalize();
    if (error == 0 && options->verbose > 0) {
        printf("\n--- Game analyzed ---\n");
        AnalysisPrintEverything(stdout, analysis);
    } else {
        fprintf(stderr, "RegularSolverAnalyze: failed with code %d\n", error);
    }

    GamesmanFree(analysis);
    return error;
}

static int RegularSolverGetStatus(void) {
    return DbManagerTierStatus(kDefaultTier);
}

static const SolverConfig *RegularSolverGetCurrentConfig(void) {
    return &current_config;
}

static int RegularSolverSetOption(int option, int selection) {
    if (option >= num_options) {
        fprintf(stderr,
                "RegularSolverSetOption: (BUG) option index out of bounds. "
                "Aborting...\n");
        return kIllegalSolverOptionError;
    }
    if (selection < 0 || selection > 1) {
        fprintf(stderr,
                "RegularSolverSetOption: (BUG) selection index out of bounds. "
                "Aborting...\n");
        return kIllegalSolverOptionError;
    }

    current_selections[option] = selection;
    if (strncmp(current_options[option].name, "Position Symmetry Removal",
                kSolverOptionNameLengthMax) == 0) {
        TogglePositionSymmetryRemoval(!selection);
    } else {
        ToggleRetrogradeAnalysis(!selection);
    }
    return kNoError;
}

static Value RegularSolverGetValue(TierPosition tier_position) {
    TierPosition canonical = {
        .tier = kDefaultTier,
        .position = current_api.GetCanonicalPosition(tier_position),
    };
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

static int RegularSolverGetRemoteness(TierPosition tier_position) {
    TierPosition canonical = {
        .tier = kDefaultTier,
        .position = current_api.GetCanonicalPosition(tier_position),
    };
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

// -----------------------------------------------------------------------------

static bool RequiredApiFunctionsImplemented(const RegularSolverApi *api) {
    if (api->GetNumPositions == NULL) return false;
    if (api->GetNumPositions() < 0) return false;
    if (api->GetInitialPosition == NULL) return false;
    if (api->GetInitialPosition() < 0) return false;

    if (api->GenerateMoves == NULL) return false;
    if (api->Primitive == NULL) return false;
    if (api->DoMove == NULL) return false;
    if (api->IsLegalPosition == NULL) return false;

    return true;
}

static bool PositionSymmetryRemovalImplemented(const RegularSolverApi *api) {
    return (api->GetCanonicalPosition != NULL);
}

static bool RetrogradeAnalysisImplemented(const RegularSolverApi *api) {
    return (api->GetCanonicalParentPositions != NULL);
}

static void ConvertApi(const RegularSolverApi *regular, TierSolverApi *tier) {
    tier->GetInitialTier = &GetInitialTier;
    tier->GetInitialPosition = &TierGetInitialPosition;

    tier->GetTierSize = &GetTierSize;
    tier->GenerateMoves = &TierGenerateMoves;
    tier->Primitive = &TierPrimitive;
    tier->DoMove = &TierDoMove;
    tier->IsLegalPosition = &TierIsLegalPosition;

    if (PositionSymmetryRemovalImplemented(regular)) {
        tier->GetCanonicalPosition = &TierGetCanonicalPosition;
        current_options[num_options++] = kPositionSymmetryRemoval;
    } else {
        tier->GetCanonicalPosition = &DefaultGetCanonicalPosition;
    }

    if (RetrogradeAnalysisImplemented(regular)) {
        tier->GetCanonicalParentPositions = &TierGetCanonicalParentPositions;
        current_options[num_options++] = kUseRetrograde;
    } else {
        tier->GetCanonicalParentPositions = NULL;
    }

    if (regular->GetNumberOfCanonicalChildPositions != NULL) {
        tier->GetNumberOfCanonicalChildPositions =
            &TierGetNumberOfCanonicalChildPositions;
    } else {
        tier->GetNumberOfCanonicalChildPositions =
            &DefaultGetNumberOfCanonicalChildPositions;
    }

    if (regular->GetCanonicalChildPositions != NULL) {
        tier->GetCanonicalChildPositions = &TierGetCanonicalChildPositions;
    } else {
        tier->GetCanonicalChildPositions = &DefaultGetCanonicalChildPositions;
    }

    // TODO: add support for loop-free games.
    tier->GetTierType = &DefaultGetTierType;
    tier->GetPositionInSymmetricTier = &DefaultGetPositionInSymmetricTier;
    tier->GetChildTiers = &GetChildTiers;
    tier->GetCanonicalTier = &GetCanonicalTier;
    tier->GetTierName = &DefaultGetTierName;
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

static bool SetCurrentApi(const RegularSolverApi *api) {
    if (!RequiredApiFunctionsImplemented(api)) return false;
    ConvertApi(api, &default_api);
    memcpy(&original_api, api, sizeof(original_api));
    memcpy(&current_api, &default_api, sizeof(current_api));
    return true;
}

// Converted API Functions

static Tier GetInitialTier(void) { return kDefaultTier; }

static Position TierGetInitialPosition(void) {
    return original_api.GetInitialPosition();
}

static int64_t GetTierSize(Tier tier) {
    (void)tier;  // Unused;
    return original_api.GetNumPositions();
}

static int TierGenerateMoves(TierPosition tier_position,
                             Move moves[static kTierSolverNumMovesMax]) {
    return original_api.GenerateMoves(tier_position.position, moves);
}

static Value TierPrimitive(TierPosition tier_position) {
    return original_api.Primitive(tier_position.position);
}

static TierPosition TierDoMove(TierPosition tier_position, Move move) {
    TierPosition ret = {.tier = kDefaultTier};
    ret.position = original_api.DoMove(tier_position.position, move);
    return ret;
}

static bool TierIsLegalPosition(TierPosition tier_position) {
    return original_api.IsLegalPosition(tier_position.position);
}

static Position TierGetCanonicalPosition(TierPosition tier_position) {
    return original_api.GetCanonicalPosition(tier_position.position);
}

static int TierGetNumberOfCanonicalChildPositions(TierPosition tier_position) {
    return original_api.GetNumberOfCanonicalChildPositions(
        tier_position.position);
}

static int TierGetCanonicalChildPositions(
    TierPosition tier_position,
    TierPosition children[static kTierSolverNumChildPositionsMax]) {
    //
    Position raw[kRegularSolverNumMovesMax];
    int num_raw =
        original_api.GetCanonicalChildPositions(tier_position.position, raw);
    for (int i = 0; i < num_raw; ++i) {
        children[i].tier = kDefaultTier;
        children[i].position = raw[i];
    }

    return num_raw;
}

static int TierGetCanonicalParentPositions(
    TierPosition tier_position, Tier parent_tier,
    Position parents[static kTierSolverNumParentPositionsMax]) {
    //
    (void)parent_tier;  // Unused;
    return original_api.GetCanonicalParentPositions(tier_position.position,
                                                    parents);
}

static int GetChildTiers(Tier tier,
                         Tier children[static kTierSolverNumChildTiersMax]) {
    (void)tier;      // Unused;
    (void)children;  // Unmodified.

    return 0;
}

static Tier GetCanonicalTier(Tier tier) { return tier; }

// Default API Functions

static Position DefaultGetCanonicalPosition(TierPosition tier_position) {
    return tier_position.position;
}

static int DefaultGetNumberOfCanonicalChildPositions(
    TierPosition tier_position) {
    //
    TierPositionHashSet dedup;
    TierPositionHashSetInit(&dedup, 0.5);

    Move moves[kRegularSolverNumMovesMax];
    int num_moves = current_api.GenerateMoves(tier_position, moves);
    for (int i = 0; i < num_moves; ++i) {
        TierPosition child = current_api.DoMove(tier_position, moves[i]);
        child.position = current_api.GetCanonicalPosition(child);
        TierPositionHashSetAdd(&dedup, child);
    }
    int num_children = (int)dedup.size;
    TierPositionHashSetDestroy(&dedup);

    return num_children;
}

static int DefaultGetCanonicalChildPositions(
    TierPosition tier_position,
    TierPosition children[static kRegularSolverNumChildPositionsMax]) {
    //
    TierPositionHashSet deduplication_set;
    TierPositionHashSetInit(&deduplication_set, 0.5);
    Move moves[kRegularSolverNumMovesMax];
    int num_moves = current_api.GenerateMoves(tier_position, moves);
    int ret = 0;
    for (int i = 0; i < num_moves; ++i) {
        TierPosition child = current_api.DoMove(tier_position, moves[i]);
        child.position = current_api.GetCanonicalPosition(child);
        if (TierPositionHashSetAdd(&deduplication_set, child)) {
            children[ret++] = child;
        }
    }
    TierPositionHashSetDestroy(&deduplication_set);

    return ret;
}

static TierType DefaultGetTierType(Tier tier) {
    (void)tier;  // Unused.
    return kTierTypeLoopy;
}

static Position DefaultGetPositionInSymmetricTier(TierPosition tier_position,
                                                  Tier symmetric) {
    (void)symmetric;  // Unused.
    return tier_position.position;
}

static int DefaultGetTierName(Tier tier,
                              char name[static kDbFileNameLengthMax + 1]) {
    // Since we only have one tier, we format it's name as
    // "<game_name>_<variant_id>".
    (void)tier;  // Unused.
    sprintf(name, "%s_%d", current_game_name, current_variant_id);

    return kNoError;
}
