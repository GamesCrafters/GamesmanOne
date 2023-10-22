#include "core/gamesman_headless.h"

#include <stdio.h>   // printf

#include "core/headless/parser.h"

// -----------------------------------------------------------------------------

int GamesmanHeadlessMain(int argc, char **argv) {
    ArgpArguments arguments = HeadlessParseArguments(argc, argv);
    printf("Action: %d\n", arguments.action);
    printf("Game: %s\n", arguments.game);
    printf("Variant ID: %s\n", arguments.variant_id);
    printf("Position: %s\n", arguments.position);
    printf("Data Directory: %s\n", arguments.data_dir);
    printf("Output: %s\n", arguments.output);
    printf("Force: %d\n", arguments.force);
    printf("Verbose: %d\n", arguments.verbose);
    printf("Quiet: %d\n", arguments.quiet);
    return 0;
}

// -----------------------------------------------------------------------------
