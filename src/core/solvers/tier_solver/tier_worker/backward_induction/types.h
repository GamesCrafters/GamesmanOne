/**
 * @file types.h
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Helper types for tier worker's backward induction solving algorithms.
 * @version 1.0.0
 * @date 2025-06-23
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

#ifndef GAMESMANONE_CORE_SOLVERS_TIER_SOLVER_TIER_WORKER_BACKWARD_INDUCTION_TYPES_H_
#define GAMESMANONE_CORE_SOLVERS_TIER_SOLVER_TIER_WORKER_BACKWARD_INDUCTION_TYPES_H_

#include <stddef.h>  // size_t
#include <stdint.h>  // int64_t

#include "core/db/db_manager.h"
#include "core/types/gamesman_types.h"

/**
 * @brief Integer type for storing the number of undecided child positions.
 *
 * @note The current definition assumes the number of children of any position
 * is no more than 32767.
 *
 * @todo Check if this can be optimized out.
 */
typedef int16_t ChildPosCounterType;
#ifdef _OPENMP
typedef _Atomic ChildPosCounterType AtomicChildPosCounterType;
#endif  // _OPENMP

/**
 * @brief Backward induction loopy solve strategies.
 */
typedef enum {
    /**
     * @brief Classic retrograde analysis which stores the whole transposition
     * table in memory and explicitly stores the newly solved positions in a
     * frontier queue. Memory usage depends on the shape of the position graph.
     */
    kFrontierPercolation,

    /**
     * @brief Retrograde analysis with frontier queues optimized out at the cost
     * of scanning the transposition table at each remoteness to rediscover the
     * positions that were solved on the previous level.
     */
    kFrontierless,

    /**
     * @brief External memory retrograde analysis algorithm using only one bit
     * per each position in the group of tiers made up of the tier currently
     * being solved and its child tiers. Algorithm devised by Ren Wu and Don
     * Beal, "Fast, Memory-Efficient Retrograde Algorithms."
     */
    kOneBit,

    /**
     * @brief Error indicator returned when the given amount of memory is not
     * enough to solve the given tier even with the most memory-efficient
     * algorithm.
     */
    kUnsolvable,
} BackwardInductionStrategy;

#endif  // GAMESMANONE_CORE_SOLVERS_TIER_SOLVER_TIER_WORKER_BACKWARD_INDUCTION_TYPES_H_
