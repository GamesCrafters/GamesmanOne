/**
 * @file tier_stack.h
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Tier stack.
 * @version 1.0.1
 * @date 2024-12-10
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

#ifndef GAMESMANONE_CORE_TYPES_TIER_STACK_H_
#define GAMESMANONE_CORE_TYPES_TIER_STACK_H_

#include "core/data_structures/int64_array.h"
#include "core/types/base.h"

/** @brief Dynamic Tier stack using Int64Array. */
typedef Int64Array TierStack;

/** @brief Initializes Tier stack STACK. */
void TierStackInit(TierStack *stack);

/** @brief Destroys Tier stack STACK. */
void TierStackDestroy(TierStack *stack);

/**
 * @brief Pushes a new TIER into the STACK.
 *
 * @param stack Destination stack.
 * @param tier New tier.
 * @return true on success,
 * @return false otherwise.
 */
bool TierStackPush(TierStack *stack, Tier tier);

/**
 * @brief Pops a tier from the STACK. Calling this function on an empty STACK
 * results in undefined behavior.
 *
 * @param stack Stack to pop the tier from.
 */
void TierStackPop(TierStack *stack);

/**
 * @brief Returns the item at the top of the STACK. Calling this function on an
 * empty STACK results in undefined behavior.
 *
 * @param stack Stack to peak into.
 * @return Tier at the top of STACK.
 */
Tier TierStackTop(const TierStack *stack);

/** @brief Returns true if the given STACK is empty, or false otherwise. */
bool TierStackEmpty(const TierStack *stack);

#endif  // GAMESMANONE_CORE_TYPES_TIER_STACK_H_
