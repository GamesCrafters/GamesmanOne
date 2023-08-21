#include "core/solvers/tier_solver/tier_solver.h"

#include <assert.h>  // assert
#include <stddef.h>  // NULL
#include <stdio.h>   // fprintf, stderr
#include <string.h>  // memset, memcpy, strncmp

#include "core/db/naivedb/naivedb.h"
#include "core/gamesman_types.h"
#include "core/misc.h"
#include "core/solvers/tier_solver/tier_manager.h"

// Solver API functions.

static int TierSolverInit(const void *solver_api);
static int TierSolverFinalize(void);
static int TierSolverSolve(void *aux);
static int TierSolverGetStatus(void);
static const SolverConfiguration *TierSolverGetCurrentConfiguration(void);
static int TierSolverSetOption(int option, int selection);

/**
 * @brief The Tier Solver definition. Used externally.
 */
const Solver kTierSolver = {
    .name = "Tier Solver",
    .db = &kNaiveDb,

    .Init = &TierSolverInit,
    .Finalize = &TierSolverFinalize,

    .Solve = &TierSolverSolve,
    .GetStatus = &TierSolverGetStatus,

    .GetCurrentConfiguration = &TierSolverGetCurrentConfiguration,
    .SetOption = &TierSolverSetOption,
};

static const char *const kChoices[] = {"On", "Off"};
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
static SolverConfiguration current_config;
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

// Default API functions

static Tier DefaultGetCanonicalTier(Tier tier);
static Position DefaultGetCanonicalPosition(TierPosition tier_position);
static int DefaultGetNumberOfCanonicalChildPositions(
    TierPosition tier_position);
static TierPositionArray DefaultGetCanonicalChildPositions(
    TierPosition tier_position);

// -----------------------------------------------------------------------------

static int TierSolverInit(const void *solver_api) {
    return !SetCurrentApi((const TierSolverApi *)solver_api);
}

static int TierSolverFinalize(void) {
    memset(&default_api, 0, sizeof(default_api));
    memset(&current_api, 0, sizeof(current_api));
    memset(&current_config, 0, sizeof(current_config));
    memset(&current_options, 0, sizeof(current_options));
    memset(&current_selections, 0, sizeof(current_selections));
    num_options = 0;
    return 0;
}

static int TierSolverSolve(void *aux) {
    bool force = false;
    if (aux != NULL) force = *((bool *)aux);
    return TierManagerSolve(&current_api, force);
}

static int TierSolverGetStatus(void) {
    // TODO
    return 0;
}

static const SolverConfiguration *TierSolverGetCurrentConfiguration(void) {
    return &current_config;
}

static int TierSolverSetOption(int option, int selection) {
    if (option >= num_options) {
        fprintf(stderr,
                "TierSolverSetOption: (BUG) option index out of bounds. "
                "Aborting...\n");
        return 1;
    }
    if (selection < 0 || selection > 1) {
        fprintf(stderr,
                "TierSolverSetOption: (BUG) selection index out of bounds. "
                "Aborting...\n");
        return 1;
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
    return 0;
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
    if (api->GetParentTiers == NULL) return false;

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
        current_api.GetPositionInSymmetricTier = NULL;
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

// Default API functions

static Tier DefaultGetCanonicalTier(Tier tier) { return tier; }

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