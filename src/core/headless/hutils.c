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

// -----------------------------------------------------------------------------

int HeadlessGetVerbosity(bool verbose, bool quiet) {
    if (verbose) return 2;
    return !quiet;
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
    const Game *game = GameManagerInitGame(game_name, NULL);
    if (game == NULL) {
        fprintf(stderr, "HeadlessInitSolver: game [%s] not found\n", game_name);
        return -1;
    }

    if (variant_id >= 0) {
        // Set variant only if user provided a variant id.
        int error = GameManagerSetVariant(variant_id);
        if (error != 0) return error;
    }

    return SolverManagerInit(data_path);
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
