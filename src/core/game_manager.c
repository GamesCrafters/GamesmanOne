/**
 * @file game_manager.c
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief The Game Manager Module which handles game initialization and
 * finalization.
 * @version 1.1.2
 * @date 2024-12-22
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

#include "core/game_manager.h"

#include <assert.h>  // assert
#include <stddef.h>  // NULL
#include <stdio.h>   // fprintf, stderr
#include <string.h>  // strcmp

#include "core/data_structures/int64_array.h"
#include "core/types/gamesman_types.h"
#include "games/game_list.h"

static const Game *current_game;

// -----------------------------------------------------------------------------

const Game *const *GameManagerGetAllGames(void) {
    return GameListGetAllGames();
}

int GameManagerNumGames(void) { return GameListGetNumGames(); }

const Game *GameManagerInitGame(ReadOnlyString game_name, void *aux) {
    const Game *const *all_games = GameListGetAllGames();
    for (int i = 0; all_games[i] != NULL; ++i) {
        if (strcmp(game_name, all_games[i]->name) == 0) {
            return GameManagerInitGameIndex(i, aux);
        }
    }

    return NULL;
}

const Game *GameManagerInitGameIndex(int index, void *aux) {
    assert(index >= 0 && index < GameManagerNumGames());
    const Game *const *all_games = GameListGetAllGames();
    int error = all_games[index]->Init(aux);
    if (error != kNoError) {
        fprintf(stderr,
                "GameManagerInitGameIndex: failed to initialize game [%s], "
                "code %d.\n",
                all_games[index]->name, error);
        return NULL;
    }

    return current_game = all_games[index];
}

const Game *GameManagerGetCurrentGame(void) { return current_game; }

int GameManagerCurrentGameSupportsMpi(void) {
    return current_game->solver->supports_mpi;
}

int GameManagerSetVariant(int variant_id) {
    const GameVariant *variant = NULL;
    if (current_game->GetCurrentVariant != NULL) {
        variant = current_game->GetCurrentVariant();
    }

    if (variant == NULL) {
        // If variants are not implemented, 0 is the only valid default variant
        // id.
        if (variant_id == 0) return kNoError;
        fprintf(stderr,
                "GameManagerSetVariant: game [%s] has no variant [%d] (only "
                "variant 0 is available)\n",
                current_game->name, variant_id);
        return kIllegalGameVariantError;
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
        return kIllegalGameVariantError;
    }

    Int64Array selections = VariantIndexToSelections(variant_id, variant);
    if (selections.array == NULL) return kMallocFailureError;

    for (int i = 0; i < (int)selections.size; ++i) {
        int selection = (int)selections.array[i];
        int error = current_game->SetVariantOption(i, selection);
        if (error != 0) {
            fprintf(stderr,
                    "GameManagerSetVariant: failed to make selection %d to "
                    "option %d of game %s\n",
                    selection, i, current_game->name);
            return error;
        }
    }
    Int64ArrayDestroy(&selections);

    return kNoError;
}

void GameManagerFinalize(void) {
    current_game->Finalize();
    current_game = NULL;
}
