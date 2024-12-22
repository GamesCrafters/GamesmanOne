/**
 * @file bi.c
 * @author Max Delgadillo: designed and implemented the original version
 * of the backward induction algorithm (solveretrograde.c in GamesmanClassic.)
 * @author Robert Shi (robertyishi@berkeley.edu): Separated functions for
 * solving of a single tier into its own module, implemented multithreading
 * using OpenMP, and reformatted functions for readability.
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Backward induction tier worker algorithm implementation.
 * @version 1.1.2
 * @date 2024-12-22
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

#include "core/solvers/tier_solver/tier_worker/bi.h"

#include <assert.h>   // assert
#include <stdbool.h>  // bool, true, false
#include <stddef.h>   // NULL
#include <stdint.h>   // int64_t
#include <stdio.h>    // fprintf, stderr
#include <stdlib.h>   // calloc, malloc, free
#include <string.h>   // memcpy

#include "core/concurrency.h"
#include "core/constants.h"
#include "core/db/db_manager.h"
#include "core/solvers/tier_solver/tier_solver.h"
#include "core/solvers/tier_solver/tier_worker/frontier.h"
#include "core/solvers/tier_solver/tier_worker/reverse_graph.h"
#include "core/types/gamesman_types.h"

// Include and use OpenMP if the _OPENMP flag is set.
#ifdef _OPENMP
#include <omp.h>
#endif  // _OPENMP

// Note on multithreading:
//   Be careful that "if (!condition) ConcurrentBoolStore(&success, false);" is
//   not equivalent to "ConcurrentBoolStore(&success, success & condition);" or
//   "ConcurrentBoolStore(&success, condition);". The former creates a race
//   condition whereas the latter may overwrite an already failing result.

// Copy of the API functions from tier_manager. Cannot use a reference here
// because we need to create/modify some of the functions.
static TierSolverApi current_api;

static int64_t current_db_chunk_size;

// A frontier array will be created for each possible remoteness.
static const int kFrontierSize = kRemotenessMax + 1;

static Tier this_tier;          // The tier being solved.
static int64_t this_tier_size;  // Size of the tier being solved.
// Array of child tiers with this_tier appended to the back.
static TierArray child_tiers;

// A frontier contains solved but unprocessed positions.
static Frontier *win_frontiers;   // Winning frontiers for each thread.
static Frontier *lose_frontiers;  // Losing frontiers for each thread.
static Frontier *tie_frontiers;   // Tying frontiers for each thread.

// Number of undecided child positions array (malloc'ed and owned by the
// TierWorkerSolve function). Note that we are assuming the number of children
// of ANY position is no more than 254. This allows us to use an unsigned 8-bit
// integer to save memory. If this assumption no longer holds for any new
// games in the future, the programmer should change this type to a wider
// integer type such as int16_t.
typedef int16_t ChildPosCounterType;
#ifdef _OPENMP
typedef _Atomic ChildPosCounterType AtomicChildPosCounterType;
static AtomicChildPosCounterType *num_undecided_children = NULL;
#else   // _OPENMP not defined
static ChildPosCounterType *num_undecided_children = NULL;
#endif  // _OPENMP

// Cached reverse position graph of the current tier. This is only initialized
// if the game does not implement Retrograde Analysis.
static ReverseGraph reverse_graph;
// The reverse graph is used if the Retrograde Analysis is turned off.
static bool use_reverse_graph = false;

static int num_threads;  // Number of threads available.

// ------------------------------ Step0Initialize ------------------------------

static bool Step0_1InitFrontiers(int dividers_size) {
#ifdef _OPENMP
    num_threads = omp_get_max_threads();
#else   // _OPENMP not defined.
    num_threads = 1;
#endif  // _OPENMP
    win_frontiers = (Frontier *)malloc(num_threads * sizeof(Frontier));
    lose_frontiers = (Frontier *)malloc(num_threads * sizeof(Frontier));
    tie_frontiers = (Frontier *)malloc(num_threads * sizeof(Frontier));
    if (!win_frontiers || !lose_frontiers || !tie_frontiers) {
        return false;
    }

    bool success = true;
    for (int i = 0; i < num_threads; ++i) {
        success &=
            FrontierInit(&win_frontiers[i], kFrontierSize, dividers_size);
        success &=
            FrontierInit(&lose_frontiers[i], kFrontierSize, dividers_size);
        success &=
            FrontierInit(&tie_frontiers[i], kFrontierSize, dividers_size);
    }

    return success;
}

static bool Step0_0SetupChildTiers(void) {
    TierArray raw = current_api.GetChildTiers(this_tier);
    if (raw.size == kIllegalSize) return false;

    TierHashSet dedup;
    TierHashSetInit(&dedup, 0.5);
    TierArrayInit(&child_tiers);
    for (int64_t i = 0; i < raw.size; ++i) {
        Tier canonical = current_api.GetCanonicalTier(raw.array[i]);

        // Another child tier is symmetric to this one and was already added.
        if (TierHashSetContains(&dedup, canonical)) continue;

        TierHashSetAdd(&dedup, canonical);
        TierArrayAppend(&child_tiers, canonical);
    }
    TierHashSetDestroy(&dedup);
    TierArrayDestroy(&raw);

    return true;
}

static PositionArray TierGetCanonicalParentPositionsFromReverseGraph(
    TierPosition child, Tier parent_tier) {
    // Unused, since all children were generated by positions in this_tier.
    (void)parent_tier;
    return ReverseGraphPopParentsOf(&reverse_graph, child);
}

static bool Step0Initialize(const TierSolverApi *api, int64_t db_chunk_size,
                            Tier tier) {
    // Copy solver API function pointers and set db chunk size.
    memcpy(&current_api, api, sizeof(current_api));
    current_db_chunk_size = db_chunk_size;

    // Initialize child tier array.
    this_tier = tier;
    this_tier_size = current_api.GetTierSize(tier);
    if (!Step0_0SetupChildTiers()) return false;

    // Initialize reverse graph without this_tier in the child_tiers array.
    use_reverse_graph = (current_api.GetCanonicalParentPositions == NULL);
    if (use_reverse_graph) {
        bool success = ReverseGraphInit(&reverse_graph, &child_tiers, this_tier,
                                        current_api.GetTierSize);
        if (!success) return false;
        current_api.GetCanonicalParentPositions =
            &TierGetCanonicalParentPositionsFromReverseGraph;
    }

    // From this point on, child_tiers will also contain this_tier.
    TierArrayAppend(&child_tiers, this_tier);

    // Initialize frontiers with size to hold all child tiers and this tier.
    if (!Step0_1InitFrontiers((int)(child_tiers.size))) return false;

    return true;
}

// ----------------------------- Step1LoadChildren -----------------------------

static int GetThreadId(void) {
#ifdef _OPENMP
    return omp_get_thread_num();
#else   // _OPENMP not defined, thread 0 is the only available thread.
    return 0;
#endif  // _OPENMP
}

static bool CheckAndLoadFrontier(int child_index, int64_t position, Value value,
                                 int remoteness, int tid) {
    if (remoteness < 0) return false;  // Error probing remoteness.
    if (value == kUndecided || value == kDraw) return true;
    Frontier *dest = NULL;
    switch (value) {
        case kUndecided:
        case kDraw:
            return true;

        case kWin:
            dest = &win_frontiers[tid];
            break;

        case kLose:
            dest = &lose_frontiers[tid];
            break;

        case kTie:
            dest = &tie_frontiers[tid];
            break;

        default:
            return false;  // Error probing value.
    }
    return FrontierAdd(dest, position, remoteness, child_index);
}

static bool Step1_0LoadTierHelper(int child_index) {
    Tier child_tier = child_tiers.array[child_index];

    // Scan child tier and load non-drawing positions into frontier.
    int64_t child_tier_size = current_api.GetTierSize(child_tier);
    ConcurrentBool success;
    ConcurrentBoolInit(&success, true);

    PRAGMA_OMP_PARALLEL {
        DbProbe probe;
        DbManagerProbeInit(&probe);
        TierPosition child_tier_position = {.tier = child_tier};
        int tid = GetThreadId();
        PRAGMA_OMP_FOR_SCHEDULE_DYNAMIC(current_db_chunk_size)
        for (Position position = 0; position < child_tier_size; ++position) {
            child_tier_position.position = position;
            Value value = DbManagerProbeValue(&probe, child_tier_position);
            int remoteness =
                DbManagerProbeRemoteness(&probe, child_tier_position);
            if (!CheckAndLoadFrontier(child_index, position, value, remoteness,
                                      tid)) {
                ConcurrentBoolStore(&success, false);
            }
        }
        DbManagerProbeDestroy(&probe);
    }

    return ConcurrentBoolLoad(&success);
}

/**
 * @brief Load all non-drawing positions from all child tiers into frontier.
 */
static bool Step1LoadChildren(void) {
    // Child tiers must be processed sequentially, otherwise the frontier
    // dividers wouldn't work.
    int num_child_tiers = (int)child_tiers.size - 1;
    for (int child_index = 0; child_index < num_child_tiers; ++child_index) {
        // Load child tier from disk.
        if (!Step1_0LoadTierHelper(child_index)) return false;
    }

    return true;
}

// -------------------------- Step2SetupSolverArrays --------------------------

/**
 * @brief Initializes database and number of undecided children array.
 */
static bool Step2SetupSolverArrays(void) {
    int error = DbManagerCreateSolvingTier(this_tier,
                                           current_api.GetTierSize(this_tier));
    if (error != 0) return false;

#ifdef _OPENMP
    num_undecided_children = (AtomicChildPosCounterType *)malloc(
        this_tier_size * sizeof(AtomicChildPosCounterType));
    if (num_undecided_children == NULL) return false;

    for (int64_t i = 0; i < this_tier_size; ++i) {
        atomic_init(&num_undecided_children[i], 0);
    }
#else   // _OPENMP not defined
    num_undecided_children = (ChildPosCounterType *)calloc(
        this_tier_size, sizeof(ChildPosCounterType));
#endif  // _OPENMP

    return (num_undecided_children != NULL);
}

// ------------------------------- Step3ScanTier -------------------------------

static bool IsCanonicalPosition(Position position) {
    TierPosition tier_position = {.tier = this_tier, .position = position};
    return current_api.GetCanonicalPosition(tier_position) == position;
}

static ChildPosCounterType Step3_0CountChildren(Position position) {
    TierPosition tier_position = {.tier = this_tier, .position = position};
    if (!use_reverse_graph) {
        return (ChildPosCounterType)
            current_api.GetNumberOfCanonicalChildPositions(tier_position);
    }
    // Else, count children manually and add position as their parent in the
    // reverse graph.
    TierPositionArray children =
        current_api.GetCanonicalChildPositions(tier_position);

    for (int64_t i = 0; i < children.size; ++i) {
        if (!ReverseGraphAdd(&reverse_graph, children.array[i], position)) {
            return -1;
        }
    }
    ChildPosCounterType num_children = (ChildPosCounterType)children.size;
    TierPositionArrayDestroy(&children);

    return num_children;
}

static void SetNumUndecidedChildren(Position pos, ChildPosCounterType value) {
#ifdef _OPENMP
    atomic_store_explicit(&num_undecided_children[pos], value,
                          memory_order_relaxed);
#else   // _OPENMP not defined
    num_undecided_children[pos] = value;
#endif  // _OPENMP
}

/**
 * @brief Counts the number of children of all positions in current tier and
 * loads primitive positions into frontier.
 */
static bool Step3ScanTier(void) {
    ConcurrentBool success;
    ConcurrentBoolInit(&success, true);

    PRAGMA_OMP_PARALLEL {
        int tid = GetThreadId();
        PRAGMA_OMP_FOR_SCHEDULE_DYNAMIC(128)
        for (Position position = 0; position < this_tier_size; ++position) {
            TierPosition tier_position = {.tier = this_tier,
                                          .position = position};

            // Skip illegal positions and non-canonical positions.
            if (!current_api.IsLegalPosition(tier_position) ||
                !IsCanonicalPosition(position)) {
                SetNumUndecidedChildren(position, 0);
                continue;
            }

            Value value = current_api.Primitive(tier_position);
            if (value != kUndecided) {  // If tier_position is primitive...
                // Set its value immediately and push it into the frontier.
                DbManagerSetValue(position, value);
                DbManagerSetRemoteness(position, 0);
                int this_tier_index = (int)child_tiers.size - 1;
                if (!CheckAndLoadFrontier(this_tier_index, position, value, 0,
                                          tid)) {
                    ConcurrentBoolStore(&success, false);
                }
                SetNumUndecidedChildren(position, 0);
                continue;
            }  // Execute the following lines if tier_position is not primitive.
            ChildPosCounterType num_children = Step3_0CountChildren(position);
            if (num_children <= 0)
                // Either OOM or no children.
                ConcurrentBoolStore(&success, false);
            SetNumUndecidedChildren(position, num_children);
        }
    }

    for (int i = 0; i < num_threads; ++i) {
        FrontierAccumulateDividers(&win_frontiers[i]);
        FrontierAccumulateDividers(&lose_frontiers[i]);
        FrontierAccumulateDividers(&tie_frontiers[i]);
    }

    return ConcurrentBoolLoad(&success);
}

// ---------------------------- Step4PushFrontierUp ----------------------------

static int64_t *MakeFrontierOffsets(const Frontier *frontiers, int remoteness) {
    int64_t *frontier_offsets =
        (int64_t *)calloc(num_threads + 1, sizeof(int64_t));
    if (frontier_offsets == NULL) return NULL;

    frontier_offsets[0] = 0;
    for (int i = 1; i <= num_threads; ++i) {
        frontier_offsets[i] =
            frontier_offsets[i - 1] + frontiers[i - 1].buckets[remoteness].size;
    }

    return frontier_offsets;
}

// This function assumes frontier_id and child_index passed in corresponds to
// a chunk of positions that either contains the i-th position in the array of
// all positions, or corresponds to a chunk that comes later in the array.
static void UpdateFrontierAndChildTierIds(int64_t i, const Frontier *frontiers,
                                          int *frontier_id, int *child_index,
                                          int remoteness,
                                          const int64_t *frontier_offsets) {
    while (i >= frontier_offsets[*frontier_id + 1]) {
        ++(*frontier_id);
        *child_index = 0;
    }
    int64_t index_in_frontier = i - frontier_offsets[*frontier_id];
    while (index_in_frontier >=
           frontiers[*frontier_id].dividers[remoteness][*child_index]) {
        ++(*child_index);
    }
}

/**
 * @details The algorithm is as follows: first count the total number N of
 * positions that need to be processed and then run a parallel for loop that
 * ranges from 0 to N-1 to process each position. In order for threads to figure
 * out which tier a position belongs to, it must first figure out which frontier
 * that position was taken from, and then use the corresponding "dividers" array
 * together with the child_tiers array to figure out which tier that position
 * is from.
 *
 * This function first inspects all positions in the frontier (multiple
 * Frontier instances if multithreading) at the given remoteness that needs to
 * be processed, and creates an array of offsets that allows us to determine
 * which Frontier instance a position belongs to. Then it uses the helper
 * function UpdateFrontierAndChildTierIds to figure out which frontier and which
 * child tier a position was from. The helper function is designed to take in
 * the old values of frontier_id and child_index as hints on where to begin
 * searching. This allows us to not start the search at index 0 for every
 * position. However, it also means that we are assuming an order of processing
 * within each tier. If the order is random, the hints will not work correctly.
 */
static bool PushFrontierHelper(
    Frontier *frontiers, int remoteness,
    bool (*ProcessPosition)(int remoteness, TierPosition tier_position)) {
    //
    int64_t *frontier_offsets = MakeFrontierOffsets(frontiers, remoteness);
    if (!frontier_offsets) return false;

    ConcurrentBool success;
    ConcurrentBoolInit(&success, true);
    PRAGMA_OMP_PARALLEL {
        int frontier_id = 0, child_index = 0;
        PRAGMA_OMP_FOR_SCHEDULE_DYNAMIC(16)
        for (int64_t i = 0; i < frontier_offsets[num_threads]; ++i) {
            UpdateFrontierAndChildTierIds(i, frontiers, &frontier_id,
                                          &child_index, remoteness,
                                          frontier_offsets);
            int64_t index_in_frontier = i - frontier_offsets[frontier_id];
            TierPosition tier_position = {
                .tier = child_tiers.array[child_index],
                .position = FrontierGetPosition(&frontiers[frontier_id],
                                                remoteness, index_in_frontier),
            };
            if (!ProcessPosition(remoteness, tier_position)) {
                ConcurrentBoolStore(&success, false);
            }
        }
    }

    // Free current remoteness from all frontiers.
    for (int i = 0; i < num_threads; ++i) {
        FrontierFreeRemoteness(&frontiers[i], remoteness);
    }
    free(frontier_offsets);
    frontier_offsets = NULL;

    return ConcurrentBoolLoad(&success);
}

// This function is called within a OpenMP parallel region.
static bool ProcessLoseOrTiePosition(int remoteness, TierPosition tier_position,
                                     bool processing_lose) {
    PositionArray parents =
        current_api.GetCanonicalParentPositions(tier_position, this_tier);
    if (parents.size < 0) {  // OOM.
        TierArrayDestroy(&parents);
        return false;
    }

    int tid = GetThreadId();
    Value value = processing_lose ? kWin : kTie;
    Frontier *frontier =
        processing_lose ? &win_frontiers[tid] : &tie_frontiers[tid];
    for (int64_t i = 0; i < parents.size; ++i) {
#ifdef _OPENMP
        // Atomically fetch num_undecided_children[parents.array[i]] into
        // child_remaining and set it to zero.
        ChildPosCounterType child_remaining = atomic_exchange_explicit(
            &num_undecided_children[parents.array[i]], 0, memory_order_relaxed);
#else                                        // _OPENMP not defined
        ChildPosCounterType child_remaining =
            num_undecided_children[parents.array[i]];
        num_undecided_children[parents.array[i]] = 0;
#endif                                       // _OPENMP
        if (child_remaining == 0) continue;  // Parent already solved.

        // All parents are win/tie in (remoteness + 1) positions.
        DbManagerSetValue(parents.array[i], value);
        DbManagerSetRemoteness(parents.array[i], remoteness + 1);
        int this_tier_index = (int)child_tiers.size - 1;
        bool success = FrontierAdd(frontier, parents.array[i], remoteness + 1,
                                   this_tier_index);
        if (!success) {  // OOM.
            TierArrayDestroy(&parents);
            return false;
        }
    }
    TierArrayDestroy(&parents);
    return true;
}

static bool ProcessLosePosition(int remoteness, TierPosition tier_position) {
    return ProcessLoseOrTiePosition(remoteness, tier_position, true);
}

#ifdef _OPENMP
/**
 * @brief Atomically decrements the content of OBJ, which is of type unsigned
 * char, if and only if it was greater than 0, and returns the original value.
 * If multiple threads call this function on the same OBJ at the same time, the
 * value returned is guaranteed to be unique for each thread if no threads are
 * performing other operations on OBJ.
 *
 * @note This is a helper function that is only used by ProcessWinPosition.
 *
 * @param obj Atomic ChildPosCounterType object to be decremented.
 * @return Original value of OBJ.
 */
static ChildPosCounterType DecrementIfNonZero(AtomicChildPosCounterType *obj) {
    ChildPosCounterType current_value =
        atomic_load_explicit(obj, memory_order_relaxed);
    while (current_value != 0) {
        // This function will set OBJ to current_value - 1 if OBJ is still equal
        // to current_value. Otherwise, it updates current_value to the new
        // value of OBJ and returns false.
        bool success = atomic_compare_exchange_strong_explicit(
            obj, &current_value, (ChildPosCounterType)(current_value - 1),
            memory_order_relaxed, memory_order_relaxed);
        // If decrement was successful, quit and return the original value of
        // OBJ that was swapped out.
        if (success) return current_value;
        // Otherwise, we keep looping until either a decrement was successfully
        // completed, or OBJ is decremented to zero by some other thread.
    }

    return 0;
}
#endif  // _OPENMP

// This function is called within a OpenMP parallel region.
static bool ProcessWinPosition(int remoteness, TierPosition tier_position) {
    PositionArray parents =
        current_api.GetCanonicalParentPositions(tier_position, this_tier);
    if (parents.size < 0) {  // OOM.
        PositionArrayDestroy(&parents);
        return false;
    }

    int tid = GetThreadId();
    for (int64_t i = 0; i < parents.size; ++i) {
#ifdef _OPENMP
        ChildPosCounterType child_remaining =
            DecrementIfNonZero(&num_undecided_children[parents.array[i]]);
#else   // _OPENMP not defined
        // If this parent has been solved already, skip it.
        if (num_undecided_children[parents.array[i]] == 0) continue;
        // Must perform the above check before decrementing to prevent overflow.
        ChildPosCounterType child_remaining =
            num_undecided_children[parents.array[i]]--;
#endif  // _OPENMP
        // If this child position is the last undecided child of parent
        // position, mark parent as lose in (childRmt + 1).
        if (child_remaining == 1) {
            DbManagerSetValue(parents.array[i], kLose);
            DbManagerSetRemoteness(parents.array[i], remoteness + 1);
            int this_tier_index = (int)child_tiers.size - 1;
            bool success = FrontierAdd(&lose_frontiers[tid], parents.array[i],
                                       remoteness + 1, this_tier_index);
            if (!success) {  // OOM.
                PositionArrayDestroy(&parents);
                return false;
            }
        }
    }
    PositionArrayDestroy(&parents);
    return true;
}

static bool ProcessTiePosition(int remoteness, TierPosition tier_position) {
    return ProcessLoseOrTiePosition(remoteness, tier_position, false);
}

static void DestroyFrontiers(void) {
    for (int i = 0; i < num_threads; ++i) {
        if (win_frontiers) FrontierDestroy(&win_frontiers[i]);
        if (lose_frontiers) FrontierDestroy(&lose_frontiers[i]);
        if (tie_frontiers) FrontierDestroy(&tie_frontiers[i]);
    }
    free(win_frontiers);
    win_frontiers = NULL;
    free(lose_frontiers);
    lose_frontiers = NULL;
    free(tie_frontiers);
    tie_frontiers = NULL;
}

/**
 * @brief Pushes frontier up.
 */
static bool Step4PushFrontierUp(void) {
    // Process winning and losing positions first.
    // Remotenesses must be processed sequentially.
    for (int remoteness = 0; remoteness < kFrontierSize; ++remoteness) {
        if (!PushFrontierHelper(lose_frontiers, remoteness,
                                &ProcessLosePosition)) {
            return false;
        } else if (!PushFrontierHelper(win_frontiers, remoteness,
                                       &ProcessWinPosition)) {
            return false;
        }
    }

    // Then move on to tying positions.
    for (int remoteness = 0; remoteness < kFrontierSize; ++remoteness) {
        if (!PushFrontierHelper(tie_frontiers, remoteness, &ProcessTiePosition))
            return false;
    }
    DestroyFrontiers();
    TierArrayDestroy(&child_tiers);
    ReverseGraphDestroy(&reverse_graph);
    return true;
}

// -------------------------- Step5MarkDrawPositions --------------------------

static ChildPosCounterType GetNumUndecidedChildren(Position pos) {
#ifdef _OPENMP
    return atomic_load_explicit(&num_undecided_children[pos],
                                memory_order_relaxed);
#else   // _OPENMP not defined
    return num_undecided_children[pos];
#endif  // _OPENMP
}

static void Step5MarkDrawPositions(void) {
    PRAGMA_OMP_PARALLEL_FOR
    for (Position position = 0; position < this_tier_size; ++position) {
        if (GetNumUndecidedChildren(position) > 0) {
            // A position is drawing if it still has undecided children.
            DbManagerSetValue(position, kDraw);
            continue;
        }
    }
    free(num_undecided_children);
    num_undecided_children = NULL;
}

// ------------------------------ Step6SaveValues ------------------------------

static void Step6SaveValues(void) {
    if (DbManagerFlushSolvingTier(NULL) != 0) {
        fprintf(stderr,
                "Step6SaveValues: an error has occurred while flushing of the "
                "current tier. The database file for tier %" PRITier
                " may be corrupt.\n",
                this_tier);
    }
    if (DbManagerFreeSolvingTier() != 0) {
        fprintf(stderr,
                "Step6SaveValues: an error has occurred while freeing of the "
                "current tier's in-memory database. Tier: %" PRITier "\n",
                this_tier);
    }
}

// --------------------------------- CompareDb ---------------------------------

static bool CompareDb(void) {
    DbProbe probe, ref_probe;
    if (DbManagerProbeInit(&probe)) return false;
    if (DbManagerRefProbeInit(&ref_probe)) {
        DbManagerProbeDestroy(&probe);
        return false;
    }

    bool success = true;
    for (Position p = 0; p < this_tier_size; ++p) {
        TierPosition tp = {.tier = this_tier, .position = p};
        Value ref_value = DbManagerRefProbeValue(&ref_probe, tp);
        if (ref_value == kUndecided) continue;

        Value actual_value = DbManagerProbeValue(&probe, tp);
        if (actual_value != ref_value) {
            printf("CompareDb: inconsistent value at tier %" PRITier
                   " position %" PRIPos "\n",
                   this_tier, p);
            success = false;
            goto _bailout;
        }

        int actual_remoteness = DbManagerProbeRemoteness(&probe, tp);
        int ref_remoteness = DbManagerRefProbeRemoteness(&ref_probe, tp);
        if (actual_remoteness != ref_remoteness) {
            printf("CompareDb: inconsistent remoteness at tier %" PRITier
                   " position %" PRIPos "\n",
                   this_tier, p);
            success = false;
            goto _bailout;
        }
    }

_bailout:
    DbManagerProbeDestroy(&probe);
    DbManagerRefProbeDestroy(&ref_probe);
    if (success)
        printf("CompareDb: tier %" PRITier " check passed\n", this_tier);

    return success;
}

// ------------------------------- Step7Cleanup -------------------------------

static void Step7Cleanup(void) {
    this_tier = kIllegalTier;
    this_tier_size = kIllegalSize;
    TierArrayDestroy(&child_tiers);
    DbManagerFreeSolvingTier();
    DestroyFrontiers();
    free(num_undecided_children);
    num_undecided_children = NULL;
    if (use_reverse_graph) {
        ReverseGraphDestroy(&reverse_graph);
        // Unset the local function pointer.
        current_api.GetCanonicalParentPositions = NULL;
    }
    num_threads = 0;
}

// -----------------------------------------------------------------------------
// ------------------------- TierWorkerSolveBIInternal -------------------------
// -----------------------------------------------------------------------------

int TierWorkerSolveBIInternal(const TierSolverApi *api, int64_t db_chunk_size,
                              Tier tier, const TierWorkerSolveOptions *options,
                              bool *solved) {
    if (solved != NULL) *solved = false;
    int ret = kRuntimeError;
    if (!options->force && DbManagerTierStatus(tier) == kDbTierStatusSolved) {
        ret = kNoError;  // Success.
        goto _bailout;
    }

    /* Solver main algorithm. */
    if (!Step0Initialize(api, db_chunk_size, tier)) goto _bailout;
    if (!Step1LoadChildren()) goto _bailout;
    if (!Step2SetupSolverArrays()) goto _bailout;
    if (!Step3ScanTier()) goto _bailout;
    if (!Step4PushFrontierUp()) goto _bailout;
    Step5MarkDrawPositions();
    Step6SaveValues();
    if (options->compare && !CompareDb()) goto _bailout;
    if (solved != NULL) *solved = true;
    ret = kNoError;  // Success.

_bailout:
    Step7Cleanup();
    return ret;
}
