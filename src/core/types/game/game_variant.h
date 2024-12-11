/**
 * @file game_variant.h
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief The generic GameVariant type.
 * @version 1.0.1
 * @date 2024-12-10
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

#ifndef GAMESMANONE_CORE_TYPES_GAME_GAME_VARIANT_H_
#define GAMESMANONE_CORE_TYPES_GAME_GAME_VARIANT_H_

#include "core/data_structures/int64_array.h"
#include "core/types/game/game_variant_option.h"

/**
 * @brief Game variant as an array of selected variant options.
 *
 * @details A game variant is determined by a set of variant options. Each
 * variant option decides some aspect of the game rule. The game developer is
 * responsible for providing the possible choices for each one of the variant
 * options as strings (see GameVariantOption::choices). The user of GAMESMAN
 * interactive can then set the variant by selecting a value for each option
 * using the SetVariantOption method provided by the game module.
 *
 * @example A Tic-Tac-Toe game can be generalized and played on a M by N board
 * with a goal of connecting K pieces in a row. Then, we can have three game
 * variant options "dimension M", "dimension N", and "number of pieces to
 * connect (K)." A board too small can make the game less interesting, whereas a
 * board too large can render the game unsolvable. Therefore, the game developer
 * decides to allow M, N, and K to be all within the range [2, 5], and sets the
 * corresponding choices to {"2", "3", "4", "5"}, for each one of the three
 * GameVariantOptions.
 */
typedef struct GameVariant {
    /**
     * Zero-terminated array of game variant options. The user of this struct
     * must make sure that the last item in this array is completely zeroed out.
     */
    const GameVariantOption *options;

    /**
     * Array of selected choice indices to each option. Zero-terminated and
     * aligned to the options array (same number of options and selections.)
     */
    const int *selections;
} GameVariant;

/**
 * @brief Returns the index of the given game VARIANT according to its option
 * selections.
 *
 * @param variant A game variant that belongs to some game.
 * @return Index of the game variant or
 * @return 0 if VARIANT is NULL.
 */
int GameVariantToIndex(const GameVariant *variant);

/**
 * @brief Returns an array of option selections that corresponds to the given
 * VARIANT, which is assumed to be non-NULL.
 *
 * @param index Index of the game variant.
 * @param variant A dummy variant object from which the options available for
 * that game is extracted.
 * @return An array of option selections that corresponds to the given
 * VARIANT.
 */
Int64Array VariantIndexToSelections(int index, const GameVariant *variant);

#endif  // GAMESMANONE_CORE_TYPES_GAME_GAME_VARIANT_H_
