/**
 * @file game_manager.c
 * @author Robert Shi (robertyishi@berkeley.edu)
 *         GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Implementation of the Game Manager Module.
 *
 * @version 1.0
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

#include "games/game_manager.h"

#include <stddef.h>  // NULL

// 1. To add a new game, include the game header here.

#include "games/mttt/mttt.h"
#include "games/mtttier/mtttier.h"

// 2. Then add the new game object to the list. Note that the list must be
// NULL-terminated.

static const Game *const kAllGames[] = {&kMtttier, &kMttt, NULL};

const Game *const *GameManagerGetAllGames(void) { return kAllGames; }

int GameManagerNumGames(void) {
    static int count = -1;
    if (count >= 0) return count;
    int i = 0;
    while (kAllGames[i] != NULL) {
        ++i;
    }
    return count = i;
}
