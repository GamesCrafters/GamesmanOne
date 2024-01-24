/**
 * @file position_hash_set.c
 * @author Robert Shi (robertyishi@berkeley.edu)
 *         GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Linear-probing Position hash set implementation.
 *
 * @version 1.0.0
 * @date 2024-01-24
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

#include "core/types/position_hash_set.h"

#include "core/data_structures/int64_hash_map.h"
#include "core/types/base.h"

void PositionHashSetInit(PositionHashSet *set, double max_load_factor) {
    Int64HashMapInit(set, max_load_factor);
}

void PositionHashSetDestroy(PositionHashSet *set) { Int64HashMapDestroy(set); }

bool PositionHashSetContains(PositionHashSet *set, Position position) {
    return Int64HashMapContains(set, position);
}

bool PositionHashSetAdd(PositionHashSet *set, Position position) {
    return Int64HashMapSet(set, position, 0);
}
