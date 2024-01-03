/*
 * Headless Commands:
 * solve <game> [<variant_id>]    // solve and analyze game.
 * analyze <game> [<variant_id>]  // analyze only, assuming solved.
 *
 * query <game> <variant_id> <position>  // get detailed position response.
 * getstart <game> [<variant_id>]        // get starting position.
 * getrandom <game> [<variant_id>]       // get a random position.
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

#ifndef GAMESMANONE_CORE_HEADLESS_HPARSER_H_
#define GAMESMANONE_CORE_HEADLESS_HPARSER_H_

#include <stdbool.h>  // bool

typedef struct ArgpArguments {
    char *command;
    char *game;
    char *variant_id;
    char *position;
    char *data_path;
    char *output;
    int action;
    bool force;
    bool verbose;
    bool quiet;
} ArgpArguments;

ArgpArguments HeadlessParseArguments(int argc, char **argv);

#endif  // GAMESMANONE_CORE_HEADLESS_HPARSER_H_
