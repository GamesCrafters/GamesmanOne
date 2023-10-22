#ifndef GAMESMANONE_CORE_HEADLESS_PARSER_H_
#define GAMESMANONE_CORE_HEADLESS_PARSER_H_

typedef struct ArgpArguments {
    char *game;
    char *variant_id;
    char *position;
    char *data_dir;
    char *output;
    int force;
    int verbose;
    int quiet;
    int action;
} ArgpArguments;

ArgpArguments HeadlessParseArguments(int argc, char **argv);

#endif  // GAMESMANONE_CORE_HEADLESS_PARSER_H_
