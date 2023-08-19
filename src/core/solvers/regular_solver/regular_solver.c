#include "core/solvers/regular_solver/regular_solver.h"

#include <assert.h>  // assert
#include <stddef.h>  // NULL
#include <stdio.h>   // fprintf, stderr
#include <string.h>  // memset

#include "core/db/naivedb/naivedb.h"
#include "core/gamesman_types.h"
#include "core/misc.h"
#include "core/solvers/tier_solver/tier_solver.h"
#include "core/solvers/tier_solver/tier_worker.h"

// Solver API functions.

static int RegularSolverInit(const void *solver_api);
static int RegularSolverFinalize(void);
static int RegularSolverSolve(void *aux);
static int RegularSolverGetStatus(void);
static const SolverConfiguration *RegularSolverGetCurrentConfiguration(void);
static int RegularSolverSetOption(int option, int selection);

/**
 * @brief The Regular Solver definition. Used externally.
 */
const Solver kRegularSolver = {
    .name = "Tier Solver",
    .db = &kNaiveDb,

    .Init = &RegularSolverInit,
    .Finalize = &RegularSolverFinalize,

    .Solve = &RegularSolverSolve,
    .GetStatus = &RegularSolverGetStatus,

    .GetCurrentConfiguration = &RegularSolverGetCurrentConfiguration,
    .SetOption = &RegularSolverSetOption,
};

static const char *const kChoices[] = {"On", "Off"};
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

// Copy of the original RegularSolverApi object.
static RegularSolverApi original_api;

// Backup of the default API functions. If the user decides to turn off some
// settings and then turn them back on, those functions will be read from this
// object.
static TierSolverApi default_api;

// The API currently being used.
static TierSolverApi current_api;

// Solver settings for external use
static SolverConfiguration current_config;
static int num_options;
#define NUM_OPTIONS_MAX 3  // At most 2 options and 1 zero-terminator.
static SolverOption current_options[NUM_OPTIONS_MAX];
static int current_selections[NUM_OPTIONS_MAX];
#undef NUM_OPTIONS_MAX

// Helper Functions

static bool RequiredApiFunctionsImplemented(const RegularSolverApi *api);
static bool PositionSymmetryRemovalImplemented(const RegularSolverApi *api);
static bool RetrogradeAnalysisImplemented(const RegularSolverApi *api);

static void ConvertApi(const RegularSolverApi *regular, TierSolverApi *tier);

static void TogglePositionSymmetryRemoval(bool on);
static void ToggleRetrogradeAnalysis(bool on);

static bool SetCurrentApi(const RegularSolverApi *api);

// Converted Tier Solver API Variables and Functions

static const Tier kDefaultTier = 0;

static Tier GetInitialTier(void);
static Position TierGetInitialPosition(void);

static int64_t GetTierSize(Tier tier);
static MoveArray TierGenerateMoves(TierPosition tier_position);
static Value TierPrimitive(TierPosition tier_position);
static TierPosition TierDoMove(TierPosition tier_position, Move move);
static bool TierIsLegalPosition(TierPosition tier_position);
static Position TierGetCanonicalPosition(TierPosition tier_position);
static int TierGetNumberOfCanonicalChildPositions(TierPosition tier_position);
static TierPositionArray TierGetCanonicalChildPositions(
    TierPosition tier_position);
static PositionArray TierGetCanonicalParentPositions(TierPosition tier_position,
                                                     Tier parent_tier);
static TierArray GetChildTiers(Tier tier);
static TierArray GetParentTiers(Tier tier);
static Tier GetCanonicalTier(Tier tier);

// Default API Functions.

static Position DefaultGetCanonicalPosition(TierPosition tier_position);
static int DefaultGetNumberOfCanonicalChildPositions(
    TierPosition tier_position);
static TierPositionArray DefaultGetCanonicalChildPositions(
    TierPosition tier_position);

// -----------------------------------------------------------------------------

static int RegularSolverInit(const void *solver_api) {
    return !SetCurrentApi((const RegularSolverApi *)solver_api);
}

static int RegularSolverFinalize(void) {
    memset(&original_api, 0, sizeof(original_api));
    memset(&default_api, 0, sizeof(default_api));
    memset(&current_api, 0, sizeof(current_api));
    memset(&current_config, 0, sizeof(current_config));
    memset(&current_options, 0, sizeof(current_options));
    memset(&current_selections, 0, sizeof(current_selections));
    num_options = 0;
    return 0;
}

static int RegularSolverSolve(void *aux) {
    bool force = false;
    if (aux != NULL) force = *((bool *)aux);

    TierWorkerInit(&current_api);
    return TierWorkerSolve(kDefaultTier, force);
}

static int RegularSolverGetStatus(void) {
    // TODO
    return 0;
}

static const SolverConfiguration *RegularSolverGetCurrentConfiguration(void) {
    return &current_config;
}

static int RegularSolverSetOption(int option, int selection) {
    if (option >= num_options) {
        fprintf(stderr,
                "RegularSolverSetOption: (BUG) option index out of bounds. "
                "Aborting...\n");
        return 1;
    }
    if (selection < 0 || selection > 1) {
        fprintf(stderr,
                "RegularSolverSetOption: (BUG) selection index out of bounds. "
                "Aborting...\n");
        return 1;
    }

    current_selections[option] = selection;
    if (strncmp(current_options[option].name, "Position Symmetry Removal",
                kSolverOptionNameLengthMax) == 0) {
        TogglePositionSymmetryRemoval(!selection);
    } else {
        ToggleRetrogradeAnalysis(!selection);
    }
    return 0;
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

    tier->GetPositionInSymmetricTier = NULL;
    tier->GetChildTiers = &GetChildTiers;
    tier->GetParentTiers = &GetParentTiers;
    tier->GetCanonicalTier = &GetCanonicalTier;
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

static MoveArray TierGenerateMoves(TierPosition tier_position) {
    return original_api.GenerateMoves(tier_position.position);
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

static TierPositionArray TierGetCanonicalChildPositions(
    TierPosition tier_position) {
    //
    PositionArray children =
        original_api.GetCanonicalChildPositions(tier_position.position);
    TierPositionArray ret;
    TierPositionArrayInit(&ret);

    for (int64_t i = 0; i < children.size; ++i) {
        TierPosition this_child = {.tier = kDefaultTier,
                                   .position = children.array[i]};
        TierPositionArrayAppend(&ret, this_child);
    }
    PositionArrayDestroy(&children);
    return ret;
}

static PositionArray TierGetCanonicalParentPositions(TierPosition tier_position,
                                                     Tier parent_tier) {
    (void)parent_tier;  // Unused;
    return original_api.GetCanonicalParentPositions(tier_position.position);
}

static TierArray GetChildTiers(Tier tier) {
    (void)tier;  // Unused;
    TierArray ret;
    Int64ArrayInit(&ret);
    return ret;
}

static TierArray GetParentTiers(Tier tier) {
    (void)tier;  // Unused;
    TierArray ret;
    Int64ArrayInit(&ret);
    return ret;
}

static Tier GetCanonicalTier(Tier tier) {
    return tier;
}

// Default API Functions

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
