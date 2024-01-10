/**
 * @file naivedb.h
 * @author Robert Shi (robertyishi@berkeley.edu)
 *         GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Naive database which stores Values and Remotenesses in uncompressed
 * raw bytes.
 * @version 1.0.0
 * @date 2023-08-19
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

#ifndef GAMESMANONE_CORE_DB_NAIVEDB_NAIVEDB_H_
#define GAMESMANONE_CORE_DB_NAIVEDB_NAIVEDB_H_

#include "core/types/gamesman_types.h"

/**
 * @brief Naive database which stores Values and Remotenesses in uncompressed
 * raw bytes.
 */
extern const Database kNaiveDb;

#endif  // GAMESMANONE_CORE_DB_NAIVEDB_NAIVEDB_H_
