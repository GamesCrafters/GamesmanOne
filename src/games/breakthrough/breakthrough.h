/**
 * @file breakthrough.h
 * @author Kelvin Ortiz: ported from the GamesmanPy version (breakthrough.py).
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Breakthrough, solved with the tier solver using piece-count tiers.
 * @details https://en.wikipedia.org/wiki/Breakthrough_(board_game)
 * @version 1.0.0
 * @date 2026-10-05
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

#ifndef GAMESMANONE_GAMES_BREAKTHROUGH_BREAKTHROUGH_H_
#define GAMESMANONE_GAMES_BREAKTHROUGH_BREAKTHROUGH_H_

#include "core/types/game/game.h"

/** @brief The Breakthrough game object, registered in games/game_list.c. */
extern const Game kBreakthrough;

#endif  // GAMESMANONE_GAMES_BREAKTHROUGH_BREAKTHROUGH_H_
