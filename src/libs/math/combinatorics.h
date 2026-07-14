/**
 * @file combinatorics.h
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Mathematical combinatorics utility functions.
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

#ifndef GAMESMANONE_LIBS_MATH_COMBINATORICS_H_
#define GAMESMANONE_LIBS_MATH_COMBINATORICS_H_

#include <stdint.h>

/**
 * @brief Returns the number of ways to choose `r` elements from a total of `n`
 * elements.
 *
 * @details Computes the binomial coefficient of `n` and `r`. It leverages a
 * pre-computed cache for smaller values of `n` and `r` to maximize performance.
 * For inputs that exceed cache bounds, it falls back to a Greatest Common
 * Divisor (GCD) based formula calculation to prevent intermediate overflow.
 *
 * @param[in] n Positive integer, number of elements to choose from.
 * @param[in] r Positive integer, number of elements to choose.
 *
 * @return The calculated binomial coefficient if it can be expressed as a
 * 64-bit signed integer.
 * @retval -1 If `n` or `r` is negative, or if the calculation overflows an
 * `int64_t`.
 * @retval 0 If `n` is strictly less than `r`.
 */
int64_t NChooseR(int n, int r);

#endif  // GAMESMANONE_LIBS_MATH_COMBINATORICS_H_
