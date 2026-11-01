/**
 * @file one_bit.h
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief External memory retrograde analysis algorithm using only one bit per
 * position in the group of tiers made up of the tier currently being
 * solved and its child tiers.
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

#ifndef GAMESMANONE_CORE_SOLVERS_TIER_SOLVER_TIER_WORKER_BACKWARD_INDUCTION_ONE_BIT_H_
#define GAMESMANONE_CORE_SOLVERS_TIER_SOLVER_TIER_WORKER_BACKWARD_INDUCTION_ONE_BIT_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "core/solvers/tier_solver/tier_solver.h"
#include "core/types/base.h"

/**
 * @brief Returns the amount of memory required in bytes to solve a tier group
 * of \p tier_group_size positions.
 *
 * @param tier_group_size Number of positions in the tier group, which includes
 * the positions in the solving tier and its child tiers.
 * @return Amount of memory required in bytes.
 */
size_t OneBitMemReq(int64_t tier_group_size);

/**
 * @brief Solves the given \p tier using the one-bit strategy of the backward
 * induction algorithm.
 *
 * @param api Game-specific tier solver API functions.
 * @param db_chunk_size Number of positions in each database compression block.
 * The algorithm then uses this number as the chunk size for OpenMP dynamic
 * scheduling to prevent repeated decompression of the same block.
 * @param tier Tier to solve.
 * @param options Non-null pointer to a \c TierSolverSolveOptions object which
 * contains the options.
 * @param solved (Output parameter) If non-NULL, its value will be set to
 * \c true on success. Otherwise it remains unmodified.
 * @return \c kSuccess on success, or
 * @return non-zero error code otherwise.
 */
int TierWorkerBIOneBit(const TierSolverApi *api, int64_t db_chunk_size,
                       Tier tier, const TierSolverSolveOptions *options,
                       bool *solved);

#endif  // GAMESMANONE_CORE_SOLVERS_TIER_SOLVER_TIER_WORKER_BACKWARD_INDUCTION_ONE_BIT_H_
