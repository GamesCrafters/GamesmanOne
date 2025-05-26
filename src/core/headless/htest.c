/**
 * @file htest.h
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Implementation of the game testing functionality of headless mode.
 * @version 1.0.0
 * @date 2025-05-10
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

#include <stddef.h>  // NULL
#include <stdio.h>   // printf, fprintf, stderr

#include "core/game_manager.h"
#include "core/headless/hutils.h"
#include "core/misc.h"
#include "core/solvers/regular_solver/regular_solver.h"
#include "core/solvers/solver_manager.h"
#include "core/solvers/tier_solver/tier_solver.h"
#include "core/types/gamesman_types.h"

static int TestOneVariant(ReadOnlyString game_name, int variant_id, long seed) {
    printf("TESTING VARIANT %d OF GAME %s\n", variant_id, game_name);
    int error = HeadlessInitSolver(game_name, variant_id, NULL);
    if (error != 0) return error;

    if (GameManagerGetCurrentGame()->solver == &kTierSolver) {
        TierSolverTestOptions options = {
            .seed = seed, .test_size = 1000, .verbose = 1};
        error = SolverManagerTest(&options);
    } else if (GameManagerGetCurrentGame()->solver == &kRegularSolver) {
        RegularSolverTestOptions options = {
            .seed = seed, .test_size = 1000000, .verbose = 1};
        error = SolverManagerTest(&options);
    } else {
        NotReached("TestOneVariant: unknown solver\n");
    }

    if (error) {
        printf("%s\n", SolverManagerExplainTestError(error));
    }

    GameManagerFinalize();

    return kGameTestFailureError;
}

static int GetNumVarinats(ReadOnlyString game_name, int *num_variants) {
    int error = HeadlessInitSolver(game_name, -1, NULL);
    if (error != 0) return error;

    *num_variants = GameManagerGetNumVariants();
    GameManagerFinalize();

    return error;
}

int HeadlessTest(ReadOnlyString game_name, int variant_id, long seed,
                 int verbose) {
    (void)verbose;         // TODO: SolverManagerTest should take in verbose
    if (variant_id < 0) {  // Test all variants
        int num_variants;
        int error = GetNumVarinats(game_name, &num_variants);
        if (error) return error;

        for (int i = 0; i < num_variants; ++i) {
            error = TestOneVariant(game_name, i, seed);
            if (error != 0) {
                fprintf(stderr, "HeadlessTest: test failed with code %d\n",
                        error);
                return error;
            }
        }
    } else {  // Test the given variant
        int error = TestOneVariant(game_name, variant_id, seed);
        if (error != 0) {
            fprintf(stderr, "HeadlessTest: test failed with code %d\n", error);
        }
    }
    puts(
        "\n****************************\n"
        "***** ALL TESTS PASSED *****\n"
        "****************************\n");

    return kNoError;
}
