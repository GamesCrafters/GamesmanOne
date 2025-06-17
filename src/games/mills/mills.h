/**
 * @file mills.h
 * @author Patricia Fong, Kevin Liu, Erwin A. Vedar, Wei Tu, Elmer Lee,
 * Cameron Cheung: developed the first version in GamesmanClassic (m369mm.c).
 * @author Cameron Cheung (cameroncheung@berkeley.edu): prototype version
 * @author Robert Shi (robertyishi@berkeley.edu): x86 SIMD hash version
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Most variants of the Mills Games (Morris Family of Games).
 * @version 1.0.0
 * @date 2025-04-27
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

#ifndef GAMESMANONE_GAMES_MILLS_MILLS_H_
#define GAMESMANONE_GAMES_MILLS_MILLS_H_

#include "core/types/gamesman_types.h"

/**
 * @brief Most variants of the Mills Games. Provided options include piece and
 * board configurations, flying rules, the Lasker variant rule (which merges the
 * placement and the moving phases), and piece removal rules regarding pieces
 * that are already in a mill. All of the following popular variants can be
 * configured using the options provided:
 *
 *     - Five Men's Morris
 *
 *     - Six Men's Morris
 *
 *     - Seven Men's Morris
 *
 *     - Nine Men's Morris
 *
 *     - Lasker Morris
 *
 *     - Eleven Men's Morris
 *
 *     - Twelve Men's Morris (Morabaraba)
 *
 *     - Sesotho Morabaraba
 */
extern const Game kMills;

#endif  // GAMESMANONE_GAMES_MILLS_MILLS_H_
