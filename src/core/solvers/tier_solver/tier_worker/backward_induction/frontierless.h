/**
 * @file frontierless.h
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Frontierless strategy of the backward induction tier worker solving
 * algorithm. Uses retrograde analysis with frontier queues optimized out at the
 * cost of scanning the transposition table at each remoteness to rediscover the
 * positions that were solved on the previous level.
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

#ifndef GAMESMANONE_CORE_SOLVERS_TIER_SOLVER_TIER_WORKER_BACKWARD_INDUCTION_FRONTIERLESS_H_
#define GAMESMANONE_CORE_SOLVERS_TIER_SOLVER_TIER_WORKER_BACKWARD_INDUCTION_FRONTIERLESS_H_

#include <stddef.h>  // size_t
#include <stdint.h>  // int64_t

#include "core/solvers/tier_solver/tier_worker.h"
#include "core/types/gamesman_types.h"

/**
 * @brief Solves the given \p tier using the frontierless strategy of the
 * backward induction algorithm.
 *
 * @param api Game-specific tier solver API functions.
 * @param db_chunk_size Number of positions in each database compression block.
 * The algorithm then uses this number as the chunk size for OpenMP dynamic
 * scheduling to prevent repeated decompression of the same block.
 * @param tier Tier to solve.
 * @param options Pointer to a \c TierWorkerSolveOptions object which contains
 * the options.
 * @param solved (Output parameter) If non-NULL, its value will be set to
 * \c true if \p tier is actually solved, or \p false if \p tier is loaded from
 * an existing database.
 * @return kNoError on success, or
 * @return non-zero error code otherwise.
 */
int TierWorkerBIFrontierless(const TierSolverApi *api, int64_t db_chunk_size,
                             Tier tier, const TierWorkerSolveOptions *options,
                             bool *solved);

/**
 * @brief Returns the minimum amount of memory in bytes required to solve the
 * given \p tier of \p size positions using the frontierless strategy of the
 * backward induction tier worker solving algorithm.
 *
 * @param tier Tier to solve.
 * @param size Number of positions in the given \p tier .
 * @return Amount of memory in bytes required to use the frontierless strategy.
 */
size_t TierWorkerBIFrontierlessMemReq(Tier tier, int64_t size);

#endif  // GAMESMANONE_CORE_SOLVERS_TIER_SOLVER_TIER_WORKER_BACKWARD_INDUCTION_FRONTIERLESS_H_
