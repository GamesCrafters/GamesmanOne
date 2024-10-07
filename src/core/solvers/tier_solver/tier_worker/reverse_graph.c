/**
 * @file reverse_graph.c
 * @author Max Delgadillo: designed and implemented the original version
 * of tier solver (solveretrograde.c in GamesmanClassic.)
 * @author Robert Shi (robertyishi@berkeley.edu): Extracted the reverse graph
 * implementation as a separate module from the tier solver, replaced
 * linked-lists with dynamic arrays for optimization, and implemented
 * thread-safety for the new OpenMP multithreaded tier solver.
 *         GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Implementation of the ReverseGraph type.
 * @version 1.1.0
 * @date 2023-10-20
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

#include "core/solvers/tier_solver/reverse_graph.h"

#include <assert.h>   // assert
#include <stdbool.h>  // bool, true, false
#include <stddef.h>   // NULL
#include <stdint.h>   // int64_t
#include <stdlib.h>   // malloc, free
#include <string.h>   // memset

#include "core/types/gamesman_types.h"  // PositionArray, TierArray, TierPosition

#ifdef _OPENMP
#include <omp.h>
#endif  // _OPENMP

static bool InitOffsetMap(ReverseGraph *graph, const TierArray *child_tiers,
                          Tier this_tier, int64_t (*GetTierSize)(Tier tier)) {
    TierHashMapInit(&graph->offset_map, 0.5);
    graph->size = 0;

    // Map all child tiers.
    for (int64_t i = 0; i < child_tiers->size; ++i) {
        bool success = TierHashMapSet(&graph->offset_map, child_tiers->array[i],
                                      graph->size);
        if (!success) {
            TierHashMapDestroy(&graph->offset_map);
            return false;
        }
        graph->size += GetTierSize(child_tiers->array[i]);
    }

    // Map currently solving tier.
    bool success = TierHashMapSet(&graph->offset_map, this_tier, graph->size);
    if (!success) {
        TierHashMapDestroy(&graph->offset_map);
        return false;
    }
    graph->size += GetTierSize(this_tier);

    return true;
}

static bool InitParentPositionArrays(ReverseGraph *graph) {
    // Assumes graph->size has been set.
    graph->parents_of =
        (PositionArray *)malloc(graph->size * sizeof(PositionArray));
    if (graph->parents_of == NULL) return false;
    for (int64_t i = 0; i < graph->size; ++i) {
        PositionArrayInit(&graph->parents_of[i]);
    }
    return true;
}

#ifdef _OPENMP
static bool InitLocks(ReverseGraph *graph) {
    graph->locks = (omp_lock_t *)malloc(graph->size * sizeof(omp_lock_t));
    if (graph->locks == NULL) return false;
    for (int64_t i = 0; i < graph->size; ++i) {
        omp_init_lock(&graph->locks[i]);
    }
    return true;
}
#endif  // _OPENMP

// Assumes GetTierSize() has been set up correctly.
bool ReverseGraphInit(ReverseGraph *graph, const TierArray *child_tiers,
                      Tier this_tier, int64_t (*GetTierSize)(Tier tier)) {
    if (!InitOffsetMap(graph, child_tiers, this_tier, GetTierSize)) {
        return false;
    }
    if (!InitParentPositionArrays(graph)) {
        TierHashMapDestroy(&graph->offset_map);
        return false;
    }
#ifdef _OPENMP
    if (!InitLocks(graph)) {
        free(graph->parents_of);
        graph->parents_of = NULL;
        TierHashMapDestroy(&graph->offset_map);
        return false;
    }
#endif  // _OPENMP
    return true;
}

void ReverseGraphDestroy(ReverseGraph *graph) {
    if (graph->parents_of) {
        for (int64_t i = 0; i < graph->size; ++i) {
            PositionArrayDestroy(&graph->parents_of[i]);
#ifdef _OPENMP
            omp_destroy_lock(&graph->locks[i]);
#endif  // _OPENMP
        }
        free(graph->parents_of);
#ifdef _OPENMP
        free(graph->locks);
#endif  // _OPENMP
    }
    graph->parents_of = NULL;
#ifdef _OPENMP
    graph->locks = NULL;
#endif  // _OPENMP
    graph->size = 0;
    TierHashMapDestroy(&graph->offset_map);
}

/**
 * @brief Returns the index into the parents_of array of GRAPH corresponding to
 * TIER_POSITION.
 *
 * @note Assumes that GRAPH is initialized. Results in undefined behavior
 * otherwise.
 *
 * @param graph Reverse graph.
 * @param tier_position Position to get index of.
 * @return Non-negative index into GRAPH->parents_of to get to retrieve parent
 * positions of TIER_POSITION on success. Negative error code on failure.
 */
static int64_t ReverseGraphGetIndex(ReverseGraph *graph,
                                    TierPosition tier_position) {
    TierHashMapIterator it =
        TierHashMapGet(&graph->offset_map, tier_position.tier);
    assert(TierHashMapIteratorIsValid(&it));
    int64_t offset = TierHashMapIteratorValue(&it);
    return offset + tier_position.position;
}

PositionArray ReverseGraphPopParentsOf(ReverseGraph *graph,
                                       TierPosition tier_position) {
    int64_t index = ReverseGraphGetIndex(graph, tier_position);
    PositionArray ret = graph->parents_of[index];

    // Clears the entry in reverse graph since it is no longer needed.
    // The caller of this function is responsible for freeing the array.
    memset(&graph->parents_of[index], 0, sizeof(graph->parents_of[index]));
    return ret;
}

bool ReverseGraphAdd(ReverseGraph *graph, TierPosition child, Position parent) {
    assert(graph->size > child.position);
    int64_t index = ReverseGraphGetIndex(graph, child);
#ifdef _OPENMP
    omp_set_lock(&graph->locks[index]);
#endif  // _OPENMP
    bool success = PositionArrayAppend(&graph->parents_of[index], parent);
#ifdef _OPENMP
    omp_unset_lock(&graph->locks[index]);
#endif  // _OPENMP
    return success;
}
