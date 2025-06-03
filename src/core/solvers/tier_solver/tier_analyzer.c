/**
 * @file tier_analyzer.c
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Implementation of the analyzer module for the Loopy Tier Solver.
 * @version 2.0.2
 * @date 2025-06-03
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
#include <stdint.h>   // int64_t, intptr_t
#include <string.h>   // memset

#include "core/analysis/analysis.h"
#include "core/analysis/stat_manager.h"
#include "core/concurrency.h"
#include "core/data_structures/concurrent_bitset.h"
#include "core/db/db_manager.h"
#include "core/gamesman_memory.h"
#include "core/misc.h"
#include "core/solvers/tier_solver/tier_solver.h"
#include "core/types/gamesman_types.h"

// Include and use OpenMP if the _OPENMP flag is set.
#ifdef _OPENMP
#include <omp.h>
#endif  // _OPENMP

// ============================= Global Variables =============================

// An internal reference to the API.
static const TierSolverApi *api_internal;

// Whether to explore the canonical graph only
static bool explore_canonical;

// Size-tracking memory allocator
static GamesmanAllocator *allocator;

static Tier this_tier;          // The tier being analyzed.
static int64_t this_tier_size;  // Size of the tier being analyzed.

// Array of canonical child tiers.
static Tier child_tiers[kTierSolverNumChildTiersMax];
static int num_child_tiers;  // Size of child_tiers.

// Child tier to its index in the child_tiers array.
static TierHashMap child_tier_to_index;

static ConcurrentBitset *this_tier_map;
static ConcurrentBitset **child_tier_maps;

static int num_threads;  // Number of threads available.

typedef struct {
    PositionArray a;
    char padding[GM_CACHE_LINE_PAD(sizeof(PositionArray))];
} PaddedPositionArray;
static PaddedPositionArray *fringe;  // Discovered but unprocessed positions.
static PaddedPositionArray *discovered;  // Newly discovered positions.
static ConcurrentBitset *bs_fringe, *bs_discovered;

// ============================= TierAnalyzerInit =============================

bool TierAnalyzerInit(const TierSolverApi *api, intptr_t memlimit) {
    api_internal = api;
    explore_canonical = (api->GetNumberOfSymmetries != NULL);
    GamesmanAllocatorOptions options;
    GamesmanAllocatorOptionsSetDefaults(&options);
    options.pool_size =
        memlimit ? memlimit : (intptr_t)GetPhysicalMemory() / 10 * 9;
    allocator = GamesmanAllocatorCreate(&options);

    return allocator != NULL;
}

// ============================ TierAnalyzerAnalyze ============================

// Step0Initialize

static int GetCanonicalChildTiers(
    Tier tier, Tier canonical_children[kTierSolverNumChildTiersMax]) {
    //
    Tier children[kTierSolverNumChildTiersMax];
    int num_children = api_internal->GetChildTiers(tier, children);

    // Convert child tiers to canonical and deduplicate.
    TierHashSet dedup;
    TierHashSetInit(&dedup, 0.5);
    int ret = 0;
    for (int i = 0; i < num_children; ++i) {
        Tier canonical = api_internal->GetCanonicalTier(children[i]);
        if (!TierHashSetContains(&dedup, canonical)) {
            TierHashSetAdd(&dedup, canonical);
            canonical_children[ret++] = canonical;
        }
    }
    TierHashSetDestroy(&dedup);

    return ret;
}

static bool Step0_0CheckMem(void) {
    size_t fringe_container_size =
        2 * num_threads * sizeof(PaddedPositionArray);
    size_t bitset_fringe_size = 2 * ConcurrentBitsetMemRequired(this_tier_size);
    size_t this_tier_map_size = 2 * ConcurrentBitsetMemRequired(this_tier_size);
    size_t child_tier_maps_size = num_child_tiers * sizeof(ConcurrentBitset *);
    size_t serialize_overhead = 0;
    for (int i = 0; i < num_child_tiers; ++i) {
        int64_t tier_size = api_internal->GetTierSize(child_tiers[i]);
        child_tier_maps_size += ConcurrentBitsetMemRequired(tier_size);
        if (child_tier_maps_size > serialize_overhead) {
            serialize_overhead = child_tier_maps_size;
        }
    }
    size_t partial_analysis_size = num_threads * sizeof(CacheAlignedAnalysis);
    size_t fringe_offsets_size = (num_threads + 1) * sizeof(int64_t);
    size_t total = fringe_container_size + bitset_fringe_size +
                   child_tier_maps_size + serialize_overhead +
                   this_tier_map_size + partial_analysis_size +
                   fringe_offsets_size;

    return total <= GamesmanAllocatorGetRemainingPoolSize(allocator);
}

static void InitFringeArray(PaddedPositionArray *target) {
    for (int i = 0; i < num_threads; ++i) {
        PositionArrayInitAllocator(&target[i].a, allocator);
    }
}

static void Step0_1InitFringes(void) {
    // Array fringes.
    fringe = (PaddedPositionArray *)GamesmanAllocatorAllocate(
        allocator, num_threads * sizeof(PaddedPositionArray));
    discovered = (PaddedPositionArray *)GamesmanAllocatorAllocate(
        allocator, num_threads * sizeof(PaddedPositionArray));
    memset(fringe, 0, num_threads * sizeof(PaddedPositionArray));
    memset(discovered, 0, num_threads * sizeof(PaddedPositionArray));
    InitFringeArray(fringe);
    InitFringeArray(discovered);

    // Initialize discovered bitset. The fringe bitset will be initialized
    // to a copy of the current tier's position bit map once it is loaded
    // from disk or created.
    bs_discovered = ConcurrentBitsetCreateAllocator(this_tier_size, allocator);
}

static void Step0_2InitChildTiersReverseLookupMap(void) {
    TierHashMapInit(&child_tier_to_index, 0.5);
    for (int i = 0; i < num_child_tiers; ++i) {
        TierHashMapSet(&child_tier_to_index, child_tiers[i], i);
    }
}

static void Step0_3InitAnalysis(Analysis *dest) {
    AnalysisInit(dest);
    AnalysisSetHashSize(dest, this_tier_size);
}

static bool Step0Initialize(Analysis *dest) {
    num_threads = ConcurrencyGetOmpNumThreads();
    this_tier_size = api_internal->GetTierSize(this_tier);
    num_child_tiers = GetCanonicalChildTiers(this_tier, child_tiers);
    if (!Step0_0CheckMem()) return false;
    Step0_1InitFringes();
    Step0_2InitChildTiersReverseLookupMap();
    Step0_3InitAnalysis(dest);

    return true;
}

// Step1LoadDiscoveryMaps

// Loads discovery map of TIER from disk or creates it if not found on disk.
// Returns NULL if creation fails. If TIER is the initial tier, also sets the
// initial position bit.
static ConcurrentBitset *LoadDiscoveryMap(Tier tier) {
    // Try to load from disk first.
    int64_t tier_size = api_internal->GetTierSize(tier);
    ConcurrentBitset *ret = NULL;
    int error = StatManagerLoadDiscoveryMap(tier, tier_size, allocator, &ret);
    if (error != kFileSystemError) return ret;

    // Create a discovery map for the tier.
    ret = ConcurrentBitsetCreate(tier_size);
    if (ret == NULL) {
        fprintf(
            stderr,
            "LoadDiscoveryMap: failed to initialize bitset for tier %" PRITier
            "\n",
            tier);
        return ret;
    }

    if (tier == api_internal->GetInitialTier()) {
        ConcurrentBitsetSet(ret, api_internal->GetInitialPosition(),
                            memory_order_relaxed);
    }

    return ret;
}

static bool Step1LoadDiscoveryMaps(void) {
    this_tier_map = LoadDiscoveryMap(this_tier);
    if (this_tier_map == NULL) return false;
    bs_fringe = ConcurrentBitsetCreateCopy(this_tier_map);

    if (num_child_tiers > 0) {
        child_tier_maps = (ConcurrentBitset **)GamesmanAllocatorAllocate(
            allocator, num_child_tiers * sizeof(ConcurrentBitset *));
        if (child_tier_maps == NULL) return false;
        memset(child_tier_maps, 0,
               num_child_tiers * sizeof(ConcurrentBitset *));

        for (int i = 0; i < num_child_tiers; ++i) {
            child_tier_maps[i] = LoadDiscoveryMap(child_tiers[i]);
            if (child_tier_maps[i] == NULL) return false;
        }
    }

    return true;
}

// Step2Discover

static int64_t GetFringeSize(void) {
    int64_t size = 0;
    for (int i = 0; i < num_threads; ++i) {
        size += fringe[i].a.size;
    }

    return size;
}

static int64_t *MakeFringeOffsets(PaddedPositionArray *src) {
    int64_t *fringe_offsets = (int64_t *)GamesmanAllocatorAllocate(
        allocator, (num_threads + 1) * sizeof(int64_t));
    if (fringe_offsets == NULL) return NULL;

    fringe_offsets[0] = 0;
    for (int i = 1; i <= num_threads; ++i) {
        fringe_offsets[i] = fringe_offsets[i - 1] + src[i - 1].a.size;
    }

    return fringe_offsets;
}

static void UpdateFringeId(int *fringe_id, int64_t i,
                           const int64_t *fringe_offsets) {
    while (i >= fringe_offsets[*fringe_id + 1]) {
        ++(*fringe_id);
    }
}

static CacheAlignedAnalysis *MakePartialAnalyses(void) {
    CacheAlignedAnalysis *parts =
        (CacheAlignedAnalysis *)GamesmanAllocatorAllocate(
            allocator, num_threads * sizeof(CacheAlignedAnalysis));
    if (parts == NULL) return NULL;

    for (int i = 0; i < num_threads; ++i) {
        AnalysisInit(&parts[i].data);
    }

    return parts;
}

static void MergePartialAnalysisMoves(Analysis *dest,
                                      const CacheAlignedAnalysis *parts) {
    for (int i = 0; i < num_threads; ++i) {
        AnalysisMergeMoves(dest, &parts[i]);
    }
}

static void MergePartialAnalysisCounts(Analysis *dest,
                                       const CacheAlignedAnalysis *parts) {
    for (int i = 0; i < num_threads; ++i) {
        AnalysisMergeCounts(dest, &parts[i]);
    }
}

static bool IsPrimitive(TierPosition tier_position) {
    return api_internal->Primitive(tier_position) != kUndecided;
}

static bool IsCanonicalPosition(TierPosition tier_position) {
    return api_internal->GetCanonicalPosition(tier_position) ==
           tier_position.position;
}

static int GetChildPositions(
    TierPosition parent,
    TierPosition children[static kTierSolverNumChildPositionsMax],
    Analysis *dest) {
    //
    int num_children = 0;
    Move moves[kTierSolverNumMovesMax];
    int num_moves = api_internal->GenerateMoves(parent, moves);

    TierPositionHashSet dedup;
    TierPositionHashSetInit(&dedup, 0.5);
    TierPositionHashSetReserve(&dedup, num_moves / 2);
    for (int i = 0; i < num_moves; ++i) {
        TierPosition child = api_internal->DoMove(parent, moves[i]);
        children[num_children++] = child;
        child.position = api_internal->GetCanonicalPosition(child);
        TierPositionHashSetAdd(&dedup, child);
    }

    // If the parent position is not a canonical position, then the number of
    // canonical moves is 0. Using multiplication to avoid branching.
    int num_canonical_moves = (int)dedup.size * IsCanonicalPosition(parent);
    TierPositionHashSetDestroy(&dedup);
    AnalysisDiscoverMoves(dest, parent, num_moves, num_canonical_moves);

    return num_children;
}

static int GetCanonicalChildPositions(
    TierPosition parent,
    TierPosition children[static kTierSolverNumChildPositionsMax],
    Analysis *dest) {
    // Generate moves just to find out how many moves there are
    Move moves[kTierSolverNumMovesMax];
    int num_moves = api_internal->GenerateMoves(parent, moves);

    // Generate canonical child positions
    int num_canonical_children =
        api_internal->GetCanonicalChildPositions(parent, children);
    int num_symmetries = api_internal->GetNumberOfSymmetries(parent);
    AnalysisDiscoverMovesGroup(dest, parent, num_symmetries, num_moves,
                               num_canonical_children);

    return num_canonical_children;
}

static bool DiscoverProcessThisTierArray(Position child, int tid) {
    bool child_is_discovered =
        ConcurrentBitsetSet(this_tier_map, child, memory_order_relaxed);

    // Only add each unique position to the fringe once.
    if (child_is_discovered) return true;

    return PositionArrayAppend(&discovered[tid].a, child);
}

static void DiscoverProcessThisTierBitset(Position child) {
    bool child_is_discovered =
        ConcurrentBitsetSet(this_tier_map, child, memory_order_relaxed);

    if (!child_is_discovered) {
        ConcurrentBitsetSet(bs_discovered, child, memory_order_relaxed);
    }
}

static void DiscoverProcessChildTier(TierPosition child) {
    // Convert child to the symmetric position in canonical tier.
    Tier canonical_tier = api_internal->GetCanonicalTier(child.tier);
    child.position =
        api_internal->GetPositionInSymmetricTier(child, canonical_tier);
    child.tier = canonical_tier;

    TierHashMapIterator it = TierHashMapGet(&child_tier_to_index, child.tier);
    if (!TierHashMapIteratorIsValid(&it)) {
        fprintf(stderr,
                "DiscoverProcessChildTier: child position %" PRIPos
                " in tier %" PRITier " not found in the list of child tiers\n",
                child.position, child.tier);
        NotReached("Terminating...");
    }

    int64_t child_tier_index = TierHashMapIteratorValue(&it);
    ConcurrentBitset *target_map = child_tier_maps[child_tier_index];
    ConcurrentBitsetSet(target_map, child.position, memory_order_relaxed);
}

static void DestroyFringeArray(PaddedPositionArray *target) {
    if (target == NULL) return;
    for (int i = 0; i < num_threads; ++i) {
        PositionArrayDestroy(&target[i].a);
    }
}

static void SwapFringeArrays(void) {
    PaddedPositionArray *tmp = fringe;
    fringe = discovered;
    discovered = tmp;
}

static void SwapFringeBitsets(void) {
    ConcurrentBitset *tmp = bs_fringe;
    bs_fringe = bs_discovered;
    bs_discovered = tmp;
}

/**
 * @brief Expands the given \p parent position and collect information into
 * \p dest . Assumes that \p parent has not been expanded and will be expanded
 * only once by the calling thread.
 *
 * @param parent Position to expand as parent.
 * @param dest Destination Analysis object.
 * @param tid ID of the calling thread.
 * @param use_array If set to \p true , the function will begin by collecting
 * child positions into array discovered until an OOM occurs. Otherwise, the
 * function begins by collecting child positions into bitset discovered, in
 * which case no OOM can occur.
 * @return \p true if \c true was passed to \p use_array and the function did
 * not encounter an OOM error (in other words, the function call successfully
 * completed by collecting all child positions of \p parent into the discovered
 * array with no error), or
 * @return \p false otherwise. There are two cases where this function may
 * return false:
 *  1. \c false was passed to \p use_array ;
 *  2. \p use_array was true but the function encountered OOM while trying to
 * push the child positions into array discovered.
 */
static bool Expand(TierPosition parent, Analysis *dest, int tid,
                   bool use_array) {
    // Do not generate children of primitive positions.
    if (IsPrimitive(parent)) return true;

    TierPosition children[kTierSolverNumChildPositionsMax];
    int num_children;
    if (explore_canonical) {
        num_children = GetCanonicalChildPositions(parent, children, dest);
    } else {
        num_children = GetChildPositions(parent, children, dest);
    }

    for (int64_t i = 0; i < num_children; ++i) {
        if (children[i].tier != this_tier) {
            DiscoverProcessChildTier(children[i]);
        } else if (use_array) {
            use_array = DiscoverProcessThisTierArray(children[i].position, tid);
            i -= (!use_array);  // Reprocess the i-th child if failed.
        } else {                // Already OOM, expand to bitset discovered
            DiscoverProcessThisTierBitset(children[i].position);
        }
    }

    return use_array;
}

// Preconditions:
//   - array fringe is empty initialized
//   - array discovered is empty initialized
//   - bitset fringe contains at least one position to expand
//   - bitset discovered is zero initialized
//
// Output (on success):
//   - array fringe remains empty initialized
//   - array discovered contains all discoverable positions from bitset fringe
//   - bitset fringe remains unmodified
//   - bitset discovered remains zero initialized
//
// Output (on OOM):
//   - array fringe remains empty initialized
//   - array discovered contains positions discovered before OOM
//   - bitset fringe remains unmodified
//   - bitset discovered contains positions discovered after OOM
static bool DiscoverFromBitsetToArray(Analysis *dest) {
    ConcurrentBool no_oom;
    ConcurrentBoolInit(&no_oom, true);
    CacheAlignedAnalysis *parts = MakePartialAnalyses();
    if (parts == NULL) {
        fprintf(stderr, "DiscoverFromBitsetToArray: (BUG) unexpected OOM\n");
        ConcurrentBoolStore(&no_oom, false);
        goto _bailout;
    }

    PRAGMA_OMP_PARALLEL {
        int tid = ConcurrencyGetOmpThreadId();
        PRAGMA_OMP_FOR_SCHEDULE_DYNAMIC(1024)
        for (int64_t i = 0; i < this_tier_size; ++i) {
            // Skip positions that are not in the fringe
            if (!ConcurrentBitsetTest(bs_fringe, i, memory_order_relaxed)) {
                continue;
            }

            bool use_array = ConcurrentBoolLoad(&no_oom);
            TierPosition parent = {.tier = this_tier, .position = i};
            bool step_no_oom = Expand(parent, &parts[tid].data, tid, use_array);
            if (!step_no_oom) ConcurrentBoolStore(&no_oom, false);
        }
    }
    MergePartialAnalysisMoves(dest, parts);

_bailout:
    GamesmanAllocatorDeallocate(allocator, parts);

    return ConcurrentBoolLoad(&no_oom);
}

// Preconditions:
//   - array fringe is loaded with at least one element,
//   - array discovered is empty initialized
//   - bitset fringe is zero initialized
//   - bitset discovered is zero initialized
//
// Output (on success):
//   - array fringe is unmodified
//   - array discovered contains all discoverable positions from array fringe
//   - bitset fringe remains zero initialized
//   - bitset discovered remains zero initialized
//
// Output (on OOM):
//   - array fringe is unmodified
//   - array discovered contains positions discovered before OOM
//   - bitset fringe remains zero initialized
//   - bitset discovered contains positions discovered after OOM
static bool DiscoverFromArrayToArray(Analysis *dest) {
    ConcurrentBool no_oom;
    ConcurrentBoolInit(&no_oom, true);

    // Allocate space
    int64_t *fringe_offsets = MakeFringeOffsets(fringe);
    CacheAlignedAnalysis *parts = MakePartialAnalyses();
    if (fringe_offsets == NULL || parts == NULL) {
        fprintf(stderr, "DiscoverFromArrayToArray: (BUG) unexpected OOM\n");
        ConcurrentBoolStore(&no_oom, false);
        goto _bailout;
    }

    PRAGMA_OMP_PARALLEL {
        int tid = ConcurrencyGetOmpThreadId();
        int fringe_id = 0;
        PRAGMA_OMP_FOR_SCHEDULE_MONOTONIC_DYNAMIC(1024)
        for (int64_t i = 0; i < fringe_offsets[num_threads]; ++i) {
            UpdateFringeId(&fringe_id, i, fringe_offsets);
            int64_t index_in_fringe = i - fringe_offsets[fringe_id];
            TierPosition parent = {
                .tier = this_tier,
                .position = fringe[fringe_id].a.array[index_in_fringe],
            };
            bool use_array = ConcurrentBoolLoad(&no_oom);
            bool step_no_oom = Expand(parent, &parts[tid].data, tid, use_array);
            if (!step_no_oom) ConcurrentBoolStore(&no_oom, false);
        }
    }
    MergePartialAnalysisMoves(dest, parts);

_bailout:
    GamesmanAllocatorDeallocate(allocator, fringe_offsets);
    GamesmanAllocatorDeallocate(allocator, parts);

    return ConcurrentBoolLoad(&no_oom);
}

// Preconditions:
//   - array fringe is empty initialized
//   - array discovered contains positions discovered before OOM
//   - bitset fringe is empty initialized
//   - bitset discovered contains positions that were discovered after OOM
//
// Output:
//   - array fringe remains empty initialized
//   - array discovered becomes empty initialized
//   - bitset fringe contains the union of array discovered and bitset
//   discovered.
//   - bitset discovered becomes empty initialized
static void MergeDiscoveredToBitsetFringe(void) {
    // Swap bitset fringe with bitset discovered
    SwapFringeBitsets();

    // Transfer positions from array discovered to bitset fringe
    int64_t *fringe_offsets = MakeFringeOffsets(discovered);
    if (fringe_offsets == NULL) {
        fprintf(stderr,
                "MergeDiscoveredToBitsetFringe: (BUG) unexpected OOM\n");
        NotReached("Terminating...\n");
    }

    PRAGMA_OMP_PARALLEL {
        int fringe_id = 0;
        PRAGMA_OMP_FOR_SCHEDULE_MONOTONIC_DYNAMIC(1024)
        for (int64_t i = 0; i < fringe_offsets[num_threads]; ++i) {
            UpdateFringeId(&fringe_id, i, fringe_offsets);
            int64_t index_in_fringe = i - fringe_offsets[fringe_id];
            Position pos = discovered[fringe_id].a.array[index_in_fringe];
            ConcurrentBitsetSet(bs_fringe, pos, memory_order_relaxed);
        }
    }
    GamesmanAllocatorDeallocate(allocator, fringe_offsets);
}

static void Step2Discover(Analysis *dest) {
    enum State { ArrayToArray, BitsetToArray };
    enum State state = BitsetToArray;
    while (state != ArrayToArray || GetFringeSize() > 0) {
        bool no_oom;
        if (state == ArrayToArray) {
            no_oom = DiscoverFromArrayToArray(dest);
            DestroyFringeArray(fringe);
            InitFringeArray(fringe);
        } else {
            assert(GetFringeSize() == 0);
            no_oom = DiscoverFromBitsetToArray(dest);
            ConcurrentBitsetResetAll(bs_fringe);
        }

        if (no_oom) {
            SwapFringeArrays();
            state = ArrayToArray;
        } else {
            MergeDiscoveredToBitsetFringe();
            state = BitsetToArray;
        }
    }
}

// Step3SaveChildMaps

static bool Step3SaveChildMaps(void) {
    for (int64_t i = 0; i < num_child_tiers; ++i) {
        int error =
            StatManagerSaveDiscoveryMap(child_tier_maps[i], child_tiers[i]);
        if (error != 0) return false;
        ConcurrentBitsetDestroy(child_tier_maps[i]);
        child_tier_maps[i] = NULL;
    }
    GamesmanAllocatorDeallocate(allocator, child_tier_maps);
    child_tier_maps = NULL;

    return true;
}

// Step4Analyze

static bool Step4Analyze(Analysis *dest) {
    int error = DbManagerLoadTier(this_tier, this_tier_size);
    if (error != kNoError) return false;

    CacheAlignedAnalysis *parts = MakePartialAnalyses();
    if (parts == NULL) {
        fprintf(stderr, "Step4Analyze: (BUG) unexpected OOM\n");
        NotReached("Terminating...\n");
    }

    ConcurrentBool success;
    ConcurrentBoolInit(&success, true);
    PRAGMA_OMP_PARALLEL {
        int tid = ConcurrencyGetOmpThreadId();
        PRAGMA_OMP_FOR_SCHEDULE_DYNAMIC(1024)
        for (int64_t i = 0; i < this_tier_size; ++i) {
            if (!ConcurrentBoolLoad(&success)) continue;  // fail fast.

            // Skip if the current position is not reachable.
            if (!ConcurrentBitsetTest(this_tier_map, i, memory_order_relaxed)) {
                continue;
            }

            TierPosition tp = {.tier = this_tier, .position = i};
            if (explore_canonical) {
                Value value =
                    DbManagerGetValueFromLoaded(this_tier, tp.position);
                int remoteness =
                    DbManagerGetRemotenessFromLoaded(this_tier, tp.position);
                int num_symmetries = api_internal->GetNumberOfSymmetries(tp);
                int error = AnalysisCountGroup(
                    &parts[tid].data, tp, num_symmetries, value, remoteness);
                if (error != 0) ConcurrentBoolStore(&success, false);
            } else {
                // Must probe canonical positions as the original might not be
                // solved.
                Position canonical = api_internal->GetCanonicalPosition(tp);
                Value value = DbManagerGetValueFromLoaded(this_tier, canonical);
                int remoteness =
                    DbManagerGetRemotenessFromLoaded(this_tier, canonical);
                bool is_canonical = (tp.position == canonical);
                int error = AnalysisCount(&parts[tid].data, tp, value,
                                          remoteness, is_canonical);
                if (error != 0) ConcurrentBoolStore(&success, false);
            }
        }
    }
    MergePartialAnalysisCounts(dest, parts);
    GamesmanAllocatorDeallocate(allocator, parts);

    error = DbManagerUnloadTier(this_tier);
    if (error != kNoError) ConcurrentBoolStore(&success, false);
    ConcurrentBitsetDestroy(this_tier_map);
    this_tier_map = NULL;

    return ConcurrentBoolLoad(&success);
}

// Step5SaveAnalysis

static bool Step5SaveAnalysis(const Analysis *dest) {
    bool success = (StatManagerSaveAnalysis(this_tier, dest) == 0);
    if (!success) return false;

    if (this_tier != api_internal->GetInitialTier()) {
        StatManagerRemoveDiscoveryMap(this_tier);
    }
    return true;
}

// Step6CleanUp

static void Step6CleanUp(void) {
    num_child_tiers = 0;
    TierHashMapDestroy(&child_tier_to_index);
    ConcurrentBitsetDestroy(this_tier_map);
    this_tier_map = NULL;
    for (int i = 0; i < num_child_tiers; ++i) {
        ConcurrentBitsetDestroy(child_tier_maps[i]);
    }
    GamesmanAllocatorDeallocate(allocator, child_tier_maps);
    child_tier_maps = NULL;

    // Clean up fringes.
    DestroyFringeArray(fringe);
    GamesmanAllocatorDeallocate(allocator, fringe);
    fringe = NULL;
    DestroyFringeArray(discovered);
    GamesmanAllocatorDeallocate(allocator, discovered);
    discovered = NULL;
    ConcurrentBitsetDestroy(bs_fringe);
    bs_fringe = NULL;
    ConcurrentBitsetDestroy(bs_discovered);
    bs_discovered = NULL;
}

static int AnalysisStatus(Tier tier) { return StatManagerGetStatus(tier); }

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
    Step2Discover(dest);
    if (!Step3SaveChildMaps()) goto _bailout;
    if (!Step4Analyze(dest)) goto _bailout;
    if (!Step5SaveAnalysis(dest)) goto _bailout;

_done:
    ret = 0;

_bailout:
    Step6CleanUp();
    return ret;
}

// =========================== TierAnalyzerFinalize ===========================

void TierAnalyzerFinalize(void) {
    api_internal = NULL;
    GamesmanAllocatorRelease(allocator);
}
