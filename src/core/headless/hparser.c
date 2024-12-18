/**
 * @file hparser.c
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Implementation of the command line parsing module for headless mode.
 * @version 1.2.0
 * @date 2024-09-08
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

#include <getopt.h>  // struct option, getopt_long
#include <stdarg.h>  // va_list, va_start, va_end
#include <stddef.h>  // NULL
#include <stdio.h>   // vprintf, fprintf, stderr, FILE, stdout
#include <stdlib.h>  // exit
#include <string.h>  // strcmp

#include "config.h"
#include "core/gamesman_headless.h"
#include "core/types/gamesman_types.h"

static HeadlessArguments arguments;
static ConstantReadOnlyString HeadlessCommands[] = {
    "solve", "analyze", "query", "getstart", "getrandom",
};

static const struct option kLongOptions[] = {
    {
        .name = "data-path",
        .has_arg = required_argument,
        .flag = NULL,
        .val = 'd',
    },
    {
        .name = "memory",
        .has_arg = required_argument,
        .flag = NULL,
        .val = 'M',
    },
    {
        .name = "force",
        .has_arg = no_argument,
        .flag = NULL,
        .val = 'f',
    },
    {
        .name = "help",
        .has_arg = no_argument,
        .flag = NULL,
        .val = '?',
    },
    {
        .name = "output",
        .has_arg = required_argument,
        .flag = NULL,
        .val = 'o',
    },
    {
        .name = "quiet",
        .has_arg = no_argument,
        .flag = NULL,
        .val = 'q',
    },
    {
        .name = "usage",
        .has_arg = no_argument,
        .flag = NULL,
        .val = '?',
    },
    {
        .name = "verbose",
        .has_arg = no_argument,
        .flag = NULL,
        .val = 'v',
    },
    {
        .name = "version",
        .has_arg = no_argument,
        .flag = NULL,
        .val = 'V',
    },
    {0},
};

static const HeadlessArguments kDefaultHeadlessArguments = {
    // All other fields initializes to 0.
    .action = kInvalidHeadlessAction,
};

static void PrintVersion(FILE *stream);
static void ParseOption(int key, int option_index);
static void ParseArgument(char *arg, int arg_num);
static void ParseCommand(char *arg);
static void ValidateArguments(int arg_num);
static void PrintUsage(void);
static void ParserError(const char *format, ...);

static const char kDoc[] =
    "\nList of options:\n\n"
    "\t-d, --data-path=PATH\tSpecify data path (default=\"data\")\n"
    "\t-M, --memory=LIMIT\tSpecify heap memory limit in GiB (default=90%)"
    "\t-o, --output=PATH\tSpecify output file (default=stdout)\n"
    "\t-f, --force\t\tForce re-solve/re-analyze\n"
    "\t-q, --quiet\t\tProduce no output\n"
    "\t-v, --verbose\t\tProduce verbose output\n"
    "\t-?, --help\t\tGive this help list\n"
    "\t--usage\t\t\tGive a short usage message\n"
    "\t-V, --version\t\tPrint program version\n"
    "\nGamesmanOne commands:\n"
    "\n"
    "solve or analyze a game\n"
    "    solve\tgamesman solve <game> [<variant>]\n"
    "    analyze\tgamesman analyze <game> [<variant>]\n"
    "\n"
    "query game information\n"
    "    query\tgamesman query <game> <variant> <position>\n"
    "    getstart\tgamesman getstart <game> [<variant>]\n"
    "    getrandom\tgamesman getrandom <game> [<variant>]\n";

// -----------------------------------------------------------------------------

HeadlessArguments HeadlessParseArguments(int argc, char **argv) {
    arguments = kDefaultHeadlessArguments;
    int key;
    while (1) {
        /* getopt_long stores the option index here. */
        int option_index = 0;
        key =
            getopt_long(argc, argv, "dM:f?o:qvV", kLongOptions, &option_index);
        /* Detect the end of the options. */
        if (key == -1) break;
        ParseOption(key, option_index);
    }

    // Process remaining non-option
    int arg_num = 0;
    while (optind < argc) {
        ParseArgument(argv[optind++], arg_num++);
    }
    ValidateArguments(arg_num);

    return arguments;
}

// -----------------------------------------------------------------------------

static void PrintVersion(FILE *stream) {
    fprintf(stream, "GamesmanOne version %s (%s)\n", GAMESMAN_VERSION,
            GAMESMAN_DATE);
}

static void ParseOption(int key, int option_index) {
    switch (key) {
        case 0:
            /* If this option set a flag, do nothing else now. */
            if (kLongOptions[option_index].flag != 0) break;
            printf("option %s", kLongOptions[option_index].name);
            if (optarg) printf(" with arg %s", optarg);
            printf("\n");
            break;

        case 'd':
            arguments.data_path = optarg;
            break;

        case 'M':
            arguments.memlimit = optarg;
            break;

        case 'f':
            arguments.force = 1;
            break;

        case 'h':
            PrintUsage();
            exit(0);

        case 'o':
            arguments.output = optarg;
            break;

        case 'q':
            arguments.quiet = 1;
            break;

        case 'v':
            arguments.verbose = 1;
            break;

        case 'V':
            PrintVersion(stdout);
            exit(0);

        default:
            PrintUsage();
            exit(kHeadlessError);
    }
}

static void ParseArgument(char *arg, int arg_num) {
    switch (arg_num) {
        case 0:
            ParseCommand(arg);
            break;
        case 1:
            arguments.game = arg;
            break;
        case 2:
            arguments.variant_id = arg;
            break;
        case 3:
            arguments.position = arg;
            break;
        default:
            fprintf(stderr, "too many arguments\n");
            exit(kHeadlessError);
    }
}

static void ParseCommand(char *arg) {
    arguments.command = arg;

    // Determine the action based on the command
    for (int i = 0; i < kNumHeadlessActions; ++i) {
        if (strcmp(arg, HeadlessCommands[i]) == 0) {
            arguments.action = i;
            return;
        }
    }
    arguments.action = kInvalidHeadlessAction;
}

static void ValidateArguments(int arg_num) {
    ReadOnlyString command = arguments.command;
    int min_args = -1, max_args = -1;

    switch (arguments.action) {
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
            ParserError("invalid command %s", command);
    }

    if (arg_num < min_args) {
        ParserError(
            "too few arguments for command %s (requires %d, provided %d)",
            command, min_args, arg_num);
    } else if (arg_num > max_args) {
        ParserError(
            "too many arguments for command %s (at most %d, provided %d)",
            command, max_args, arg_num);
    }
}

static void PrintUsage(void) {
    printf("Usage: %s\n%s\n", "gamesman [OPTION...] <command> [<args>]", kDoc);
}

static void ParserError(const char *format, ...) {
    va_list args;
    va_start(args, format);  // Initialize the argument list

    // Print the formatted error message to stderr
    vfprintf(stderr, format, args);
    fprintf(stderr, "\n");

    va_end(args);  // Clean up the argument list
    exit(kHeadlessError);
}
