/**
 * @file constants.c
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Definitions of global constants.
 * @version 2.0.0
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

#include "core/constants.h"

#include "core/types/gamesman_types.h"

ConstantReadOnlyString kValueStrings[] = {
    "undecided", "lose", "draw", "tie", "win",
};

const Tier kDefaultTier = 0;
const Tier kIllegalTier = -1;
const Position kIllegalPosition = -1;
const TierPosition kIllegalTierPosition = {
    .tier = kIllegalTier,
    .position = kIllegalPosition,
};
