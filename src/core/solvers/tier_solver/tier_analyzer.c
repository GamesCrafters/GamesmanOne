#include "core/solvers/tier_solver/tier_analyzer.h"

#include <inttypes.h>  // PRId64
#include <stdbool.h>   // bool, true, false
#include <stddef.h>    // NULL
#include <stdlib.h>    // calloc, free
#include <string.h>    // memset

#include "core/analysis/analysis.h"
#include "core/analysis/stat_manager.h"
#include "core/data_structures/bitstream.h"
#include "core/db/db_manager.h"
#include "core/gamesman_types.h"
#include "core/misc.h"
#include "core/solvers/tier_solver/tier_solver.h"

static const TierSolverApi *current_api;

static Tier this_tier;          // The tier being analyzed.
static int64_t this_tier_size;  // Size of the tier being analyzed.
static TierArray child_tiers;   // Array of canonical child tiers.

// Child tier to its index in the child_tiers array.
static TierHashMap child_tier_to_index;

static BitStream this_tier_map;
static BitStream *child_tier_maps;

static PositionArray fringe;      // Discovered but unprocessed positions.
static PositionArray discovered;  // Newly discovered positions.

static int AnalysisStatus(Tier tier);

static bool Step0Initialize(Analysis *dest);
static TierArray GetCanonicalChildTiers(Tier tier);

static bool Step1LoadDiscoveryMaps(void);
static BitStream LoadDiscoveryMap(Tier tier);

static bool Step2LoadFringe(void);

static bool Step3Discover(Analysis *dest);
static TierPositionArray GetChildPositions(TierPosition tier_position,
                                           Analysis *dest);
static bool DiscoverHelper(Analysis *dest);
static bool DiscoverHelperProcessThisTier(Position child);
static bool DiscoverHelperProcessChildTier(TierPosition child);

static bool Step4SaveChildMaps(void);

static bool Step5Analyze(Analysis *dest);

static bool Step6SaveAnalysis(const Analysis *dest);

static void Step7PrintResults(FILE *stream, const Analysis *dest);

static void Step8CleanUp(void);

// -----------------------------------------------------------------------------

void TierAnalyzerInit(const TierSolverApi *api) { current_api = api; }

int TierAnalyzerDiscover(Analysis *dest, Tier tier, bool force) {
    int ret = -1;
    
    this_tier = tier;
    if (current_api == NULL) goto _bailout;
    if (!force) {
        Tier canonical = current_api->GetCanonicalTier(tier);
        int status = AnalysisStatus(canonical);
        if (status == kAnalysisTierCheckError) {
            ret = kAnalysisTierCheckError;
            goto _bailout;
        } else if (status == kAnalysisTierAnalyzed) {
            StatManagerLoadAnalysis(dest, tier);
            goto _done;
        }
    }

    if (!Step0Initialize(dest)) goto _bailout;
    if (!Step1LoadDiscoveryMaps()) goto _bailout;
    if (!Step2LoadFringe()) goto _bailout;
    if (!Step3Discover(dest)) goto _bailout;
    if (!Step4SaveChildMaps()) goto _bailout;
    if (!Step5Analyze(dest)) goto _bailout;
    if (!Step6SaveAnalysis(dest)) goto _bailout;

_done:
    Step7PrintResults(stdout, dest);
    ret = 0;

_bailout:
    Step8CleanUp();
    return ret;
}

void TierAnalyzerFinalize(void) { current_api = NULL; }

// -----------------------------------------------------------------------------

static int AnalysisStatus(Tier tier) { return StatManagerGetStatus(tier); }

static bool Step0Initialize(Analysis *dest) {
    this_tier_size = current_api->GetTierSize(this_tier);
    child_tiers = GetCanonicalChildTiers(this_tier);
    if (child_tiers.size < 0) return false;

    PositionArrayInit(&fringe);
    PositionArrayInit(&discovered);

    TierHashMapInit(&child_tier_to_index, 0.5);
    for (int64_t i = 0; i < child_tiers.size; ++i) {
        bool success =
            TierHashMapSet(&child_tier_to_index, child_tiers.array[i], i);
        if (!success) return false;
    }

    memset(&this_tier_map, 0, sizeof(this_tier_map));
    child_tier_maps = NULL;

    AnalysisInit(dest);
    AnalysisSetHashSize(dest, this_tier_size);
    return true;
}

static TierArray GetCanonicalChildTiers(Tier tier) {
    static const TierArray kInvalidTierArray;
    TierArray children = current_api->GetChildTiers(tier);
    TierArray canonical_children;
    TierHashSet deduplication;

    if (children.array == NULL) return kInvalidTierArray;
    TierArrayInit(&canonical_children);
    TierHashSetInit(&deduplication, 0.5);

    // Convert child tiers to canonical and deduplicate.
    for (int64_t i = 0; i < children.size; ++i) {
        Tier canonical = current_api->GetCanonicalTier(children.array[i]);
        if (TierHashSetContains(&deduplication, canonical)) continue;
        TierHashSetAdd(&deduplication, canonical);
        TierArrayAppend(&canonical_children, canonical);
    }

    TierArrayDestroy(&children);
    TierHashSetDestroy(&deduplication);
    return canonical_children;
}

static bool Step1LoadDiscoveryMaps(void) {
    child_tier_maps = (BitStream *)calloc(child_tiers.size, sizeof(BitStream));
    if (child_tier_maps == NULL) return false;

    this_tier_map = LoadDiscoveryMap(this_tier);
    for (int64_t i = 0; i < child_tiers.size; ++i) {
        child_tier_maps[i] = LoadDiscoveryMap(child_tiers.array[i]);
    }
    return true;
}

// Loads discovery map of TIER, or returns an empty BitStream if not found
// on disk. IF TIER IS THE INITIAL TIER, ALSO SETS THE INITIAL POSITION BIT.
static BitStream LoadDiscoveryMap(Tier tier) {
    // Try to load from disk first.
    BitStream ret = StatManagerLoadDiscoveryMap(tier);
    if (ret.stream != NULL) return ret;

    // Create a discovery map for the tier.
    int64_t tier_size = current_api->GetTierSize(tier);
    int error = BitStreamInit(&ret, tier_size);
    if (error != 0) {
        fprintf(stderr, "LoadDiscoveryMap: failed to initialize stream\n");
        return ret;
    }

    if (tier == current_api->GetInitialTier()) {
        BitStreamSet(&ret, current_api->GetInitialPosition());
    }

    return ret;
}

static bool Step2LoadFringe(void) {
    bool success = true;
    for (int64_t position = 0; position < this_tier_size; ++position) {
        if (BitStreamGet(&this_tier_map, position)) {
            // critical section here
            if (!PositionArrayAppend(&fringe, position)) success = false;
        }
    }
    return success;
}

static bool Step3Discover(Analysis *dest) {
    bool success = true;

    while (fringe.size > 0) {
        success = DiscoverHelper(dest);
        if (!success) break;

        // Swap fringe with discovered and restart the process.
        PositionArrayDestroy(&fringe);
        fringe = discovered;
        memset(&discovered, 0, sizeof(discovered));
        PositionArrayInit(&discovered);
    }

    PositionArrayDestroy(&fringe);
    PositionArrayDestroy(&discovered);

    return success;
}

static bool DiscoverHelper(Analysis *dest) {
    bool success = true;

    // parallel for
    for (int64_t i = 0; success && i < fringe.size; ++i) {
        TierPosition parent = {.tier = this_tier, .position = fringe.array[i]};
        TierPositionArray children = GetChildPositions(parent, dest);
        if (children.size < 0) {
            success = false;
            continue;
        }

        for (int64_t j = 0; j < children.size; ++j) {
            TierPosition child = children.array[j];
            if (child.tier == this_tier) {
                DiscoverHelperProcessThisTier(child.position);
            } else {
                DiscoverHelperProcessChildTier(child);
            }
        }
        TierPositionArrayDestroy(&children);
    }

    return success;
}

static bool DiscoverHelperProcessThisTier(Position child) {
    // Only add each unique position to the fringe once.
    if (BitStreamGet(&this_tier_map, child)) return true;

    PositionArrayAppend(&discovered, child);
    int error = BitStreamSet(&this_tier_map, child);
    return error == 0;
}

static bool DiscoverHelperProcessChildTier(TierPosition child) {
    // Convert child to the symmetric position in canonical tier.
    Tier canonical_tier = current_api->GetCanonicalTier(child.tier);
    child.position =
        current_api->GetPositionInSymmetricTier(child, canonical_tier);
    child.tier = canonical_tier;

    TierHashMapIterator it = TierHashMapGet(&child_tier_to_index, child.tier);
    if (!TierHashMapIteratorIsValid(&it)) {
        fprintf(stderr,
                "DiscoverHelperProcessChildTier: child position %" PRId64
                " in tier %" PRId64 " not found in the list of child tiers\n",
                child.position, child.tier);
        return false;
    }

    int64_t child_tier_index = TierHashMapIteratorValue(&it);
    BitStream *target_map = &child_tier_maps[child_tier_index];
    int error = BitStreamSet(target_map, child.position);
    return error == 0;
}

static TierPositionArray GetChildPositions(TierPosition tier_position,
                                           Analysis *dest) {
    TierPositionArray ret = {.array = NULL, .capacity = -1, .size = -1};
    MoveArray moves = current_api->GenerateMoves(tier_position);
    if (moves.size < 0) return ret;

    AnalysisDiscoverMoves(dest, tier_position, moves.size);

    TierPositionArrayInit(&ret);
    for (int64_t i = 0; i < moves.size; ++i) {
        TierPosition child = current_api->DoMove(tier_position, moves.array[i]);
        TierPositionArrayAppend(&ret, child);
    }
    MoveArrayDestroy(&moves);
    return ret;
}

static bool Step4SaveChildMaps(void) {
    for (int64_t i = 0; i < child_tiers.size; ++i) {
        int error = StatManagerSaveDiscoveryMap(&child_tier_maps[i],
                                                child_tiers.array[i]);
        if (error != 0) return false;
        BitStreamDestroy(&child_tier_maps[i]);
    }
    free(child_tier_maps);
    child_tier_maps = NULL;

    return true;
}

static bool Step5Analyze(Analysis *dest) {
    DbProbe probe;
    int error = DbManagerProbeInit(&probe);
    if (error != 0) return false;

    bool success = true;

    // parallel, for, move probe inside parallel
    for (int64_t i = 0; success && i < this_tier_size; ++i) {
        TierPosition tier_position = {.tier = this_tier, .position = i};
        TierPosition canonical = {
            .tier = this_tier,
            .position = current_api->GetCanonicalPosition(tier_position),
        };
        if (!BitStreamGet(&this_tier_map, i)) continue;

        Value value = DbManagerProbeValue(&probe, canonical);
        int remoteness = DbManagerProbeRemoteness(&probe, canonical);
        bool is_canonical = (tier_position.position == canonical.position);
        int error =
            AnalysisCount(dest, tier_position, value, remoteness, is_canonical);
        if (error != 0) success = false;
    }
    DbManagerProbeDestroy(&probe);
    BitStreamDestroy(&this_tier_map);
    return success;
}

static bool Step6SaveAnalysis(const Analysis *dest) {
    return StatManagerSaveAnalysis(this_tier, dest) == 0;
}

static void Step7PrintResults(FILE *stream, const Analysis *dest) {
    fprintf(stream, "Tier %" PRId64 " analyzed:\n", this_tier);
    AnalysisPrintEverything(stream, dest);
}

static void Step8CleanUp(void) {
    TierArrayDestroy(&child_tiers);
    TierHashMapDestroy(&child_tier_to_index);
    BitStreamDestroy(&this_tier_map);
    for (int64_t i = 0; i < child_tiers.size; ++i) {
        BitStreamDestroy(&child_tier_maps[i]);
    }
    free(child_tier_maps);
    child_tier_maps = NULL;
    PositionArrayDestroy(&fringe);
    PositionArrayDestroy(&discovered);
}
