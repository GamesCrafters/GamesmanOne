/**
 * @file gamesman_types.h
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Declarations of GAMESMAN types and global macros.
 * @version 1.4.0
 * @date 2025-03-17
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

#ifndef GAMESMANONE_CORE_GAMESMAN_TYPES_H_
#define GAMESMANONE_CORE_GAMESMAN_TYPES_H_

#include "core/types/base.h"
#include "core/types/database/database.h"
#include "core/types/game/game.h"
#include "core/types/gameplay_api/gameplay_api.h"
#include "core/types/gamesman_error.h"
#include "core/types/move_array.h"
#include "core/types/position_array.h"
#include "core/types/position_hash_set.h"
#include "core/types/solver/solver.h"
#include "core/types/solver/solver_config.h"
#include "core/types/solver/solver_option.h"
#include "core/types/tier_array.h"
#include "core/types/tier_hash_map.h"
#include "core/types/tier_hash_map_sc.h"
#include "core/types/tier_hash_set.h"
#include "core/types/tier_position_array.h"
#include "core/types/tier_position_hash_set.h"
#include "core/types/tier_queue.h"
#include "core/types/tier_stack.h"
#include "core/types/uwapi/autogui.h"
#include "core/types/uwapi/uwapi.h"

#ifndef GM_CACHE_LINE_SIZE
/**
 * @brief Size of each cache line in bytes. Either set at build time or defaults
 * to 64.
 */
#define GM_CACHE_LINE_SIZE 64
#endif

/**
 * @brief Returns the number of bytes to be padded to an object of size \p n so
 * that its size becomes a multiple of \c GM_CACHE_LINE_SIZE.
 *
 * @param n Size of the object.
 * @return The number of bytes to be padded to an object of size \p n so
 * that its size becomes a multiple of \c GM_CACHE_LINE_SIZE.
 */
#define GM_CACHE_LINE_PAD(n)                                    \
    ((((n) + (GM_CACHE_LINE_SIZE) - 1) / (GM_CACHE_LINE_SIZE) * \
      (GM_CACHE_LINE_SIZE)) -                                   \
     n)

#endif  // GAMESMANONE_CORE_GAMESMAN_TYPES_H_
