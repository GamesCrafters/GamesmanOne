/**
 * @file reverse_tier_graph.c
 * @author Max Delgadillo: designed and implemented the original version
 * of tier solver (solveretrograde.c in GamesmanClassic.)
 * @author Robert Shi (robertyishi@berkeley.edu): Extracted the reverse tier
 * graph implementation as a separate module from the tier solver
 *         GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Implementation of the ReverseTierGraph type.
 *
 * @version 1.0.0
 * @date 2024-01-19
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

#include "core/solvers/tier_solver/reverse_tier_graph.h"

#include <assert.h>  // assert
#include <stddef.h>  // NULL
#include <stdint.h>  // int64_t
#include <stdlib.h>  // realloc, free
#include <string.h>  // memset

static int AddNewTier(ReverseTierGraph *graph, Tier child);
static int ReverseTierGraphExpand(ReverseTierGraph *graph);

// -----------------------------------------------------------------------------

void ReverseTierGraphInit(ReverseTierGraph *graph) {
    graph->parents_of = NULL;
    TierHashMapInit(&graph->index_of, 0.5);
    graph->size = 0;
    graph->capacity = 0;
}

void ReverseTierGraphDestroy(ReverseTierGraph *graph) {
    for (int64_t i = 0; i < graph->size; ++i) {
        TierArrayDestroy(&graph->parents_of[i]);
    }
    free(graph->parents_of);
    TierHashMapDestroy(&graph->index_of);
    memset(graph, 0, sizeof(*graph));
}

int ReverseTierGraphAdd(ReverseTierGraph *graph, Tier child, Tier parent) {
    if (!TierHashMapContains(&graph->index_of, child)) {
        int error = AddNewTier(graph, child);
        if (error != kNoError) return error;
    }

    TierHashMapIterator it = TierHashMapGet(&graph->index_of, child);
    int64_t index = TierHashMapIteratorValue(&it);
    bool success = TierArrayAppend(&graph->parents_of[index], parent);
    if (success) return kNoError;
    return kMallocFailureError;
}

TierArray ReverseTierGraphPopParentsOf(ReverseTierGraph *graph, Tier child) {
    static const TierArray kIllegalTierArray = {
        .array = NULL,
        .size = -1,
        .capacity = -1,
    };
    TierHashMapIterator it = TierHashMapGet(&graph->index_of, child);
    if (!TierHashMapIteratorIsValid(&it)) return kIllegalTierArray;

    int index = TierHashMapIteratorValue(&it);
    TierArray ret = graph->parents_of[index];
    memset(&graph->parents_of[index], 0, sizeof(graph->parents_of[index]));
    return ret;
}

// -----------------------------------------------------------------------------

static int AddNewTier(ReverseTierGraph *graph, Tier child) {
    if (graph->size == graph->capacity) {
        int error = ReverseTierGraphExpand(graph);
        if (error != kNoError) return error;
    }

    assert(graph->capacity > graph->size);
    TierHashMapSet(&graph->index_of, child, graph->size);
    TierArrayInit(&graph->parents_of[graph->size]);
    ++graph->size;
    return kNoError;
}

static int ReverseTierGraphExpand(ReverseTierGraph *graph) {
    int64_t new_capacity = graph->capacity == 0 ? 1 : graph->capacity * 2;
    TierArray *new_parents_of = (TierArray *)realloc(
        graph->parents_of, new_capacity * sizeof(TierArray));
    if (new_parents_of == NULL) return kMallocFailureError;

    graph->parents_of = new_parents_of;
    graph->capacity = new_capacity;

    return kNoError;
}
