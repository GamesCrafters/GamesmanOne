/**
 * @file tier_stack.c
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Tier stack implementation.
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

#include "core/types/tier_stack.h"

#include "core/data_structures/int64_array.h"
#include "core/types/base.h"

void TierStackInit(TierStack *stack) { Int64ArrayInit(stack); }

void TierStackDestroy(TierStack *stack) { Int64ArrayDestroy(stack); }

bool TierStackPush(TierStack *stack, Tier tier) {
    return Int64ArrayPushBack(stack, tier);
}

void TierStackPop(TierStack *stack) { Int64ArrayPopBack(stack); }

Tier TierStackTop(const TierStack *stack) { return Int64ArrayBack(stack); }

bool TierStackEmpty(const TierStack *stack) { return Int64ArrayEmpty(stack); }
