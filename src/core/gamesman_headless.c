/**
 * @file gamesman_headless.c
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Implementation of GAMESMAN headless mode.
 * @version 1.3.0
 * @date 2025-05-11
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
#include <stddef.h>   // size_t
#include <stdlib.h>   // atoi
#ifdef USE_MPI
#include <mpi.h>
#endif  // USE_MPI

#include "core/headless/hanalyze.h"
#include "core/headless/hparser.h"
#include "core/headless/hquery.h"
#include "core/headless/hsolve.h"
#include "core/headless/htest.h"
#include "core/headless/hutils.h"
#include "core/misc.h"

/**
 * @brief Convert the input memory limit string \p str, which is in GiB, into
 * an integer memory limit, which is in bytes.
 */
static size_t ParseMemLimit(const char *str) {
    if (str == NULL || *str == '\0') return 0;
    int gigabytes = atoi(str);
    if (gigabytes < 0) return 0;

    return (size_t)gigabytes << 30;
}

int GamesmanHeadlessMain(int argc, char **argv) {
#ifdef USE_MPI
#ifdef _OPENMP
    int thread_level_provided;
    SafeMpiInitThread(&argc, &argv, MPI_THREAD_FUNNELED,
                      &thread_level_provided);
    if (thread_level_provided != MPI_THREAD_FUNNELED) {
        fprintf(stderr,
                "GamesmanHeadlessMain: failed to initialize MPI execution "
                "environment with thread support at level MPI_THREAD_FUNNELED. "
                "Aborting...\n");
        return kMpiError;
    }
#else
    SafeMpiInit(&argc, &argv);
#endif  // _OPENMP
#endif  // USE_MPI
    HeadlessArguments arguments = HeadlessParseArguments(argc, argv);
    char *game = arguments.game;
    char *data_path = arguments.data_path;
    size_t memlimit = ParseMemLimit(arguments.memlimit);
    bool force = arguments.force;
    char *position = arguments.position;
    int verbose = HeadlessGetVerbosity(arguments.verbose, arguments.quiet);
    int variant_id = arguments.variant_id ? atoi(arguments.variant_id) : -1;
    long seed = arguments.seed ? atol(arguments.seed) : (long)time(NULL);

    int error = HeadlessRedirectOutput(arguments.output);
    if (error != 0) return error;

    switch (arguments.action) {
        case kHeadlessSolve:
            error = HeadlessSolve(game, variant_id, data_path, force, verbose,
                                  memlimit);
            break;
        case kHeadlessAnalyze:
            error = HeadlessAnalyze(game, variant_id, data_path, force, verbose,
                                    memlimit);
            break;
        case kHeadlessTest:
            error = HeadlessTest(game, variant_id, seed, verbose);
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
