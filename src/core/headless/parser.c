/*
 * Headless Commands:
 * solve <game> [<variant_id>]    // solve and analyze game.
 * analyze <game> [<variant_id>]  // analyze only, assuming solved.
 *
 * query <game> <variant_id> <position>  // get detailed position response
 * getstart <game> [<variant_id>]        // get starting position
 * getrandom <game> [<variant_id>]       // get a random position
 *
 * Options:
 * --data-path=<path>
 * -o, --output=<path>
 * -f, --force    // only effective when solving/analyzing
 * -q, --quiet    // only effective when solving/analyzing
 * -v, --verbose  // only effective when solving/analyzing
 * -V, --version  // automatic
 *     --usage    // automatic
 * -?, --help     // automatic
 */

#include "core/headless/parser.h"

#include <argp.h>    // argp related variables and functions
#include <stddef.h>  // NULL
#include <stdio.h>   // fprintf, stderr, FILE
#include <string.h>  // strcmp

#include "core/constants.h"

enum HeadlessActions {
    kInvalid = 0,
    kSolve,
    kAnalyze,
    kQuery,
    kGetStart,
    kGetRandom,
};

static const int kEmptyFlag = 0;
static const char *const kNoArgs = NULL;
static const ArgpArguments kDefaultArgpArguments = {
    .game = "-",
    .variant_id = "-",
    .position = "-",
    .data_dir = "-",
    .output = "-",
    .force = 0,
    .verbose = 0,
    .quiet = 0,
    .action = kInvalid,
};

static void PrintVersion(FILE *stream, struct argp_state *state);

static error_t ParseOption(int key, char *arg, struct argp_state *state);
static void ParseArgument(char *arg, struct argp_state *state);
static void ParseCommand(const char *arg, ArgpArguments *arguments);
static int ArgumentMismatch(unsigned int arg_num, int action);

// Argp extern global variables.

void (*argp_program_version_hook)(FILE *, struct argp_state *) = &PrintVersion;
const char *argp_program_bug_address = "robertyishi@berkeley.edu";

// Argp options list.
static const struct argp_option options[] = {
    {"data-path", 'd', "PATH", kEmptyFlag, "Specify data path", 0},
    {"output", 'o', "PATH", kEmptyFlag, "Specify output file (default=stdout)",
     0},
    {"force", 'f', kNoArgs, kEmptyFlag, "Force re-solve/re-analyze", 0},
    {"quiet", 'q', kNoArgs, kEmptyFlag, "Produce no output", 0},
    {"verbose", 'v', kNoArgs, kEmptyFlag, "Produce verbose output", 0},
    {0},  // Terminator.
};

static const char kArgpDoc[] =
    "\nList of options:"
    "\v"  // Separates message before and after argp usage.
    "GamesmanOne commands:\n"
    "\n"
    "solve or analyze a game\n"
    "    solve\tSolve a game\n"
    "    analyze\tAnalyze a game\n"
    "\n"
    "query game information\n"
    "    query\tGet detailed position response of a game position\n"
    "    getstart\tGet the starting position of a game\n"
    "    getrandom\tGet a random legal position from the game\n";

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
            arguments->data_dir = arg;
            break;
        case 'o':
            arguments->output = arg;
            break;
        case 'f':
            arguments->force = 1;
            break;
        case 'v':
            arguments->verbose = 1;
            break;
        case 'q':
            arguments->quiet = 1;
            break;
        case ARGP_KEY_ARG:
            ParseArgument(arg, state);
            break;
        case ARGP_KEY_END:
            if (ArgumentMismatch(state->arg_num, arguments->action)) {
                argp_usage(state);
            }
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
            argp_usage(state);
    }
}

static void ParseCommand(const char *arg, ArgpArguments *arguments) {
    // Determine the action based on the command
    if (strcmp(arg, "solve") == 0) {
        arguments->action = kSolve;
    } else if (strcmp(arg, "analyze") == 0) {
        arguments->action = kAnalyze;
    } else if (strcmp(arg, "query") == 0) {
        arguments->action = kQuery;
    } else if (strcmp(arg, "getstart") == 0) {
        arguments->action = kGetStart;
    } else if (strcmp(arg, "getrandom") == 0) {
        arguments->action = kGetRandom;
    } else {
        arguments->action = kInvalid;
    }
}

static int ArgumentMismatch(unsigned int arg_num, int action) {
    switch (action) {
        case kSolve:
        case kAnalyze:
        case kGetStart:
        case kGetRandom:
            return arg_num < 2 || arg_num > 3;

        case kQuery:
            return arg_num != 4;
    }

    // Default - return mismatch.
    return 1;
}
