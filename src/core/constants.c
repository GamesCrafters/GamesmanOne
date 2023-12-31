/**
 * @file constants.c
 * @author Robert Shi (robertyishi@berkeley.edu)
 *         GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Definitions of global constants.
 * @version 1.0
 * @date 2023-10-21
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

#include "core/gamesman_types.h"

const Tier kIllegalTier = -1;
const Position kIllegalPosition = -1;
const TierPosition kIllegalTierPosition = {
    .tier = kIllegalTier,
    .position = kIllegalPosition,
};
const int kIllegalRemoteness = -1;

ConstantReadOnlyString kDate = "2023.10.21";
ConstantReadOnlyString kVersion = "0.41";
