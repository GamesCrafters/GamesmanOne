/**
 * @file frontier.h
 * @author Max Delgadillo: designed and implemented the original version
 * of tier solver (solveretrograde.c in GamesmanClassic.)
 * @author Robert Shi (robertyishi@berkeley.edu): Extracted the frontier
 * implementation as a separate module from the tier solver, replaced
 * linked-lists with dynamic arrays for optimization, and implemented
 * thread-safety for the new OpenMP multithreaded tier solver.
 *         GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Declaration of the Frontier type.
 *
 * @version 1.0
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

#ifndef GAMESMANONE_CORE_SOLVERS_TIER_SOLVER_FRONTIER_H_
#define GAMESMANONE_CORE_SOLVERS_TIER_SOLVER_FRONTIER_H_

#include <stdbool.h>  // bool
#include <stdint.h>   // int64_t

#include "core/gamesman_types.h"  // PositionArray

#ifdef _OPENMP
#include <omp.h>  // omp_lock_t
#endif

/**
 * @brief A Frontier is a dynamic 2D Position array which stores solved
 * positions that have not been used to deduce the values of their parents.
 *
 * @details A Frontier object contains an array of PositionArray objects,
 * where the i-th PositionArray stores solved but unprocessed Positions
 * with remoteness i.
 */
typedef struct Frontier {
    /**
     * 2-dimensional Position array. The first dimension is fixed and set to
     * the frontier_size passed to the FrontierInit() function. This is usually
     * set to the maximum remoteness supported by GAMESMAN plus one. The second
     * dimension can be dynamically expanded if needed, and the expansion
     * process is handled by the PositionArray type.
     */
    PositionArray *buckets;

    /**
     * A 2-dimensional integer array storing the "divider" values. Both
     * dimensions are fixed and set to the frontier_size and dividers_size
     * passed to the FrontierInit() function respectively. The frontier_size
     * is usually set to the maximum remoteness supported by GAMESMAN plus one.
     * The dividers_size should be set to the number of child tiers of the
     * current solving tier.
     *
     * Before FrontierAccumulateDividers() is called on the Frontier
     * object, dividers[i] stores the NUMBERS of positions of remoteness i
     * loaded from each child tier. After FrontierAccumulateDividers() is
     * called, dividers[i] stores the OFFSETS to the first positions loaded
     * from each child tier.
     *
     * Note that for dividers to work, we must assume that child tiers are
     * processed sequentially so that positions loaded from each child tier are
     * in consecutive chunks.
     *
     * The dividers are used by the tier solver to figure out the tier from
     * which the unprocess position was loaded from. Otherwise, we would have
     * to store TierPosition arrays instead, which would cost more memory.
     */
    int64_t **dividers;

#ifdef _OPENMP
    /** An array of locks for each bucket (PositionArray). */
    omp_lock_t *locks;
#endif
    int size;
    int dividers_size;
} Frontier;

/**
 * @brief Initializes FRONTIER.
 *
 * @param frontier Frontier object to initialize.
 * @param frontier_size Number of frontier arrays to allocate. This is usually
 * set to the maximum remoteness supported by GAMESMAN plus one.
 * @param dividers_size Number of dividers to allocate. This should be set to
 * the number of child tiers of the current solving tier.
 * @return true on success,
 * @return false otherwise.
 */
bool FrontierInit(Frontier *frontier, int frontier_size, int dividers_size);

/** @brief Destroys FRONTIER, freeing all allocated memory. */
void FrontierDestroy(Frontier *frontier);

/**
 * @brief Adds POSITION loaded from child tier of index CHILD_TIER_INDEX of
 * remoteness REMOTENESS to FRONTIER.
 *
 * @param frontier Frontier to add the position to.
 * @param position Position to add.
 * @param remoteness Remoteness of the position.
 * @param child_tier_index Index of the child tier from which the position was
 * loaded. The largest index indicates that the position was not loaded from
 * a child tier but solved from the current tier instead.
 * @return true on success,
 * @return false otherwise.
 */
bool FrontierAdd(Frontier *frontier, Position position, int remoteness,
                 int child_tier_index);

/**
 * @brief Accumulates the divider values of FRONTIER so that they become offsets
 * instead of sizes.
 *
 * @note This function is designed to be called only once. Calling this function
 * multiple times renders the divider values unusable.
 */
void FrontierAccumulateDividers(Frontier *frontier);

/**
 * @brief Deallocates the bucket and divider array for remoteness REMOTENESS in
 * FRONTIER.
 */
void FrontierFreeRemoteness(Frontier *frontier, int remoteness);

#endif  // GAMESMANONE_CORE_SOLVERS_TIER_SOLVER_FRONTIER_H_
