/**
 * @file hsolve.c
 * @author Robert Shi (robertyishi@berkeley.edu)
 *         GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Implementation of the game solving functionality of headless mode.
 * @version 1.0.0
 * @date 2024-01-20
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

#include "core/headless/hsolve.h"

#include <assert.h>   // assert
#include <stdbool.h>  // bool
#include <stddef.h>   // NULL
#include <stdio.h>    // printf, fprintf, stderr
#include <stdlib.h>   // free

#include "core/types/gamesman_types.h"
#include "core/headless/hutils.h"
#include "core/misc.h"
#include "core/solvers/regular_solver/regular_solver.h"
#include "core/solvers/solver_manager.h"
#include "core/solvers/tier_solver/tier_solver.h"
#include "core/game_manager.h"

static void *GenerateSolveOptions(bool force, int verbose);

// -----------------------------------------------------------------------------

int HeadlessSolve(ReadOnlyString game_name, int variant_id,
                  ReadOnlyString data_path, bool force, int verbose) {
    int error = HeadlessInitSolver(game_name, variant_id, data_path);
    if (error != 0) return error;

    void *options = GenerateSolveOptions(force, verbose);
    error = SolverManagerSolve(options);
    free(options);
    if (error != 0) {
        fprintf(stderr, "HeadlessSolve: solve failed with code %d\n", error);
    }
    return error;
}

// -----------------------------------------------------------------------------

static void *GenerateSolveOptions(bool force, int verbose) {
    const Game *game = GameManagerGetCurrentGame();
    assert(game != NULL);

    if (game->solver == &kRegularSolver) {
        RegularSolverSolveOptions *options =
            (RegularSolverSolveOptions *)SafeMalloc(
                sizeof(RegularSolverSolveOptions));
        options->force = force;
        options->verbose = verbose;
        return (void *)options;
    } else if (game->solver == &kTierSolver) {
        TierSolverSolveOptions *options = (TierSolverSolveOptions *)SafeMalloc(
            sizeof(TierSolverSolveOptions));
        options->force = force;
        options->verbose = verbose;
        return (void *)options;
    }  // Append new solvers to the end.

    NotReached("GenerateSolveOptions: no valid solver found");
    return NULL;
}
