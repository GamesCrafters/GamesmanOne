/**
 * @file frontier.c
 * @author Max Delgadillo: designed and implemented the original version
 * of tier solver (solveretrograde.c in GamesmanClassic.)
 * @author Robert Shi (robertyishi@berkeley.edu): Extracted the frontier
 * implementation as a separate module from the tier solver, replaced
 * linked-lists with dynamic arrays for optimization, and implemented
 * thread-safety for the new OpenMP multithreaded tier solver.
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Implementation of the Frontier type.
 * @version 2.1.0
 * @date 2025-03-18
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

#include "core/solvers/tier_solver/tier_worker/backward_induction/frontier.h"

#include <assert.h>   // assert
#include <stdbool.h>  // bool, true, false
#include <stddef.h>   // NULL, size_t
#include <stdint.h>   // int64_t
#include <stdio.h>    // fprintf, stderr
#include <string.h>   // memset

#include "core/concurrency.h"
#include "core/gamesman_memory.h"
#include "core/misc.h"
#include "core/types/gamesman_types.h"

#ifdef _OPENMP
#include <omp.h>
#endif  // _OPENMP

static bool FrontierAllocateBuckets(Frontier *frontier, int size,
                                    GamesmanAllocator *allocator) {
    frontier->f.buckets = (PositionArray *)GamesmanAllocatorAllocate(
        allocator, size * sizeof(PositionArray));
    if (frontier->f.buckets == NULL) {
        fprintf(stderr, "FrontierInit: failed to allocate buckets.\n");
        return false;
    }
    memset(frontier->f.buckets, 0, size * sizeof(PositionArray));

    return true;
}

static bool FrontierAllocateDividers(Frontier *frontier, int frontier_size,
                                     int dividers_size,
                                     GamesmanAllocator *allocator) {
    frontier->f.dividers = (int64_t **)GamesmanAllocatorAllocate(
        allocator, frontier_size * sizeof(int64_t *));
    if (frontier->f.dividers == NULL) {
        fprintf(stderr, "FrontierInit: failed to allocate dividers.\n");
        return false;
    }
    memset(frontier->f.dividers, 0, frontier_size * sizeof(int64_t *));

    size_t dividers_size_bytes = dividers_size * sizeof(int64_t);
    for (int i = 0; i < frontier_size; ++i) {
        frontier->f.dividers[i] = (int64_t *)GamesmanAllocatorAllocate(
            allocator, dividers_size_bytes);
        if (frontier->f.dividers[i] == NULL) {
            fprintf(stderr, "FrontierInit: failed to allocate dividers.\n");
            return false;
        }
        memset(frontier->f.dividers[i], 0, dividers_size_bytes);
    }

    return true;
}

static void FrontierInitAllFields(Frontier *frontier,
                                  GamesmanAllocator *allocator) {
    for (int i = 0; i < frontier->f.size; ++i) {
        PositionArrayInitAllocator(&frontier->f.buckets[i], allocator);
    }
}

bool FrontierInit(Frontier *frontier, int frontier_size, int dividers_size,
                  GamesmanAllocator *allocator) {
    bool success = true;
    memset(frontier, 0, sizeof(*frontier));
    success &= FrontierAllocateBuckets(frontier, frontier_size, allocator);
    success &= FrontierAllocateDividers(frontier, frontier_size, dividers_size,
                                        allocator);

    if (success) {
        // Initialize fields after all memory allocation is successfully
        // completed.
        frontier->f.allocator = GamesmanAllocatorAddRef(allocator);
        frontier->f.size = frontier_size;
        frontier->f.dividers_size = dividers_size;
        FrontierInitAllFields(frontier, allocator);
    } else {
        // Otherwise, pointers are either NULL or pointing to spaces that can be
        // freed.
        GamesmanAllocatorDeallocate(allocator, frontier->f.buckets);
        if (frontier->f.dividers) {
            for (int i = 0; i < frontier_size; ++i) {
                GamesmanAllocatorDeallocate(allocator, frontier->f.dividers[i]);
            }
            GamesmanAllocatorDeallocate(allocator, frontier->f.dividers);
        }

        // Set all member pointers to NULL and all fields to 0.
        memset(frontier, 0, sizeof(*frontier));
    }

    return success;
}

void FrontierDestroy(Frontier *frontier) {
    // Buckets.
    if (frontier->f.buckets) {
        for (int i = 0; i < frontier->f.size; ++i) {
            PositionArrayDestroy(&frontier->f.buckets[i]);
        }
        GamesmanAllocatorDeallocate(frontier->f.allocator, frontier->f.buckets);
    }

    // Dividers.
    if (frontier->f.dividers) {
        for (int i = 0; i < frontier->f.dividers_size; ++i) {
            GamesmanAllocatorDeallocate(frontier->f.allocator,
                                        frontier->f.dividers[i]);
        }
        GamesmanAllocatorDeallocate(frontier->f.allocator,
                                    frontier->f.dividers);
    }

    // Allocator
    GamesmanAllocatorRelease(frontier->f.allocator);

    // Set all member pointers to NULL and all fields to 0.
    memset(frontier, 0, sizeof(*frontier));
}

bool FrontierAdd(Frontier *frontier, Position position, int remoteness,
                 int child_tier_index) {
    // If this fails, there is a bug in tier solver's code.
    assert(remoteness >= 0);

    if (remoteness >= frontier->f.size) {
        fprintf(stderr,
                "FrontierAdd: hard-coded frontier size is not large enough to "
                "hold remoteness %d. Consider changing the value in tier "
                "solver and recompile GAMESMAN.\n",
                remoteness);
        return false;
    }

    // Push position into frontier.
    bool success =
        PositionArrayAppend(&frontier->f.buckets[remoteness], position);
    if (!success) return false;

    // Update divider.
    ++frontier->f.dividers[remoteness][child_tier_index];
    return true;
}

void FrontierAccumulateDividers(Frontier *frontier) {
    PRAGMA_OMP(parallel for)
    for (int remoteness = 0; remoteness < frontier->f.size; ++remoteness) {
        // This for-loop must be executed sequentially.
        for (int i = 1; i < frontier->f.dividers_size; ++i) {
            frontier->f.dividers[remoteness][i] +=
                frontier->f.dividers[remoteness][i - 1];
        }
    }
}

Position FrontierGetPosition(const Frontier *frontier, int remoteness,
                             int64_t i) {
    return frontier->f.buckets[remoteness].array[i];
}

void FrontierFreeRemoteness(Frontier *frontier, int remoteness) {
    PositionArrayDestroy(&frontier->f.buckets[remoteness]);
    GamesmanAllocatorDeallocate(frontier->f.allocator,
                                frontier->f.dividers[remoteness]);
    frontier->f.dividers[remoteness] = NULL;
}
