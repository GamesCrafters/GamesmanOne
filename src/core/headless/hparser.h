/**
 * @file hparser.h
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Command line parsing module for headless mode.
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

#ifndef GAMESMANONE_CORE_HEADLESS_HPARSER_H_
#define GAMESMANONE_CORE_HEADLESS_HPARSER_H_

/*
 * Headless Commands:
 * solve <game> [<variant_id>]    // solve and analyze game.
 * analyze <game> [<variant_id>]  // analyze only, assuming solved.
 * test <game> [<variant_id>]     // test game variant or all variants.
 *
 * query <game> <variant_id> <position>  // get detailed position response.
 * getstart <game> [<variant_id>]        // get starting position.
 * getrandom <game> [<variant_id>]       // get a random position.
 *
 * Options:
 * --data-path=<path>
 * -M<limit>, --memory=<limit>  // in GiB
 * -o, --output=<path>
 * --seed=<seed>  // only effective when testing
 * -f, --force    // only effective when solving/analyzing
 * -q, --quiet    // only effective when solving/analyzing
 * -v, --verbose  // only effective when solving/analyzing
 * -V, --version  // automatic
 *     --usage    // automatic
 * -?, --help     // automatic
 */

/** @brief Enumeration of all possible actions in headless mode. */
enum HeadlessAction {
    kInvalidHeadlessAction = -1, /**< Invalid. */
    kHeadlessSolve,              /**< Solve. */
    kHeadlessAnalyze,            /**< Analyze. */
    kHeadlessTest,               /**< Test. */
    kHeadlessQuery,              /**< Query position. */
    kHeadlessGetStart,           /**< Get start position. */
    kHeadlessGetRandom,          /**< Get random position. */
    kNumHeadlessActions,         /**< Number of all valid actions. */
};

/** @brief Collection of all arguments used for command line parsing. */
typedef struct HeadlessArguments {
    char *command;    /**< User command. See Headless Commands for details. */
    char *game;       /**< Game name. */
    char *variant_id; /**< Variant index. */
    char *position;   /**< Position to query. */
    char *data_path;  /**< Path to the "data" directory, NULL for default. */
    char *memlimit;   /**< Heap memory limit, NULL for default (90%). */
    char *output;     /**< Path to output file, defaults to stdout if NULL. */
    char *seed;       /**< Seed for PRNGs, defaults to current system time. */
    int action;       /**< Action to take. */
    int force;        /**< Whether to force solve/analyze. */
    int verbose;      /**< Whether to print additional output. */
    int quiet;        /**< Whether to give no output. */
} HeadlessArguments;

HeadlessArguments HeadlessParseArguments(int argc, char **argv);

#endif  // GAMESMANONE_CORE_HEADLESS_HPARSER_H_
