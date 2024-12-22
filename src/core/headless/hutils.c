/**
 * @file hutils.c
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Implementation of the common utility functions for headless mode.
 * @version 1.0.1
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

#include "core/headless/hutils.h"

#include <stdbool.h>  // bool
#include <stddef.h>   // NULL
#include <stdio.h>    // printf, fprintf, stdout, stderr
#include <stdlib.h>   // free
#include <string.h>   // strcmp, strcpy

#include "core/data_structures/int64_array.h"
#include "core/game_manager.h"
#include "core/misc.h"
#include "core/solvers/solver_manager.h"
#include "core/types/gamesman_types.h"

static int MakeDirectory(ReadOnlyString output);

// -----------------------------------------------------------------------------

int HeadlessGetVerbosity(bool verbose, bool quiet) {
    if (verbose) return 2;
    return !quiet;
}

int HeadlessRedirectOutput(ReadOnlyString output) {
    if (output == NULL) return kNoError;

    int error = MakeDirectory(output);
    if (error != 0) {
        fprintf(
            stderr,
            "HeadlessRedirectOutput: failed to mkdir for the output file\n");
        return error;
    }

    if (GuardedFreopen(output, "w", stdout) == NULL) {
        fprintf(stderr, "HeadlessRedirectOutput: failed to redirect output\n");
        return kFileSystemError;
    }

    return kNoError;
}

int HeadlessInitSolver(ReadOnlyString game_name, int variant_id,
                       ReadOnlyString data_path) {
    const Game *game = GameManagerInitGame(game_name, NULL);
    if (game == NULL) {
        fprintf(stderr, "HeadlessInitSolver: game [%s] not found\n", game_name);
        return kIllegalGameNameError;
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
    int length = (int)strlen(output);

    // Path is not valid for a file if it's empty or its last character is '/'.
    if (length == 0 || output[length - 1] == '/') return -1;

    int ret = kNoError;
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
