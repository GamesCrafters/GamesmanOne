/**
 * @file solver_manager.c
 * @author Robert Shi (robertyishi@berkeley.edu)
 *         GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Implementation of the Solver manager module, which handles solver
 * loading, solving, and database status checking.
 *
 * @details One core assumption made by GAMESMAN is that "no more than one game
 * can be loaded at the same time." This makes sense because a user can always
 * run multiple instances of GAMESMAN if they wish to solve/play multiple games
 * simultaneously. As a result, no more than one solver or database can be
 * loaded at the same time. This module handles the loading and deallocation
 * of THE solver used by the current GAMESMAN instance.
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

#include "core/solvers/solver_manager.h"

#include <assert.h>  // assert
#include <stddef.h>  // NULL

#include "core/types/gamesman_types.h"
#include "core/game_manager.h"

static const Solver *current_solver;

int SolverManagerInit(ReadOnlyString data_path) {
    const Game *game = GameManagerGetCurrentGame();
    if (game == NULL) return -1;

    const GameVariant *variant = game->GetCurrentVariant();
    int variant_id = GameVariantToIndex(variant);

    if (current_solver != NULL) {
        current_solver->Finalize();
    }
    current_solver = game->solver;
    return current_solver->Init(game->name, variant_id, game->solver_api,
                                data_path);
}

int SolverManagerGetSolverStatus(void) {
    assert(current_solver != NULL);
    return current_solver->GetStatus();
}

int SolverManagerSolve(void *aux) { return current_solver->Solve(aux); }

int SolverManagerAnalyze(void *aux) { return current_solver->Analyze(aux); }
