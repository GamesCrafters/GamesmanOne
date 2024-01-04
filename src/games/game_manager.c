/**
 * @file game_manager.c
 * @author Robert Shi (robertyishi@berkeley.edu)
 *         GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Implementation of the Game Manager Module.
 *
 * @version 1.01
 * @date 2024-01-04
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

#include <assert.h>  // assert
#include <stddef.h>  // NULL
#include <stdio.h>   // fprintf, stderr

#include "core/data_structures/int64_array.h"
#include "core/gamesman_types.h"

// 1. To add a new game, include the game header here.

#include "games/mttt/mttt.h"
#include "games/mtttier/mtttier.h"

// 2. Then add the new game object to the list.

static const Game *const kAllGames[] = {&kMtttier, &kMttt};
static const Game *current_game;

// -----------------------------------------------------------------------------

const Game *const *GameManagerGetAllGames(void) { return kAllGames; }

int GameManagerNumGames(void) {
    return sizeof(kAllGames) / sizeof(kAllGames[0]);
}

const Game *GameManagerInitGame(ReadOnlyString game_name, void *aux) {
    int num_games = GameManagerNumGames();
    for (int i = 0; i < num_games; ++i) {
        if (strcmp(game_name, kAllGames[i]->name) == 0) {
            return GameManagerInitGameIndex(i, aux);
        }
    }
    return NULL;
}

const Game *GameManagerInitGameIndex(int index, void *aux) {
    assert(index >= 0 && index < GameManagerNumGames());
    int ret = kAllGames[index]->Init(aux);
    if (ret != 0) {
        fprintf(stderr,
                "GameManagerInitGameIndex: failed to initialize game [%s], "
                "code %d.\n",
                kAllGames[index]->name, ret);
        return NULL;
    }

    return current_game = kAllGames[index];
}

const Game *GameManagerGetCurrentGame(void) { return current_game; }

int GameManagerSetVariant(int variant_id) {
    const GameVariant *variant = current_game->GetCurrentVariant();
    if (variant == NULL) {
        // If variants are not implemented, 0 is the only valid default variant
        // id.
        if (variant_id == 0) return 0;
        fprintf(stderr,
                "GameManagerSetVariant: game [%s] has no variant [%d] (only "
                "variant 0 is available)\n",
                current_game->name, variant_id);
        return -1;
    }

    int num_variants = 1;
    for (int i = 0; variant->options[i].num_choices > 0; ++i) {
        num_variants *= variant->options[i].num_choices;
    }
    if (variant_id >= num_variants || variant_id < 0) {
        fprintf(stderr,
                "GameManagerSetVariant: game [%s] has no variant [%d] (only "
                "variants 0-%d are available)\n",
                current_game->name, variant_id, num_variants - 1);
        return -1;
    }

    Int64Array selections = VariantIndexToSelections(variant_id, variant);
    if (selections.size <= 0) return 1;

    for (int64_t i = 0; i < selections.size; ++i) {
        int selection = selections.array[i];
        int error = current_game->SetVariantOption(i, selection);
        if (error != 0) {
            fprintf(stderr,
                    "GameManagerSetVariant: failed to make selection %d to "
                    "option %d of game %s\n",
                    selection, (int)i, current_game->name);
            return error;
        }
    }
    Int64ArrayDestroy(&selections);

    return 0;
}

void GameManagerFinalize(void) {
    current_game->Finalize();
    current_game = NULL;
}
