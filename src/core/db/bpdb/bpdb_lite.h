/**
 * @file bpdb_lite.h
 * @author Dan Garcia: designed the "lookup table" compression algorithm
 * @author Max Fierro: improved the algorithm for BpArray compression
 * @author Sameer Nayyar: improved the algorithm for BpArray compression
 * @author Robert Shi (robertyishi@berkeley.edu): improved and implemented
 *         compression algorithms and bpdb.
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Bit-Perfect Database Lite.
 * @version 1.2.2
 * @date 2024-12-22
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

#ifndef GAMESMANONE_CORE_DB_BPDB_BPDB_LITE_H_
#define GAMESMANONE_CORE_DB_BPDB_BPDB_LITE_H_

#include "core/types/gamesman_types.h"

/**
 * @brief BPDB "Lite" version which only supports position values and
 * remotenesses.
 */
extern const Database kBpdbLite;

#endif  // GAMESMANONE_CORE_DB_BPDB_BPDB_LITE_H_
