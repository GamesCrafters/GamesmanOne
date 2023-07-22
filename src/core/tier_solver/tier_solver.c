/**
 * @file tier_solver.c
 * @author Robert Shi (robertyishi@berkeley.edu)
 *         GamesCrafters Research Group, UC Berkeley
 *		   Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Loopy tier solver.
 * @version 0.2
 * @date 2023-07-18
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
 *
 * @todo
 * 1. Implement symmetries
 * 2. Link database system
 * 3. Check all malloc error handling
 */

#include "core/tier_solver/tier_solver.h"

#include <assert.h>
#include <inttypes.h>  // PRId64
#include <malloc.h>    // calloc, free
#include <stddef.h>    // NULL
#include <stdint.h>    // int64_t, uing8_t, UINT8_MAX
#include <stdio.h>     // fprintf, stderr
#include <string.h>    // memset

#include "core/gamesman.h"
#include "core/gamesman_types.h"
#include "core/misc.h"
#include "core/naivedb.h"
#include "core/tier_solver/frontier.h"
#include "core/tier_solver/reverse_graph.h"

#ifdef _OPENMP
#include <omp.h>
#define PRAGMA(X) _Pragma(#X)
#define PRAGMA_OMP_PARALLEL_FOR PRAGMA(omp parallel for)
#define PRAGMA_OMP_PARALLEL_FOR_FIRSTPRIVATE(var) \
    PRAGMA(omp parallel for firstprivate(var))
#else
#define PRAGMA
#define PRAGMA_OMP_PARALLEL_FOR
#define PRAGMA_OMP_PARALLEL_FOR_FIRSTPRIVATE(var)
#endif

// Note on multithreading: there is a lot of code with the following pattern
// in this file: if (!condition) success = false; Be careful that this is not
// equivalent to "success &= condition" or "success = condition". The former
// creates a race condition whereas the latter may overwrite an already failing
// result with a TRUE value.

// Largest remoteness expected. Increase this value and recompile if
// this value is not large enough for a specific game.
static const int kFrontierSize = 1024;

// Illegal positions are marked with this number of undecided children.
// This value should be chosen based on the integer type of the
// num_undecided_children array.
static const uint8_t kIllegalNumChildren = UINT8_MAX;

static Tier this_tier;          // The tier being solved.
static int64_t this_tier_size;  // Size of the tier being solved.
static TierArray child_tiers;   // Array of child tiers.

static Frontier win_frontier;   // Solved but unprocessed winning positions.
static Frontier lose_frontier;  // Solved but unprocessed losing positions.
static Frontier tie_frontier;   // Solved but unprocessed tying positions.

// Number of undecided child positions array (heap). Note that we are assuming
// the number of children of ANY position is no more than 254. This allows us to
// use uint8_t to save memory. If this assumption is no longer true for any new
// games in the future, use a wider integer type, such as int16_t, instead.
static uint8_t *num_undecided_children = NULL;

// Cached reverse position graph of the current tier.
ReverseGraph reverse_graph;
static bool use_reverse_graph = false;

#ifdef _OPENMP
// Lock for the above array.
static omp_lock_t num_undecided_children_lock;
#endif

static bool Step0Initialize(Tier tier);
static bool Step0_0InitFrontiers(int dividers_size);
static PositionArray TierGetCanonicalParentPositionsFromReverseGraph(
    Tier tier, Position position, Tier parent_tier);

static bool Step1LoadChildren(void);
static bool IsCanonicalTier(Tier tier);
static bool Step1_0LoadCanonicalTier(int child_index);
static bool Step1_1LoadNonCanonicalTier(int child_index);
static bool CheckAndLoadFrontier(int child_index, int64_t position, Value value,
                                 int remoteness);

static bool Step2SetupSolverArrays(void);

static bool Step3ScanTier(void);
static bool IsCanonicalPosition(Position position);
static int Step3_0CountChildrenAndAddToReverseGraph(Position position);

static bool Step4PushFrontierUp(void);
static bool PushFrontierHelper(Frontier *frontier, int remoteness,
                               bool (*ProcessPosition)(int remoteness,
                                                       Tier tier,
                                                       Position position));
static int UpdateChildIndex(int child_index, Frontier *frontier, int remoteness,
                            int i);
static bool ProcessLosePosition(int remoteness, Tier tier, Position position);
static bool ProcessWinPosition(int remoteness, Tier tier, Position position);
static bool ProcessTiePosition(int remoteness, Tier tier, Position position);

static void Step5MarkDrawPositions(void);
static void Step6SaveValues(void);
static void Step7Cleanup(void);
static void DestroyFrontiers(void);

// ----------------------------------------------------------------------------
int TierSolverSolve(Tier tier, bool force) {
    int ret = -1;
    if (!force) {
        // Check if tier has already been solved.
    }

    /* Solver main algorithm. */
    if (!Step0Initialize(tier)) goto _bailout;
    if (!Step1LoadChildren()) goto _bailout;
    if (!Step2SetupSolverArrays()) goto _bailout;
    if (!Step3ScanTier()) goto _bailout;
    if (!Step4PushFrontierUp()) goto _bailout;
    Step5MarkDrawPositions();
    Step6SaveValues();
    ret = 0;  // Success.

_bailout:
    Step7Cleanup();
    return ret;
}
// ----------------------------------------------------------------------------

static bool Step0Initialize(Tier tier) {
    // Initialize child tier array.
    child_tiers = tier_solver.GetChildTiers(this_tier);
    if (child_tiers.size == -1) return false;

    use_reverse_graph = (tier_solver.GetCanonicalParentPositions == NULL);
    if (use_reverse_graph) {
        bool success =
            ReverseGraphInit(&reverse_graph, &child_tiers, this_tier);
        if (!success) return false;
        tier_solver.GetCanonicalParentPositions =
            &TierGetCanonicalParentPositionsFromReverseGraph;
    }

    // Initialize frontiers with size to hold all child tiers and this tier.
    if (!Step0_0InitFrontiers(child_tiers.size + 1)) return false;

    // Non-memory-allocating initializations.
    this_tier = tier;
    this_tier_size = tier_solver.GetTierSize(tier);
#ifdef _OPENMP
    omp_init_lock(&num_undecided_children_lock);
#endif
    return true;
}

static bool Step0_0InitFrontiers(int dividers_size) {
    bool success = FrontierInit(&win_frontier, kFrontierSize, dividers_size);
    success &= FrontierInit(&lose_frontier, kFrontierSize, dividers_size);
    success &= FrontierInit(&tie_frontier, kFrontierSize, dividers_size);
    return success;
}

static PositionArray TierGetCanonicalParentPositionsFromReverseGraph(
    Tier tier, Position position, Tier parent_tier) {
    // Unused, since all children were generated by positions in this_tier.
    (void)parent_tier;

    TierPosition tier_position;
    tier_position.tier = tier;
    tier_position.position = position;
    int64_t index = ReverseGraphGetIndex(&reverse_graph, tier_position);
    PositionArray ret = reverse_graph.parents_of[index];

    // Clears the entry in reverse graph since it is no longer needed.
    // The entry will be freed by the caller of this function.
    memset(&reverse_graph.parents_of[index], 0,
           sizeof(reverse_graph.parents_of[index]));
    return ret;
}

/**
 * @brief Load all non-drawing positions from all child tiers into frontier.
 */
static bool Step1LoadChildren(void) {
    // Child tiers must be processed sequentially, otherwise the frontier
    // dividers wouldn't work.
    for (int child_index = 0; child_index < child_tiers.size; ++child_index) {
        // Load child tier from disk.
        bool childIsCanonical = IsCanonicalTier(child_tiers.array[child_index]);
        bool success;
        if (childIsCanonical) {
            success = Step1_0LoadCanonicalTier(child_index);
        } else {
            success = Step1_1LoadNonCanonicalTier(child_index);
        }
        if (!success) return false;
    }
    return true;
}

static bool IsCanonicalTier(Tier tier) {
    return tier_solver.GetCanonicalTier(tier) == tier;
}

static bool Step1_0LoadCanonicalTier(int child_index) {
    Tier child_tier = child_tiers.array[child_index];
    if (!DbLoadTier(child_tier)) return false;  // OOM.

    // Scan child tier and load non-drawing positions into frontier.
    int64_t child_tier_size = tier_solver.GetTierSize(child_tier);
    bool success = true;

    PRAGMA_OMP_PARALLEL_FOR
    for (Position position = 0; position < child_tier_size; ++position) {
        Value value = DbGetValue(position);
        int remoteness = DbGetRemoteness(position);
        if (!CheckAndLoadFrontier(child_index, position, value, remoteness)) {
            success = false;
        }
    }
    return success;
}

static bool Step1_1LoadNonCanonicalTier(int child_index) {
    Tier original_tier = child_tiers.array[child_index];
    Tier canonical_tier = tier_solver.GetCanonicalTier(original_tier);
    if (!DbLoadTier(canonical_tier)) return false;  // OOM.

    // Scan child tier and load winning/losing positions into frontier.
    int64_t child_tier_size = tier_solver.GetTierSize(canonical_tier);
    bool success = true;

    PRAGMA_OMP_PARALLEL_FOR
    for (int64_t position = 0; position < child_tier_size; ++position) {
        Value value = DbGetValue(position);

        // No need to convert hash if position does not need to be loaded.
        if (value == kUndecided || value == kDraw) continue;

        int remoteness = DbGetRemoteness(position);
        Position noncanonical_position =
            tier_solver.GetPositionInNonCanonicalTier(canonical_tier, position,
                                                      original_tier);
        if (!CheckAndLoadFrontier(child_index, noncanonical_position, value,
                                  remoteness)) {
            success = false;
        }
    }
    return success;
}

static bool CheckAndLoadFrontier(int child_index, int64_t position, Value value,
                                 int remoteness) {
    if (value == kUndecided || value == kDraw) return true;
    Frontier *dest = NULL;
    switch (value) {
        case kUndecided:
        case kDraw:
            return true;

        case kWin:
            dest = &win_frontier;
            break;

        case kLose:
            dest = &lose_frontier;
            break;

        case kTie:
            dest = &tie_frontier;
            break;

        default:
            NotReached("CheckAndLoadFrontier: unknown value.\n");
    }
    return FrontierAdd(dest, position, remoteness, child_index);
}

/**
 * @brief Initializes database and number of undecided children array.
 */
static bool Step2SetupSolverArrays(void) {
    bool success = DbCreateTier(this_tier);
    num_undecided_children = (uint8_t *)calloc(this_tier_size, sizeof(uint8_t));
    return success && (num_undecided_children != NULL);
}

/**
 * @brief Counts the number of children of all positions in current tier and
 * loads primitive positions into frontier.
 */
static bool Step3ScanTier(void) {
    bool success = true;

    PRAGMA_OMP_PARALLEL_FOR
    for (Position position = 0; position < this_tier_size; ++position) {
        if (!tier_solver.IsLegalPosition(this_tier, position) ||
            !IsCanonicalPosition(position)) {
            // Skip illegal positions and non-canonical positions.
            num_undecided_children[position] = kIllegalNumChildren;
            continue;
        }
        Value value = tier_solver.Primitive(this_tier, position);
        bool is_primitive = (value != kUndecided);
        if (is_primitive) {
            DbSetValueRemoteness(position, value, 0);
            if (!CheckAndLoadFrontier(child_tiers.size, position, value, 0)) {
                success = false;
            }
            num_undecided_children[position] = 0;
            continue;
        }
        int num_children;
        if (use_reverse_graph) {
            num_children = Step3_0CountChildrenAndAddToReverseGraph(position);
        } else {
            num_children = tier_solver.GetNumberOfCanonicalChildPositions(
                this_tier, position);
        }
        if (num_children <= 0) success = false;  // Either OOM or no children.
        num_undecided_children[position] = num_children;
    }

    FrontierAccumulateDividers(&win_frontier);
    FrontierAccumulateDividers(&lose_frontier);
    FrontierAccumulateDividers(&tie_frontier);
    return success;
}

static bool IsCanonicalPosition(Position position) {
    return tier_solver.GetCanonicalPosition(this_tier, position) == position;
}

static int Step3_0CountChildrenAndAddToReverseGraph(Position position) {
    TierPositionArray children =
        tier_solver.GetCanonicalChildPositions(this_tier, position);
    for (int64_t i = 0; i < children.size; ++i) {
        if (!ReverseGraphAdd(&reverse_graph, children.array[i], position)) {
            return -1;
        }
    }
    int num_children = (int)children.size;
    TierPositionArrayDestroy(&children);
    return num_children;
}

/**
 * @brief Pushes frontier up.
 */
static bool Step4PushFrontierUp(void) {
    // Process winning and losing positions first.
    // Remotenesses must be processed sequentially.
    for (int remoteness = 0; remoteness < kFrontierSize; ++remoteness) {
        if (!PushFrontierHelper(&lose_frontier, remoteness,
                                &ProcessLosePosition))
            return false;
        if (!PushFrontierHelper(&win_frontier, remoteness, &ProcessWinPosition))
            return false;
    }

    // Then move on to tying positions.
    for (int remoteness = 0; remoteness < kFrontierSize; ++remoteness) {
        if (!PushFrontierHelper(&tie_frontier, remoteness, &ProcessTiePosition))
            return false;
    }
    DestroyFrontiers();
    TierArrayDestroy(&child_tiers);
    ReverseGraphDestroy(&reverse_graph);
    return true;
}

static bool PushFrontierHelper(Frontier *frontier, int remoteness,
                               bool (*ProcessPosition)(int remoteness,
                                                       Tier tier,
                                                       Position position)) {
    bool success = true;
    int child_index = 0;

    PRAGMA_OMP_PARALLEL_FOR_FIRSTPRIVATE(child_index)
    for (int64_t i = 0; i < frontier->buckets[remoteness].size; ++i) {
        child_index = UpdateChildIndex(child_index, frontier, remoteness, i);
        Tier tier = child_index < child_tiers.size
                        ? child_tiers.array[child_index]
                        : this_tier;
        if (!ProcessPosition(remoteness, tier,
                             frontier->buckets[remoteness].array[i])) {
            success = false;
        }
    }
    FrontierFreeRemoteness(frontier, remoteness);
    return success;
}

static int UpdateChildIndex(int child_index, Frontier *frontier, int remoteness,
                            int i) {
    while (i >= frontier->dividers[remoteness][child_index]) {
        ++child_index;
    }
    return child_index;
}

static bool ProcessLoseOrTiePosition(int remoteness, Tier tier,
                                     Position position, bool processing_lose) {
    PositionArray parents =
        tier_solver.GetCanonicalParentPositions(tier, position, this_tier);
    if (parents.size < 0) {  // OOM.
        TierArrayDestroy(&parents);
        return false;
    }
    Value value = processing_lose ? kWin : kTie;
    Frontier *frontier = processing_lose ? &win_frontier : &tie_frontier;
    for (int64_t i = 0; i < parents.size; ++i) {
#ifdef _OPENMP
        omp_set_lock(&num_undecided_children_lock);
#endif
        uint8_t child_remaining = num_undecided_children[parents.array[i]];
        num_undecided_children[parents.array[i]] = 0;
#ifdef _OPENMP
        omp_unset_lock(&num_undecided_children_lock);
#endif
        if (child_remaining == 0) continue;  // Parent already solved.

        // All parents are win/tie in (remoteness + 1) positions.
        DbSetValueRemoteness(parents.array[i], value, remoteness + 1);
        if (!FrontierAdd(frontier, parents.array[i], remoteness + 1,
                         child_tiers.size)) {  // OOM.
            TierArrayDestroy(&parents);
            return false;
        }
    }
    TierArrayDestroy(&parents);
    return true;
}

static bool ProcessLosePosition(int remoteness, Tier tier, Position position) {
    return ProcessLoseOrTiePosition(remoteness, tier, position, true);
}

static bool ProcessWinPosition(int remoteness, Tier tier, Position position) {
    PositionArray parents =
        tier_solver.GetCanonicalParentPositions(tier, position, this_tier);
    if (parents.size < 0) {  // OOM.
        PositionArrayDestroy(&parents);
        return false;
    }
    for (int64_t i = 0; i < parents.size; ++i) {
#ifdef _OPENMP
        omp_set_lock(&num_undecided_children_lock);
#endif
        if (num_undecided_children[parents.array[i]] == 0) {
            // This parent has been solved already.
#ifdef _OPENMP
            omp_unset_lock(&num_undecided_children_lock);
#endif
            continue;
        }
        // Must perform the above check before decrementing to prevent overflow.
        uint8_t child_remaining = --num_undecided_children[parents.array[i]];
#ifdef _OPENMP
        omp_unset_lock(&num_undecided_children_lock);
#endif

        // If this child position is the last undecided child of parent
        // position, mark parent as lose in (childRmt + 1).
        if (child_remaining == 0) {
            DbSetValueRemoteness(parents.array[i], kLose, remoteness + 1);
            if (!FrontierAdd(&lose_frontier, parents.array[i], remoteness + 1,
                             child_tiers.size)) {  // OOM.
                PositionArrayDestroy(&parents);
                return false;
            }
        }
    }
    PositionArrayDestroy(&parents);
    return true;
}

static bool ProcessTiePosition(int remoteness, Tier tier, Position position) {
    return ProcessLoseOrTiePosition(remoteness, tier, position, false);
}

static void Step5MarkDrawPositions(void) {
    PRAGMA_OMP_PARALLEL_FOR
    for (Position position = 0; position < this_tier_size; ++position) {
        // Skip illegal positions.
        if (num_undecided_children[position] == kIllegalNumChildren) continue;

        if (num_undecided_children[position] > 0) {
            // A position is drawing if it still has undecided children.
            DbSetValueRemoteness(position, kDraw, 0);
            continue;
        }
        assert(num_undecided_children[position] == 0);
    }
    free(num_undecided_children);
    num_undecided_children = NULL;
}

static void Step6SaveValues(void) { DbSave(this_tier); }

static void Step7Cleanup(void) {
    this_tier = -1;
    this_tier_size = -1;
    TierArrayDestroy(&child_tiers);
    DestroyFrontiers();
    free(num_undecided_children);
    num_undecided_children = NULL;
    ReverseGraphDestroy(&reverse_graph);
#ifdef _OPENMP
    omp_destroy_lock(&num_undecided_children_lock);
#endif
}

static void DestroyFrontiers(void) {
    FrontierDestroy(&win_frontier);
    FrontierDestroy(&lose_frontier);
    FrontierDestroy(&tie_frontier);
}

#undef PRAGMA
#undef PRAGMA_OMP_PARALLEL_FOR
#undef PRAGMA_OMP_PARALLEL_FOR_FIRSTPRIVATE
