/**
 * @file frontier.C
 * @author Max Delgadillo: designed and implemented the original version
 * of tier solver (solveretrograde.c in GamesmanClassic.)
 * @author Robert Shi (robertyishi@berkeley.edu): Extracted the frontier
 * implementation as a separate module from the tier solver, replaced
 * linked-lists with dynamic arrays for optimization, and implemented
 * thread-safety for the new OpenMP multithreaded tier solver.
 *         GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Implementation of the Frontier type.
 *
 * @version 1.0.0
 * @date 2023-08-19
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

#include "core/solvers/tier_solver/frontier.h"

#include <assert.h>   // assert
#include <stdbool.h>  // bool, true, false
#include <stddef.h>   // NULL
#include <stdint.h>   // int64_t
#include <stdio.h>    // fprintf, stderr
#include <stdlib.h>   // calloc, free
#include <string.h>   // memset

#include "core/types/gamesman_types.h"  // PositionArray

#ifdef _OPENMP
#include <omp.h>
#define PRAGMA(X) _Pragma(#X)
#define PRAGMA_OMP_ATOMIC PRAGMA(omp atomic)
#define PRAGMA_OMP_PARALLEL_FOR PRAGMA(omp parallel for)
#else
#define PRAGMA
#define PRAGMA_OMP_ATOMIC
#define PRAGMA_OMP_PARALLEL_FOR
#endif

static bool FrontierAllocateBuckets(Frontier *frontier, int size) {
    frontier->buckets = (PositionArray *)calloc(size, sizeof(PositionArray));
    if (frontier->buckets == NULL) {
        fprintf(stderr, "FrontierInit: failed to calloc buckets.\n");
        return false;
    }
    return true;
}

static bool FrontierAllocateDividers(Frontier *frontier, int frontier_size,
                                     int dividers_size) {
    bool success = true;
    frontier->dividers = (int64_t **)calloc(frontier_size, sizeof(int64_t *));
    success = (frontier->dividers != NULL);
    if (!success) goto _bailout;

    for (int64_t i = 0; i < frontier_size; ++i) {
        frontier->dividers[i] =
            (int64_t *)calloc(dividers_size, sizeof(int64_t));
        success = (frontier->dividers[i] != NULL);
        if (!success) goto _bailout;
    }

_bailout:
    if (!success) fprintf(stderr, "FrontierInit: failed to calloc dividers.\n");
    return success;
}

#ifdef _OPENMP
static bool FrontierAllocateLocks(Frontier *frontier, int size) {
    frontier->locks = (omp_lock_t *)calloc(size, sizeof(omp_lock_t));
    if (frontier->locks == NULL) {
        fprintf(stderr, "FrontierInit: failed to calloc locks.\n");
        return false;
    }
    return true;
}
#endif

static void FrontierInitAllFields(Frontier *frontier) {
    for (int i = 0; i < frontier->size; ++i) {
        PositionArrayInit(&frontier->buckets[i]);
#ifdef _OPENMP
        omp_init_lock(&frontier->locks[i]);
#endif
    }
}

bool FrontierInit(Frontier *frontier, int frontier_size, int dividers_size) {
    bool success = true;
    memset(frontier, 0, sizeof(*frontier));
    success &= FrontierAllocateBuckets(frontier, frontier_size);
    success &= FrontierAllocateDividers(frontier, frontier_size, dividers_size);
#ifdef _OPENMP
    success &= FrontierAllocateLocks(frontier, frontier_size);
#endif

    if (success) {
        // Initialize fields after all memory
        // allocation is successfully completed.
        frontier->size = frontier_size;
        frontier->dividers_size = dividers_size;
        FrontierInitAllFields(frontier);
    } else {
        // Otherwise, pointers are either NULL or pointing
        // to spaces that can be free()-d.
        free(frontier->buckets);
        if (frontier->dividers) {
            for (int i = 0; i < dividers_size; ++i) {
                free(frontier->dividers[i]);
            }
            free(frontier->dividers);
        }
#ifdef _OPENMP
        free(frontier->locks);
#endif
        // Set all member pointers to NULL and all fields to 0.
        memset(frontier, 0, sizeof(*frontier));
    }
    return success;
}

void FrontierDestroy(Frontier *frontier) {
    // Buckets.
    if (frontier->buckets) {
        for (int i = 0; i < frontier->size; ++i) {
            PositionArrayDestroy(&frontier->buckets[i]);
        }
        free(frontier->buckets);
    }

    // Dividers.
    if (frontier->dividers) {
        for (int i = 0; i < frontier->dividers_size; ++i) {
            free(frontier->dividers[i]);
        }
        free(frontier->dividers);
    }

#ifdef _OPENMP
    // Locks.
    if (frontier->locks) {
        for (int i = 0; i < frontier->size; ++i) {
            omp_destroy_lock(&frontier->locks[i]);
        }
        free(frontier->locks);
    }
#endif

    // Set all member pointers to NULL and all fields to 0.
    memset(frontier, 0, sizeof(*frontier));
}

bool FrontierAdd(Frontier *frontier, Position position, int remoteness,
                 int child_tier_index) {
    // If this fails, there is a bug in tier solver's code.
    assert(remoteness >= 0);

    if (remoteness >= frontier->size) {
        fprintf(stderr,
                "FrontierAdd: hard-coded frontier size is not large enough to "
                "hold remoteness %d. Consider changing the value in tier "
                "solver and recompile Gamesman.\n",
                remoteness);
        return false;
    }

    // Push position into frontier.
#ifdef _OPENMP
    omp_set_lock(&frontier->locks[remoteness]);
#endif
    bool success =
        PositionArrayAppend(&frontier->buckets[remoteness], position);
#ifdef _OPENMP
    omp_unset_lock(&frontier->locks[remoteness]);
#endif
    if (!success) return false;

    // Update divider.
    PRAGMA_OMP_ATOMIC
    ++frontier->dividers[remoteness][child_tier_index];
    return true;
}

void FrontierAccumulateDividers(Frontier *frontier) {
    PRAGMA_OMP_PARALLEL_FOR
    for (int remoteness = 0; remoteness < frontier->size; ++remoteness) {
        // This for-loop must be executed sequentially.
        for (int i = 1; i < frontier->dividers_size; ++i) {
            frontier->dividers[remoteness][i] +=
                frontier->dividers[remoteness][i - 1];
        }
    }
}

void FrontierFreeRemoteness(Frontier *frontier, int remoteness) {
    PositionArrayDestroy(&frontier->buckets[remoteness]);
    free(frontier->dividers[remoteness]);
    frontier->dividers[remoteness] = NULL;
}

#undef PRAGMA
#undef PRAGMA_OMP_ATOMIC
#undef PRAGMA_OMP_PARALLEL_FOR
