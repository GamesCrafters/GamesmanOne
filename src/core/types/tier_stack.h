/**
 * @file tier_stack.h
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Tier stack.
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

#include <stdbool.h>

#include "core/data_structures/int64_array.h"
#include "core/types/base.h"

/** @brief Dynamic Tier stack using Int64Array. */
typedef Int64Array TierStack;

/** @brief Initializes Tier stack STACK. */
static inline void TierStackInit(TierStack *stack) { Int64ArrayInit(stack); }

/** @brief Destroys Tier stack STACK. */
static inline void TierStackDestroy(TierStack *stack) {
    Int64ArrayDestroy(stack);
}

/**
 * @brief Pushes a new TIER into the STACK.
 *
 * @param stack Destination stack.
 * @param tier New tier.
 * @return true on success,
 * @return false otherwise.
 */
static inline bool TierStackPush(TierStack *stack, Tier tier) {
    return Int64ArrayPushBack(stack, tier);
}

/**
 * @brief Pops a tier from the STACK. Calling this function on an empty STACK
 * results in undefined behavior.
 *
 * @param stack Stack to pop the tier from.
 */
static inline void TierStackPop(TierStack *stack) { Int64ArrayPopBack(stack); }

/**
 * @brief Returns the item at the top of the STACK. Calling this function on an
 * empty STACK results in undefined behavior.
 *
 * @param stack Stack to peak into.
 * @return Tier at the top of STACK.
 */
static inline Tier TierStackTop(const TierStack *stack) {
    return Int64ArrayBack(stack);
}

/** @brief Returns true if the given STACK is empty, or false otherwise. */
static inline bool TierStackEmpty(const TierStack *stack) {
    return Int64ArrayEmpty(stack);
}

#endif  // GAMESMANONE_CORE_TYPES_TIER_STACK_H_
