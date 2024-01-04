#include "core/headless/hutils.h"

#include <stdbool.h>  // bool
#include <stddef.h>   // NULL
#include <stdio.h>    // printf, fprintf, stdout, stderr
#include <stdlib.h>   // free
#include <string.h>   // strcmp, strcpy

#include "core/data_structures/int64_array.h"
#include "core/gamesman_types.h"
#include "core/misc.h"
#include "core/solvers/solver_manager.h"
#include "games/game_manager.h"

static int MakeDirectory(ReadOnlyString output);

static int SetGameVariant(const Game *game, int variant_id);

// -----------------------------------------------------------------------------

int HeadlessGetVerbosity(bool verbose, bool quiet) {
    if (verbose) return 2;
    return !quiet;
}

const Game *HeadlessGetGame(ReadOnlyString game_name) {
    const Game *const *all_games = GameManagerGetAllGames();
    int num_games = GameManagerNumGames();
    for (int i = 0; i < num_games; ++i) {
        if (strcmp(game_name, all_games[i]->name) == 0) {
            return all_games[i];
        }
    }
    return NULL;
}

int HeadlessRedirectOutput(ReadOnlyString output) {
    if (output == NULL) return 0;
    
    int error = MakeDirectory(output);
    if (error != 0) {
        fprintf(
            stderr,
            "HeadlessRedirectOutput: failed to mkdir for the output file\n");
        return error;
    }
    if (GuardedFreopen(output, "w", stdout) == NULL) {
        fprintf(stderr, "HeadlessRedirectOutput: failed to redirect output\n");
        return -1;
    }
    return 0;
}

int HeadlessInitSolver(ReadOnlyString game_name, int variant_id,
                       ReadOnlyString data_path) {
    const Game *game = HeadlessGetGame(game_name);
    if (game == NULL) {
        fprintf(stderr, "game [%s] not found\n", game_name);
        return -1;
    }

    if (variant_id >= 0) {
        // Set variant only if user provided a variant id.
        int error = SetGameVariant(game, variant_id);
        if (error != 0) return error;
    }

    return SolverManagerInit(game, data_path);
}

// -----------------------------------------------------------------------------

static int MakeDirectory(ReadOnlyString output) {
    int length = strlen(output);

    // Path is not valid for a file if it's empty or its last character is '/'.
    if (length == 0 || output[length - 1] == '/') return -1;

    int ret = 0;
    char *path = (char *)SafeCalloc(length + 1, sizeof(char));
    strcpy(path, output);
    for (int i = length; i >= 0; --i) {
        // Find the last '/' in path and try to make directory.
        if (path[i] == '/') {
            path[i] = '\0';
            ret = MkdirRecursive(path);
            break;
        }
    }
    // We still reach here if no '/' character is found in path.
    free(path);
    return ret;
}

static int SetGameVariant(const Game *game, int variant_id) {
    const GameVariant *variant = game->GetCurrentVariant();
    if (variant == NULL) {
        if (variant_id == 0) return 0;
        fprintf(stderr,
                "game [%s] has no variant [%d] (only variant 0 is available)\n",
                game->name, variant_id);
        return -1;
    }

    int num_variants = 1;
    for (int i = 0; variant->options[i].num_choices > 0; ++i) {
        num_variants *= variant->options[i].num_choices;
    }
    if (variant_id >= num_variants || variant_id < 0) {
        fprintf(stderr,
                "game [%s] has no variant [%d] (only variants 0-%d are "
                "available)\n",
                game->name, variant_id, num_variants - 1);
        return -1;
    }

    Int64Array selections = VariantIndexToSelections(variant_id, variant);
    if (selections.size <= 0) return 1;

    for (int64_t i = 0; i < selections.size; ++i) {
        int selection = selections.array[i];
        int error = game->SetVariantOption(i, selection);
        if (error != 0) {
            fprintf(stderr,
                    "failed to make selection %d to option %d of game %s\n",
                    selection, (int)i, game->name);
            return error;
        }
    }

    return 0;
}
