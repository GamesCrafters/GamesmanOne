/**
 * @file gamesman_headless.c
 * @author Robert Shi (robertyishi@berkeley.edu)
 *         GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Implementation of GAMESMAN headless mode.
 * @version 1.1.1
 * @date 2024-02-02
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

#include "core/gamesman_headless.h"

#include <stdbool.h>  // bool
#include <stdlib.h>   // atoi
#ifdef USE_MPI
#include <mpi.h>
#endif  // USE_MPI

#include "core/headless/hanalyze.h"
#include "core/headless/hparser.h"
#include "core/headless/hquery.h"
#include "core/headless/hsolve.h"
#include "core/headless/hutils.h"
#include "core/misc.h"
#include "core/types/gamesman_types.h"

int GamesmanHeadlessMain(int argc, char **argv) {
#ifdef USE_MPI
    SafeMpiInit(&argc, &argv);
#endif  // USE_MPI
    HeadlessArguments arguments = HeadlessParseArguments(argc, argv);
    char *game = arguments.game;
    char *data_path = arguments.data_path;
    bool force = arguments.force;
    char *position = arguments.position;
    int verbose = HeadlessGetVerbosity(arguments.verbose, arguments.quiet);
    int variant_id =
        arguments.variant_id != NULL ? atoi(arguments.variant_id) : -1;

    int error = HeadlessRedirectOutput(arguments.output);
    if (error != 0) return error;

    switch (arguments.action) {
        case kHeadlessSolve:
            error = HeadlessSolve(game, variant_id, data_path, force, verbose);
            break;
        case kHeadlessAnalyze:
            error = HeadlessAnalyze(game, variant_id, data_path, force, verbose);
            break;
        case kHeadlessQuery:
            error = HeadlessQuery(game, variant_id, data_path, position);
            break;
        case kHeadlessGetStart:
            error = HeadlessGetStart(game, variant_id);
            break;
        case kHeadlessGetRandom:
            error = HeadlessGetRandom(game, variant_id);
            break;
        default:
            fprintf(stderr, "GamesmanHeadlessMain: unknown action\n");
            error = kNotReachedError;
    }
#ifdef USE_MPI
    SafeMpiFinalize();
#endif  // USE_MPI

    return error;
}
