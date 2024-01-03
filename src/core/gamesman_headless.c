#include "core/gamesman_headless.h"

#include <stdbool.h>  // bool
#include <stdlib.h>   // atoi

#include "core/gamesman_types.h"
#include "core/headless/hanalyze.h"
#include "core/headless/hparser.h"
#include "core/headless/hquery.h"
#include "core/headless/hsolve.h"
#include "core/headless/hutils.h"

// -----------------------------------------------------------------------------

int GamesmanHeadlessMain(int argc, char **argv) {
    ArgpArguments arguments = HeadlessParseArguments(argc, argv);
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
            return HeadlessSolve(game, variant_id, data_path, force, verbose);
        case kHeadlessAnalyze:
            return HeadlessAnalyze(game, variant_id, data_path, force, verbose);
        case kHeadlessQuery:
            return HeadlessQuery(game, variant_id, data_path, position);
        case kHeadlessGetStart:
            return HeadlessGetStart(game, variant_id);
        case kHeadlessGetRandom:
            return HeadlessGetRandom(game, variant_id);
        default:
            NotReached("GamesmanHeadlessMain: unknown action\n");
    }
    // Not reached.
    return -1;
}

// -----------------------------------------------------------------------------
