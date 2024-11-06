/**
 * @file game_list.c
 * @author Robert Shi (robertyishi@berkeley.edu)
 *         GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Definition for the list of all games.
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

#include "games/game_list.h"

#include <stddef.h>  // NULL

#include "core/types/gamesman_types.h"

// 1. To add a new game, include the game header here.

#include "games/dshogi/dshogi.h"
#include "games/fsvp/fsvp.h"
#include "games/gates/gates.h"
#include "games/kaooa/kaooa.h"
#include "games/mallqueenschess/mallqueenschess.h"
#include "games/mttt/mttt.h"
#include "games/mtttier/mtttier.h"
#include "games/neutron/neutron.h"
#include "games/quixo/quixo.h"
#include "games/teeko/teeko.h"

// 2. Then add the new game object to the list. Note that the list must be
// NULL-terminated.

const Game *const kAllGames[] = {
    &kKaooa, &kNeutron, &kGates, &kTeeko,           &kDobutsuShogi, &kQuixo,
    &kFsvp,  &kMtttier, &kMttt,  &kMallqueenschess, NULL,
};
