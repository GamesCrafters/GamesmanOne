/**
 * @file game_variant_option.h
 * @author Robert Shi (robertyishi@berkeley.edu)
 *         GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief The GameVariantOption type and related constants.
 * @details A GameVariantOption defines an option of a game. A set of options
 * choices make up the rule of a variant of a game.
 * @version 1.0.0
 * @date 2024-01-21
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

#ifndef GAMESMANONE_CORE_TYPES_GAME_GAME_VARIANT_OPTION_H_
#define GAMESMANONE_CORE_TYPES_GAME_GAME_VARIANT_OPTION_H_

#include "core/types/base.h"

/**
 * @brief Constants used by the GameVariantOption type and related functions
 * implementations.
 */
enum GameVariantOptionConstants {
    kGameVariantOptionNameMax = 63, /**< Max length GameVariantOption::name. */
};

/** @brief Game variant option for display in GAMESMAN interactive mode. */
typedef struct GameVariantOption {
    /** Human-readable name of the option. */
    char name[kGameVariantOptionNameMax + 1];

    /** Number of choices associate with the option. */
    int num_choices;

    /** An array of strings, where each string is a name of a choice. Length =
     * num_choices.  */
    const ConstantReadOnlyString *choices;
} GameVariantOption;

#endif  // GAMESMANONE_CORE_TYPES_GAME_GAME_VARIANT_OPTION_H_
