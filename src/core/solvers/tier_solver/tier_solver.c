/**
 * @file tier_solver.c
 * @author Max Delgadillo: designed and implemented the original version
 * of tier solver (solveretrograde.c in GamesmanClassic.)
 * @author Robert Shi (robertyishi@berkeley.edu): Modularized, multithreaded
 * tier solver with various optimizations.
 *         GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Implementation of the Tier Solver.
 *
 * @version 1.2.0
 * @date 2024-01-08
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
#include <stdio.h>   // fprintf, stderr
#include <string.h>  // memset, memcpy, strncmp
#ifdef USE_MPI
#endif  // USE_MPI

#include "core/analysis/stat_manager.h"
#include "core/db/bpdb/bpdb_lite.h"
#include "core/db/db_manager.h"
#include "core/db/naivedb/naivedb.h"
#include "core/misc.h"
#include "core/solvers/tier_solver/tier_manager.h"
#include "core/types/gamesman_types.h"

// Solver API functions.

static int TierSolverInit(ReadOnlyString game_name, int variant,
                          const void *solver_api, ReadOnlyString data_path);
static int TierSolverFinalize(void);
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

    .Solve = &TierSolverSolve,
    .Analyze = &TierSolverAnalyze,
    .GetStatus = &TierSolverGetStatus,

    .GetCurrentConfig = &TierSolverGetCurrentConfig,
    .SetOption = &TierSolverSetOption,

    .GetValue = &TierSolverGetValue,
    .GetRemoteness = &TierSolverGetRemoteness,
};

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

// Helper Functions

static bool RequiredApiFunctionsImplemented(const TierSolverApi *api);
static bool TierSymmetryRemovalImplemented(const TierSolverApi *api);
static bool PositionSymmetryRemovalImplemented(const TierSolverApi *api);
static bool RetrogradeAnalysisImplemented(const TierSolverApi *api);

static void ToggleTierSymmetryRemoval(bool on);
static void TogglePositionSymmetryRemoval(bool on);
static void ToggleRetrogradeAnalysis(bool on);

static bool SetCurrentApi(const TierSolverApi *api);

static TierPosition GetCanonicalTierPosition(TierPosition tier_position);

// Default API functions

static Tier DefaultGetCanonicalTier(Tier tier);
static Position DefaultGetPositionInSymmetricTier(TierPosition tier_position,
                                                  Tier symmetric);
static Position DefaultGetCanonicalPosition(TierPosition tier_position);
static int DefaultGetNumberOfCanonicalChildPositions(
    TierPosition tier_position);
static TierPositionArray DefaultGetCanonicalChildPositions(
    TierPosition tier_position);

// -----------------------------------------------------------------------------

static int TierSolverInit(ReadOnlyString game_name, int variant,
                          const void *solver_api, ReadOnlyString data_path) {
    int ret = -1;
    bool success = SetCurrentApi((const TierSolverApi *)solver_api);
    if (!success) goto _bailout;

    ret = DbManagerInitDb(&kBpdbLite, game_name, variant, data_path, NULL);
    if (ret != 0) goto _bailout;

    ret = StatManagerInit(game_name, variant, data_path);
    if (ret != 0) goto _bailout;

    // Success.
    ret = 0;

_bailout:
    if (ret != 0) {
        DbManagerFinalizeDb();
        StatManagerFinalize();
        TierSolverFinalize();
    }
    return ret;
}

static int TierSolverFinalize(void) {
    DbManagerFinalizeDb();
    memset(&default_api, 0, sizeof(default_api));
    memset(&current_api, 0, sizeof(current_api));
    memset(&current_config, 0, sizeof(current_config));
    memset(&current_options, 0, sizeof(current_options));
    memset(&current_selections, 0, sizeof(current_selections));
    num_options = 0;

    return kNoError;
}

static int TierSolverSolve(void *aux) {
    static const TierSolverSolveOptions kDefaultSolveOptions = {
        .force = false,
        .verbose = 1,
    };
    const TierSolverSolveOptions *options = (TierSolverSolveOptions *)aux;
    if (options == NULL) options = &kDefaultSolveOptions;
    return TierManagerSolve(&current_api, options->force, options->verbose);
}

static int TierSolverAnalyze(void *aux) {
    static const TierSolverAnalyzeOptions kDefaultAnalyzeOptions = {
        .force = false,
        .verbose = 1,
    };
    const TierSolverAnalyzeOptions *options = (TierSolverAnalyzeOptions *)aux;
    if (options == NULL) options = &kDefaultAnalyzeOptions;
    return TierManagerAnalyze(&current_api, options->force, options->verbose);
}

static int TierSolverGetStatus(void) {
    // TODO
    return kNotImplementedError;
}

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

    if (current_api.GetCanonicalChildPositions == NULL) {
        current_api.GetCanonicalChildPositions =
            &DefaultGetCanonicalChildPositions;
    }
    return true;
}

static TierPosition GetCanonicalTierPosition(TierPosition tier_position) {
    TierPosition canonical;

    // Convert to the tier position inside the canonical tier.
    canonical.tier = current_api.GetCanonicalTier(tier_position.tier);
    canonical.position =
        current_api.GetPositionInSymmetricTier(tier_position, canonical.tier);

    // Find the canonical position inside the canonical tier.
    canonical.position = current_api.GetCanonicalPosition(canonical);

    return canonical;
}

// Default API functions

static Tier DefaultGetCanonicalTier(Tier tier) { return tier; }

static Position DefaultGetPositionInSymmetricTier(TierPosition tier_position,
                                                  Tier symmetric) {
    (void)symmetric;  // Unused.
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
        child.position = current_api.GetCanonicalPosition(child);
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
        child.position = current_api.GetCanonicalPosition(child);
        if (!TierPositionHashSetContains(&deduplication_set, child)) {
            TierPositionHashSetAdd(&deduplication_set, child);
            TierPositionArrayAppend(&children, child);
        }
    }

    MoveArrayDestroy(&moves);
    TierPositionHashSetDestroy(&deduplication_set);
    return children;
}
