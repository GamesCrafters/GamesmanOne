/**
 * @file tier_analyzer.c
 * @author Robert Shi (robertyishi@berkeley.edu)
 *         GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Implementation of the analyzer module for the Loopy Tier Solver.
 * @version 1.2.0
 * @date 2024-07-11
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

#include "core/solvers/tier_solver/tier_analyzer.h"

#include <stdbool.h>  // bool, true, false
#include <stddef.h>   // NULL
#include <stdlib.h>   // malloc, calloc, free
#include <string.h>   // memset

#include "core/analysis/analysis.h"
#include "core/analysis/stat_manager.h"
#include "core/data_structures/bitstream.h"
#include "core/db/db_manager.h"
#include "core/misc.h"
#include "core/solvers/tier_solver/tier_solver.h"
#include "core/types/gamesman_types.h"

// Include and use OpenMP if the _OPENMP flag is set.
#ifdef _OPENMP
#include <omp.h>
#define PRAGMA(X) _Pragma(#X)
#define PRAGMA_OMP_PARALLEL PRAGMA(omp parallel)
#define PRAGMA_OMP_FOR_SCHEDULE_DYNAMIC(k) PRAGMA(omp for schedule(dynamic, k))
#define PRAGMA_OMP_PARALLEL_FOR_SCHEDULE_DYNAMIC(k) PRAGMA(omp parallel for schedule(dynamic, k))
#define PRAGMA_OMP_CRITICAL(name) PRAGMA(omp critical(name))

// Otherwise, the following macros do nothing.
#else
#define PRAGMA
#define PRAGMA_OMP_PARALLEL
#define PRAGMA_OMP_FOR_SCHEDULE_DYNAMIC(k)
#define PRAGMA_OMP_PARALLEL_FOR_SCHEDULE_DYNAMIC(k)
#define PRAGMA_OMP_CRITICAL(name)
#endif  // _OPENMP

static const TierSolverApi *api_internal;

static Tier this_tier;          // The tier being analyzed.
static int64_t this_tier_size;  // Size of the tier being analyzed.
static TierArray child_tiers;   // Array of canonical child tiers.

// Child tier to its index in the child_tiers array.
static TierHashMap child_tier_to_index;

static BitStream this_tier_map;
static BitStream *child_tier_maps;
#ifdef _OPENMP
static omp_lock_t *child_tier_map_locks;
#endif  // _OPENMP

static int num_threads;            // Number of threads available.
static PositionArray *fringe;      // Discovered but unprocessed positions.
static PositionArray *discovered;  // Newly discovered positions.

static int AnalysisStatus(Tier tier);

static bool Step0Initialize(Analysis *dest);
static TierArray GetCanonicalChildTiers(Tier tier);

static bool Step1LoadDiscoveryMaps(void);
static BitStream LoadDiscoveryMap(Tier tier);

static bool Step2LoadFringe(void);

static bool Step3Discover(Analysis *dest);
static TierPositionArray GetChildPositions(TierPosition tier_position,
                                           Analysis *dest);
static bool IsCanonicalPosition(TierPosition tier_position);
static bool DiscoverHelper(Analysis *dest);
static bool DiscoverHelperProcessThisTier(Position child, int tid);
static bool DiscoverHelperProcessChildTier(TierPosition child);

static bool Step4SaveChildMaps(void);

static bool Step5Analyze(Analysis *dest);

static bool Step6SaveAnalysis(const Analysis *dest);

static void Step7CleanUp(void);

static int GetThreadId(void);
static int64_t GetFringeSize(void);
static int64_t *MakeFringeOffsets(void);
static void DestroyFringe(PositionArray *target);
static void InitFringe(PositionArray *target);

// -----------------------------------------------------------------------------

void TierAnalyzerInit(const TierSolverApi *api) { api_internal = api; }

int TierAnalyzerAnalyze(Analysis *dest, Tier tier, bool force) {
    int ret = -1;

    this_tier = tier;
    if (api_internal == NULL) goto _bailout;
    if (!force) {
        int status = AnalysisStatus(tier);
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
    ret = 0;

_bailout:
    Step7CleanUp();
    return ret;
}

void TierAnalyzerFinalize(void) { api_internal = NULL; }

// -----------------------------------------------------------------------------

static int AnalysisStatus(Tier tier) { return StatManagerGetStatus(tier); }

static bool Step0Initialize(Analysis *dest) {
#ifdef _OPENMP
    num_threads = omp_get_max_threads();
#else   // _OPENMP not defined.
    num_threads = 1;
#endif  // _OPENMP
    this_tier_size = api_internal->GetTierSize(this_tier);
    child_tiers = GetCanonicalChildTiers(this_tier);
    if (child_tiers.size < 0) return false;

    fringe = (PositionArray *)malloc(num_threads * sizeof(PositionArray));
    discovered = (PositionArray *)malloc(num_threads * sizeof(PositionArray));
    if (fringe == NULL || discovered == NULL) return false;

    InitFringe(fringe);
    InitFringe(discovered);

    TierHashMapInit(&child_tier_to_index, 0.5);
    for (int64_t i = 0; i < child_tiers.size; ++i) {
        bool success =
            TierHashMapSet(&child_tier_to_index, child_tiers.array[i], i);
        if (!success) return false;
    }

    memset(&this_tier_map, 0, sizeof(this_tier_map));
    child_tier_maps = NULL;
#ifdef _OPENMP
    child_tier_map_locks = NULL;
#endif  // _OPENMP

    AnalysisInit(dest);
    AnalysisSetHashSize(dest, this_tier_size);
    return true;
}

static TierArray GetCanonicalChildTiers(Tier tier) {
    static const TierArray kInvalidTierArray;
    TierArray children = api_internal->GetChildTiers(tier);
    TierArray canonical_children;
    TierHashSet deduplication;

    if (children.array == NULL) return kInvalidTierArray;
    TierArrayInit(&canonical_children);
    TierHashSetInit(&deduplication, 0.5);

    // Convert child tiers to canonical and deduplicate.
    for (int64_t i = 0; i < children.size; ++i) {
        Tier canonical = api_internal->GetCanonicalTier(children.array[i]);
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

#ifdef _OPENMP
    child_tier_map_locks =
        (omp_lock_t *)malloc(child_tiers.size * sizeof(omp_lock_t));
    if (child_tier_map_locks == NULL) return false;
    for (int64_t i = 0; i < child_tiers.size; ++i) {
        omp_init_lock(&child_tier_map_locks[i]);
    }
#endif  // _OPENMP

    this_tier_map = LoadDiscoveryMap(this_tier);
    if (this_tier_map.size == 0) return false;

    for (int64_t i = 0; i < child_tiers.size; ++i) {
        child_tier_maps[i] = LoadDiscoveryMap(child_tiers.array[i]);
        if (child_tier_maps[i].size == 0) return false;
    }

    return true;
}

// Loads discovery map of TIER, or returns an empty BitStream if not found
// on disk. IF TIER IS THE INITIAL TIER, ALSO SETS THE INITIAL POSITION BIT.
static BitStream LoadDiscoveryMap(Tier tier) {
    // Try to load from disk first.
    int64_t tier_size = api_internal->GetTierSize(tier);
    BitStream ret = {0};
    int error = StatManagerLoadDiscoveryMap(tier, tier_size, &ret);
    if (error != kFileSystemError) return ret;

    // Create a discovery map for the tier.
    error = BitStreamInit(&ret, tier_size);
    if (error != 0) {
        fprintf(stderr, "LoadDiscoveryMap: failed to initialize stream\n");
        return ret;
    }

    if (tier == api_internal->GetInitialTier()) {
        BitStreamSet(&ret, api_internal->GetInitialPosition());
    }

    return ret;
}

static bool Step2LoadFringe(void) {
    bool success = true;
    PRAGMA_OMP_PARALLEL {
        int tid = GetThreadId();
        PRAGMA_OMP_FOR_SCHEDULE_DYNAMIC(1024)
        for (Position position = 0; position < this_tier_size; ++position) {
            if (!success) continue;
            if (BitStreamGet(&this_tier_map, position)) {
                if (!PositionArrayAppend(&fringe[tid], position)) {
                    success = false;
                }
            }
        }
    }

    return success;
}

static bool Step3Discover(Analysis *dest) {
    bool success = true;
    while (GetFringeSize() > 0) {
        success = DiscoverHelper(dest);
        if (!success) break;

        // Swap fringe with discovered and restart.
        DestroyFringe(fringe);
        InitFringe(fringe);
        PositionArray *tmp = fringe;
        fringe = discovered;
        discovered = tmp;
    }

    DestroyFringe(fringe);
    DestroyFringe(discovered);

    return success;
}

static bool IsPrimitive(TierPosition tier_position) {
    return api_internal->Primitive(tier_position) != kUndecided;
}

static void UpdateFringeId(int *fringe_id, int64_t i,
                           const int64_t *fringe_offsets) {
    while (i >= fringe_offsets[*fringe_id + 1]) {
        ++(*fringe_id);
    }
}

static bool DiscoverHelper(Analysis *dest) {
    bool success = true;
    int64_t *fringe_offsets = MakeFringeOffsets();
    if (fringe_offsets == NULL) return false;

    PRAGMA_OMP_PARALLEL {
        int tid = GetThreadId();
        int fringe_id = 0;

        PRAGMA_OMP_FOR_SCHEDULE_DYNAMIC(1024)
        for (int64_t i = 0; i < fringe_offsets[num_threads]; ++i) {
            if (!success) continue;  // Fail fast.
            UpdateFringeId(&fringe_id, i, fringe_offsets);
            int64_t index_in_fringe = i - fringe_offsets[fringe_id];
            TierPosition parent;
            parent.tier = this_tier;
            parent.position = fringe[fringe_id].array[index_in_fringe];
            // Do not generate children of primitive positions.
            if (IsPrimitive(parent)) continue;

            TierPositionArray children = GetChildPositions(parent, dest);
            if (children.size < 0) {
                success = false;
                continue;
            }
            for (int64_t j = 0; j < children.size; ++j) {
                TierPosition child = children.array[j];
                if (child.tier == this_tier) {
                    DiscoverHelperProcessThisTier(child.position, tid);
                } else {
                    DiscoverHelperProcessChildTier(child);
                }
            }
            TierPositionArrayDestroy(&children);
        }
    }
    free(fringe_offsets);
    fringe_offsets = NULL;

    return success;
}

static bool DiscoverHelperProcessThisTier(Position child, int tid) {
    int error = 0;
    bool child_is_discovered;
    PRAGMA_OMP_CRITICAL(tier_analyzer_this_tier_map) {
        child_is_discovered = BitStreamGet(&this_tier_map, child);
        if (!child_is_discovered) {
            error = BitStreamSet(&this_tier_map, child);
        }
    }
    if (error != 0) return false;

    // Only add each unique position to the fringe once.
    if (!child_is_discovered) {
        error = !PositionArrayAppend(&discovered[tid], child);
    }

    return error == 0;
}

static bool DiscoverHelperProcessChildTier(TierPosition child) {
    // Convert child to the symmetric position in canonical tier.
    Tier canonical_tier = api_internal->GetCanonicalTier(child.tier);
    child.position =
        api_internal->GetPositionInSymmetricTier(child, canonical_tier);
    child.tier = canonical_tier;

    TierHashMapIterator it = TierHashMapGet(&child_tier_to_index, child.tier);
    if (!TierHashMapIteratorIsValid(&it)) {
        fprintf(stderr,
                "DiscoverHelperProcessChildTier: child position %" PRIPos
                " in tier %" PRITier " not found in the list of child tiers\n",
                child.position, child.tier);
        return false;
    }

    int64_t child_tier_index = TierHashMapIteratorValue(&it);
    BitStream *target_map = &child_tier_maps[child_tier_index];
#ifdef _OPENMP
    omp_set_lock(&child_tier_map_locks[child_tier_index]);
#endif  // _OPENMP
    int error = BitStreamSet(target_map, child.position);
#ifdef _OPENMP
    omp_unset_lock(&child_tier_map_locks[child_tier_index]);
#endif  // _OPENMP

    return error == 0;
}

static TierPositionArray GetChildPositions(TierPosition tier_position,
                                           Analysis *dest) {
    TierPositionArray ret = {.array = NULL, .capacity = -1, .size = -1};
    MoveArray moves = api_internal->GenerateMoves(tier_position);
    if (moves.size < 0) return ret;

    TierPositionArrayInit(&ret);
    TierPositionHashSet deduplication_set;
    TierPositionHashSetInit(&deduplication_set, 0.5);
    for (int64_t i = 0; i < moves.size; ++i) {
        TierPosition child =
            api_internal->DoMove(tier_position, moves.array[i]);
        TierPositionArrayAppend(&ret, child);
        child.position = api_internal->GetCanonicalPosition(child);
        if (!TierPositionHashSetContains(&deduplication_set, child)) {
            bool success = TierPositionHashSetAdd(&deduplication_set, child);
            if (!success) {
                TierPositionHashSetDestroy(&deduplication_set);
                TierPositionArrayDestroy(&ret);
                ret.capacity = ret.size = -1;
                return ret;
            }
        }
    }
    // Using multiplication to avoid branching.
    int num_canonical_moves =
        deduplication_set.size * IsCanonicalPosition(tier_position);
    TierPositionHashSetDestroy(&deduplication_set);

    PRAGMA_OMP_CRITICAL(tier_analyzer_get_child_positions_dest) {
        AnalysisDiscoverMoves(dest, tier_position, moves.size,
                              num_canonical_moves);
    }
    MoveArrayDestroy(&moves);

    return ret;
}

static bool IsCanonicalPosition(TierPosition tier_position) {
    return api_internal->GetCanonicalPosition(tier_position) ==
           tier_position.position;
}

static bool Step4SaveChildMaps(void) {
    for (int64_t i = 0; i < child_tiers.size; ++i) {
        int error = StatManagerSaveDiscoveryMap(&child_tier_maps[i],
                                                child_tiers.array[i]);
        if (error != 0) return false;
        BitStreamDestroy(&child_tier_maps[i]);
#ifdef _OPENMP
        omp_destroy_lock(&child_tier_map_locks[i]);
#endif  // _OPENMP
    }

    free(child_tier_maps);
    child_tier_maps = NULL;
#ifdef _OPENMP
    free(child_tier_map_locks);
    child_tier_map_locks = NULL;
#endif  // _OPENMP

    return true;
}

static bool Step5Analyze(Analysis *dest) {
    bool success = true;
    int error = DbManagerLoadTier(this_tier, this_tier_size);
    if (error != kNoError) return false;

    Analysis *parts = (Analysis *)malloc(num_threads * sizeof(Analysis));
    if (parts == NULL) return false;
    for (int i = 0; i < num_threads; ++i) {
        AnalysisInit(&parts[i]);
    }

    PRAGMA_OMP_PARALLEL {
        int tid = GetThreadId();
        PRAGMA_OMP_FOR_SCHEDULE_DYNAMIC(1024)
        for (int64_t i = 0; i < this_tier_size; ++i) {
            // Skip if the current position is not reachable.
            if (!BitStreamGet(&this_tier_map, i)) continue;

            TierPosition tier_position = {.tier = this_tier, .position = i};
            Position canonical =
                api_internal->GetCanonicalPosition(tier_position);

            // Must probe canonical positions. Original might not be solved.
            Value value = DbManagerGetValueFromLoaded(this_tier, canonical);
            int remoteness =
                DbManagerGetRemotenessFromLoaded(this_tier, canonical);
            bool is_canonical = (tier_position.position == canonical);
            error = AnalysisCount(&parts[tid], tier_position, value, remoteness,
                                  is_canonical);
            if (error != 0) success = false;
        }
    }
    for (int i = 0; i < num_threads; ++i) {
        AnalysisMergeCounts(dest, &parts[i]);
    }
    free(parts);

    error = DbManagerUnloadTier(this_tier);
    if (error != kNoError) success = false;
    BitStreamDestroy(&this_tier_map);

    return success;
}

static bool Step6SaveAnalysis(const Analysis *dest) {
    bool success = (StatManagerSaveAnalysis(this_tier, dest) == 0);
    if (!success) return false;

    if (this_tier != api_internal->GetInitialTier()) {
        StatManagerRemoveDiscoveryMap(this_tier);
    }
    return true;
}

static void Step7CleanUp(void) {
    TierArrayDestroy(&child_tiers);
    TierHashMapDestroy(&child_tier_to_index);
    BitStreamDestroy(&this_tier_map);
    for (int64_t i = 0; i < child_tiers.size; ++i) {
        BitStreamDestroy(&child_tier_maps[i]);
#ifdef _OPENMP
        omp_destroy_lock(&child_tier_map_locks[i]);
#endif  // _OPENMP
    }
    free(child_tier_maps);
    child_tier_maps = NULL;
#ifdef _OPENMP
    free(child_tier_map_locks);
    child_tier_map_locks = NULL;
#endif  // _OPENMP

    // Clean up fringe and discovered queues.
    DestroyFringe(fringe);
    DestroyFringe(discovered);
    free(fringe);
    free(discovered);
}

static int GetThreadId(void) {
#ifdef _OPENMP
    return omp_get_thread_num();
#else   // _OPENMP not defined, thread 0 is the only available thread.
    return 0;
#endif  // _OPENMP
}

static int64_t GetFringeSize(void) {
    int64_t size = 0;
    for (int i = 0; i < num_threads; ++i) {
        size += fringe[i].size;
    }

    return size;
}

static int64_t *MakeFringeOffsets(void) {
    int64_t *frontier_offsets =
        (int64_t *)malloc((num_threads + 1) * sizeof(int64_t));
    if (frontier_offsets == NULL) return NULL;

    frontier_offsets[0] = 0;
    for (int i = 1; i <= num_threads; ++i) {
        frontier_offsets[i] = frontier_offsets[i - 1] + fringe[i - 1].size;
    }

    return frontier_offsets;
}

static void DestroyFringe(PositionArray *target) {
    if (target == NULL) return;
    for (int i = 0; i < num_threads; ++i) {
        PositionArrayDestroy(&target[i]);
    }
}

static void InitFringe(PositionArray *target) {
    for (int i = 0; i < num_threads; ++i) {
        PositionArrayInit(&target[i]);
    }
}

#undef PRAGMA
#undef PRAGMA_OMP_PARALLEL
#undef PRAGMA_OMP_FOR_SCHEDULE_DYNAMIC
#undef PRAGMA_OMP_PARALLEL_FOR_SCHEDULE_DYNAMIC
#undef PRAGMA_OMP_CRITICAL
