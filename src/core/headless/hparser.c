/**
 * @file hparser.c
 * @author Robert Shi (robertyishi@berkeley.edu)
 *         GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Implementation of the command line parsing module for headless mode.
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

#include "core/headless/hparser.h"

#include <argp.h>     // argp related variables and functions
#include <stdbool.h>  // bool, false, true
#include <stddef.h>   // NULL
#include <stdio.h>    // fprintf, stderr, FILE
#include <string.h>   // strcmp

#include "core/constants.h"
#include "core/gamesman_headless.h"
#include "core/types/gamesman_types.h"

ConstantReadOnlyString HeadlessCommands[] = {
    "solve", "analyze", "query", "getstart", "getrandom",
};

static const int kEmptyFlag = 0;
static const char *const kNoArgs = NULL;
static const ArgpArguments kDefaultArgpArguments = {
    .command = NULL,
    .game = NULL,
    .variant_id = NULL,
    .position = NULL,
    .data_path = NULL,
    .output = NULL,
    .force = false,
    .verbose = false,
    .quiet = false,
    .action = kInvalidHeadlessAction,
};

static void PrintVersion(FILE *stream, struct argp_state *state);

static error_t ParseOption(int key, char *arg, struct argp_state *state);
static void ParseArgument(char *arg, struct argp_state *state);
static void ParseCommand(char *arg, ArgpArguments *arguments);
static void ValidateArguments(struct argp_state *state,
                              struct ArgpArguments *arguments);

// Argp extern global variables.

void (*argp_program_version_hook)(FILE *, struct argp_state *) = &PrintVersion;
const char *argp_program_bug_address = "robertyishi@berkeley.edu";

// Argp options list.
static const struct argp_option options[] = {
    {"data-path", 'd', "PATH", kEmptyFlag, "Specify data path", 0},
    {"output", 'o', "PATH", kEmptyFlag, "Specify output file (default=stdout)",
     0},
    {"force", 'f', kNoArgs, kEmptyFlag, "Force re-solve/re-analyze", 1},
    {"quiet", 'q', kNoArgs, kEmptyFlag, "Produce no output", 1},
    {"verbose", 'v', kNoArgs, kEmptyFlag, "Produce verbose output", 1},
    {0},  // Terminator.
};

static const char kArgpDoc[] =
    "\nList of options:"
    "\v"  // Separates message before and after argp usage.
    "GamesmanOne commands:\n"
    "\n"
    "solve or analyze a game\n"
    "    solve\tgamesman solve <game> [<variant>]\n"
    "    analyze\tgamesman analyze <game> [<variant>]\n"
    "\n"
    "query game information\n"
    "    query\tgamesman query <game> <variant> <position>\n"
    "    getstart\tgamesman getstart <game> [<variant>]\n"
    "    getrandom\tgamesman getrandom <game> [<variant>]\n";

// The argp structure.
static struct argp argp = {
    .options = options,
    .parser = &ParseOption,
    .args_doc = "<command> [<args>]",
    .doc = kArgpDoc,
};

// -----------------------------------------------------------------------------

ArgpArguments HeadlessParseArguments(int argc, char **argv) {
    ArgpArguments arguments = kDefaultArgpArguments;

    // Parse arguments
    error_t error = argp_parse(&argp, argc, argv, kEmptyFlag, NULL, &arguments);
    if (error != 0) {
        fprintf(stderr,
                "HeadlessParseArguments: argp_parse returned error code %d\n",
                error);
    }

    return arguments;
}

// -----------------------------------------------------------------------------

static void PrintVersion(FILE *stream, struct argp_state *state) {
    (void)state;  // Unused.
    fprintf(stream, "GamesmanOne version %s\n", kGamesmanVersion);
}

static error_t ParseOption(int key, char *arg, struct argp_state *state) {
    struct ArgpArguments *arguments = state->input;

    switch (key) {
        case 'd':
            arguments->data_path = arg;
            break;
        case 'o':
            arguments->output = arg;
            break;
        case 'f':
            arguments->force = true;
            break;
        case 'v':
            arguments->verbose = true;
            break;
        case 'q':
            arguments->quiet = true;
            break;
        case ARGP_KEY_ARG:
            ParseArgument(arg, state);
            break;
        case ARGP_KEY_END:
            ValidateArguments(state, arguments);
            break;
    }
    return 0;
}

static void ParseArgument(char *arg, struct argp_state *state) {
    struct ArgpArguments *arguments = state->input;

    switch (state->arg_num) {
        case 0:
            ParseCommand(arg, arguments);
            break;
        case 1:
            arguments->game = arg;
            break;
        case 2:
            arguments->variant_id = arg;
            break;
        case 3:
            arguments->position = arg;
            break;
        default:
            argp_error(state, "too many arguments");
    }
}

static void ParseCommand(char *arg, ArgpArguments *arguments) {
    arguments->command = arg;

    // Determine the action based on the command
    for (int i = 0; i < kNumHeadlessActions; ++i) {
        if (strcmp(arg, HeadlessCommands[i]) == 0) {
            arguments->action = i;
            return;
        }
    }
    arguments->action = kInvalidHeadlessAction;
}

static void ValidateArguments(struct argp_state *state,
                              struct ArgpArguments *arguments) {
    int arg_num = state->arg_num;
    ReadOnlyString command = arguments->command;
    int min_args = -1, max_args = -1;

    switch (arguments->action) {
        case kHeadlessSolve:
        case kHeadlessAnalyze:
        case kHeadlessGetStart:
        case kHeadlessGetRandom:
            min_args = 2;
            max_args = 3;
            break;

        case kHeadlessQuery:
            min_args = max_args = 4;
            break;

        default:
            argp_error(state, "invalid command %s", command);
    }

    if (arg_num < min_args) {
        argp_error(
            state,
            "too few arguments for command %s (requires %d, provided %d)",
            command, min_args, arg_num);
    } else if (arg_num > max_args) {
        argp_error(
            state,
            "too many arguments for command %s (at most %d, provided %d)",
            command, max_args, arg_num);
    }
}
