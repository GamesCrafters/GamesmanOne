#ifndef GAMESMANONE_GAMES_MILLS_BOARD_FORMATS_H_
#define GAMESMANONE_GAMES_MILLS_BOARD_FORMATS_H_

// clang-format off

static const char *const kFormat16 =
    "\n"
    "          0 ----- 1 ----- 2    %c ----- %c ----- %c     It is %c's turn.\n"
    "          |       |       |    |       |       |\n"
    "          |   3 - 4 - 5   |    |   %c - %c - %c   |     X has %d pieces left to place.\n"
    "          |   |       |   |    |   |       |   |     X has %d pieces on the board.\n"
    "LEGEND:   6 - 7       8 - 9    %c - %c       %c - %c\n"
    "          |   |       |   |    |   |       |   |     O has %d left to place.\n"
    "          |  10 - 11- 12  |    |   %c - %c - %c   |    O has %d pieces on the board.\n"
    "          |       |       |    |       |       |\n"
    "          13 ---- 14 ---- 15   %c ----- %c ----- %c\n\n";


static const char *const kFormat16BoardOnly =
"\n"
"          0 ----- 1 ----- 2    %c ----- %c ----- %c\n"
"          |       |       |    |       |       |\n"
"          |   3 - 4 - 5   |    |   %c - %c - %c   |\n"
"          |   |       |   |    |   |       |   |\n"
"LEGEND:   6 - 7       8 - 9    %c - %c       %c - %c\n"
"          |   |       |   |    |   |       |   |\n"
"          |  10 - 11- 12  |    |   %c - %c - %c   |\n"
"          |       |       |    |       |       |\n"
"          13 ---- 14 ---- 15   %c ----- %c ----- %c\n\n";

static const char *const kFormat17 =
    "\n"
    "          0 ----- 1 ----- 2    %c ----- %c ----- %c     It is %c's turn.\n"
    "          |       |       |    |       |       |\n"
    "          |   3 - 4 - 5   |    |   %c - %c - %c   |     X has %d pieces left to place.\n"
    "          |   |   |   |   |    |   |   |   |   |     X has %d pieces on the board.\n"
    "LEGEND:   6 - 7 - 8 - 9 - 10   %c - %c - %c - %c - %c\n"
    "          |   |   |   |   |    |   |   |   |   |     O has %d left to place.\n"
    "          |  11 - 12- 13  |    |   %c - %c - %c   |     O has %d pieces on the board.\n"
    "          |       |       |    |       |       |\n"
    "          14 ---- 15 ---- 16   %c ----- %c ----- %c\n\n";

static const char *const kFormat17BoardOnly =
    "\n"
    "          0 ----- 1 ----- 2    %c ----- %c ----- %c\n"
    "          |       |       |    |       |       |\n"
    "          |   3 - 4 - 5   |    |   %c - %c - %c   |\n"
    "          |   |   |   |   |    |   |   |   |   |\n"
    "LEGEND:   6 - 7 - 8 - 9 - 10   %c - %c - %c - %c - %c\n"
    "          |   |   |   |   |    |   |   |   |   |\n"
    "          |  11 - 12- 13  |    |   %c - %c - %c   |\n"
    "          |       |       |    |       |       |\n"
    "          14 ---- 15 ---- 16   %c ----- %c ----- %c\n\n";

static const char *const kFormat24 =
    "\n"
    "        0 --------- 1 --------- 2       %c --------- %c --------- %c     It is %c's turn.\n"
    "        |           |           |       |            |            |\n"
    "        |   3 ----- 4 ----- 5   |       |   %c ----- %c ----- %c   |\n"
    "        |   |       |       |   |       |   |       |       |   |     X has %d pieces left to place.\n"
    "        |   |   6 - 7 - 8   |   |       |   |   %c - %c - %c   |   |     X has %d pieces on the board.\n"
    "        |   |   |       |   |   |       |   |   |       |   |   |\n"
    "LEGEND: 9 - 10- 11      12- 13- 14      %c - %c - %c       %c - %c - %c\n"
    "        |   |   |       |   |   |       |   |   |       |   |   |\n"
    "        |   |   15- 16- 17  |   |       |   |   %c - %c - %c   |   |     O has %d left to place.\n"
    "        |   |       |       |   |       |   |       |       |   |     O has %d pieces on the board.\n"
    "        |   18 ---- 19 ---- 20  |       |   %c ----- %c ----- %c   |\n"
    "        |           |           |       |            |            |\n"
    "        21 -------- 22 -------- 23      %c --------- %c --------- %c\n\n";

static const char *const kFormat24BoardOnly =
    "\n"
    "        0 --------- 1 --------- 2       %c --------- %c --------- %c\n"
    "        |           |           |       |            |            |\n"
    "        |   3 ----- 4 ----- 5   |       |   %c ----- %c ----- %c   |\n"
    "        |   |       |       |   |       |   |       |       |   |\n"
    "        |   |   6 - 7 - 8   |   |       |   |   %c - %c - %c   |   |\n"
    "        |   |   |       |   |   |       |   |   |       |   |   |\n"
    "LEGEND: 9 - 10- 11      12- 13- 14      %c - %c - %c       %c - %c - %c\n"
    "        |   |   |       |   |   |       |   |   |       |   |   |\n"
    "        |   |   15- 16- 17  |   |       |   |   %c - %c - %c   |   |\n"
    "        |   |       |       |   |       |   |       |       |   |\n"
    "        |   18 ---- 19 ---- 20  |       |   %c ----- %c ----- %c   |\n"
    "        |           |           |       |            |            |\n"
    "        21 -------- 22 -------- 23      %c --------- %c --------- %c\n\n";

static const char *const kFormat24Plus =
    "\n"
    "        0 --------- 1 --------- 2       %c --------- %c --------- %c     It is %c's turn.\n"
    "        | \         |         / |       | \          |          / |\n"
    "        |   3 ----- 4 ----- 5   |       |   %c ----- %c ----- %c   |\n"
    "        |   | \     |     / |   |       |   | \     |     / |   |     X has %d pieces left to place.\n"
    "        |   |   6 - 7 - 8   |   |       |   |   %c - %c - %c   |   |     X has %d pieces on the board.\n"
    "        |   |   |       |   |   |       |   |   |       |   |   |\n"
    "LEGEND: 9 - 10- 11      12- 13- 14      %c - %c - %c       %c - %c - %c\n"
    "        |   |   |       |   |   |       |   |   |       |   |   |\n"
    "        |   |   15- 16- 17  |   |       |   |   %c - %c - %c   |   |     O has %d left to place.\n"
    "        |   | /     |     \ |   |       |   | /     |     \ |   |     O has %d pieces on the board.\n"
    "        |   18 ---- 19 ---- 20  |       |   %c ----- %c ----- %c   |\n"
    "        | /         |         \ |       | /          |          \ |\n"
    "        21 -------- 22 -------- 23      %c --------- %c --------- %c\n\n";

static const char *const kFormat24PlusBoardOnly =
    "\n"
    "        0 --------- 1 --------- 2       %c --------- %c --------- %c\n"
    "        | \         |         / |       | \          |          / |\n"
    "        |   3 ----- 4 ----- 5   |       |   %c ----- %c ----- %c   |\n"
    "        |   | \     |     / |   |       |   | \     |     / |   |\n"
    "        |   |   6 - 7 - 8   |   |       |   |   %c - %c - %c   |   |\n"
    "        |   |   |       |   |   |       |   |   |       |   |   |\n"
    "LEGEND: 9 - 10- 11      12- 13- 14      %c - %c - %c       %c - %c - %c\n"
    "        |   |   |       |   |   |       |   |   |       |   |   |\n"
    "        |   |   15- 16- 17  |   |       |   |   %c - %c - %c   |   |\n"
    "        |   | /     |     \ |   |       |   | /     |     \ |   |\n"
    "        |   18 ---- 19 ---- 20  |       |   %c ----- %c ----- %c   |\n"
    "        | /         |         \ |       | /          |          \ |\n"
    "        21 -------- 22 -------- 23      %c --------- %c --------- %c\n\n";

static const char *const kFormat25 =
    "\n"
    "        0 --------- 1 --------- 2       %c --------- %c --------- %c     It is %c's turn.\n"
    "        | \         |         / |       | \          |          / |\n"
    "        |   3 ----- 4 ----- 5   |       |   %c ----- %c ----- %c   |\n"
    "        |   |       |       |   |       |   |       |       |   |     X has %d pieces left to place.\n"
    "        |   |   6 - 7 - 8   |   |       |   |   %c - %c - %c   |   |     X has %d pieces on the board.\n"
    "        |   |   |   |   |   |   |       |   |   |   |   |   |   |\n"
    "LEGEND: 9 - 10- 11 -12- 13- 14- 15      %c - %c - %c - %c - %c - %c - %c\n"
    "        |   |   |   |   |   |   |       |   |   |   |   |   |   |\n"
    "        |   |   16- 17- 18  |   |       |   |   %c - %c - %c   |   |     O has %d left to place.\n"
    "        |   |       |       |   |       |   |       |       |   |     O has %d pieces on the board.\n"
    "        |   19 ---- 20 ---- 21  |       |   %c ----- %c ----- %c   |\n"
    "        | /         |         \ |       | /          |          \ |\n"
    "        22 -------- 23 -------- 24      %c --------- %c --------- %c\n\n";

static const char *const kFormat25BoardOnly =
    "\n"
    "        0 --------- 1 --------- 2       %c --------- %c --------- %c\n"
    "        | \         |         / |       | \          |          / |\n"
    "        |   3 ----- 4 ----- 5   |       |   %c ----- %c ----- %c   |\n"
    "        |   |       |       |   |       |   |       |       |   |\n"
    "        |   |   6 - 7 - 8   |   |       |   |   %c - %c - %c   |   |\n"
    "        |   |   |   |   |   |   |       |   |   |   |   |   |   |\n"
    "LEGEND: 9 - 10- 11 -12- 13- 14- 15      %c - %c - %c - %c - %c - %c - %c\n"
    "        |   |   |   |   |   |   |       |   |   |   |   |   |   |\n"
    "        |   |   16- 17- 18  |   |       |   |   %c - %c - %c   |   |\n"
    "        |   |       |       |   |       |   |       |       |   |\n"
    "        |   19 ---- 20 ---- 21  |       |   %c ----- %c ----- %c   |\n"
    "        | /         |         \ |       | /          |          \ |\n"
    "        22 -------- 23 -------- 24      %c --------- %c --------- %c\n\n";

// clang-format on

#endif  // GAMESMANONE_GAMES_MILLS_BOARD_FORMATS_H_
