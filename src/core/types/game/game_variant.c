/**
 * @file game_variant.c
 * @author Robert Shi (robertyishi@berkeley.edu)
 *         GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Implementation of the GameVariant type.
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

#include "core/types/game/game_variant.h"

#include <stddef.h>  // NULL

#include "core/data_structures/int64_array.h"
#include "core/types/game/game_variant_option.h"

static int GameVariantGetNumOptions(const GameVariant *variant);

// -----------------------------------------------------------------------------

int GameVariantToIndex(const GameVariant *variant) {
    if (variant == NULL) return 0;

    int ret = 0;
    for (int i = 0; variant->options[i].num_choices > 0; ++i) {
        ret = ret * variant->options[i].num_choices + variant->selections[i];
    }

    return ret;
}

Int64Array VariantIndexToSelections(int index, const GameVariant *variant) {
    int num_options = GameVariantGetNumOptions(variant);
    Int64Array ret;
    Int64ArrayInit(&ret);
    if (!Int64ArrayResize(&ret, num_options)) return ret;

    for (int i = num_options - 1; i >= 0; --i) {
        int selection = index % variant->options[i].num_choices;
        ret.array[i] = selection;
        index /= variant->options[i].num_choices;
    }

    return ret;
}

// -----------------------------------------------------------------------------

// Assumes VARIANT is not NULL.
static int GameVariantGetNumOptions(const GameVariant *variant) {
    int ret = 0;
    while (variant->options[ret].num_choices > 0) {
        ++ret;
    }
    return ret;
}
