/**
 * @file gamesman_memory_internal.h
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Internal allocation controls for Gamesman tests.
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

#ifndef GAMESMANONE_CORE_GAMESMAN_MEMORY_INTERNAL_H_
#define GAMESMANONE_CORE_GAMESMAN_MEMORY_INTERNAL_H_

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// These functions are available when GAMESMAN_ENABLE_TESTING is enabled.

/** Sets the maximum request size in bytes for GamesmanMalloc and
 * GamesmanCallocWhole in the current process. */
void GamesmanSetAllocationLimitForTesting(size_t max_bytes);

/** Removes the limit and returns whether it rejected any allocation. */
bool GamesmanResetAllocationLimitForTesting(void);

#ifdef __cplusplus
}
#endif

#endif  // GAMESMANONE_CORE_GAMESMAN_MEMORY_INTERNAL_H_
