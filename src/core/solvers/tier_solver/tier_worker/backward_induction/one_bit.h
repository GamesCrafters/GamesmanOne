/**
 * @file one_bit.h
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief External memory retrograde analysis algorithm using only one bit per
 * each position in the group of tiers made up of the tier currently being
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

#include <stddef.h>  // size_t
#include <stdint.h>  // int64_t

#include "core/data_structures/concurrent_bitset.h"

static inline size_t OneBitMemReq(int64_t tier_group_size) {
#ifdef _OPENMP
    return ConcurrentBitsetMemRequired(tier_group_size);
#else   // _OPENMP not defined
    return (size_t)tier_group_size / 8;  // one bit per position
#endif  // _OPENMP
}

#endif  // GAMESMANONE_CORE_SOLVERS_TIER_SOLVER_TIER_WORKER_BACKWARD_INDUCTION_ONE_BIT_H_
