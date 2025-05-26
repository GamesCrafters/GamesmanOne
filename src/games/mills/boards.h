/**
 * @file boards.h
 * @author Patricia Fong, Kevin Liu, Erwin A. Vedar, Wei Tu, Elmer Lee,
 * Cameron Cheung: developed the first version in GamesmanClassic (m369mm.c).
 * @author Cameron Cheung (cameroncheung@berkeley.edu): designed and coded board
 * strings.
 * @author Robert Shi (robertyishi@berkeley.edu): grouped board formats into an
 * array indexed by board variants.
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Board string formats and slot mappings for all Mills variants.
 * @version 1.0.0
 * @date 2025-05-25
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
#ifndef GAMESMANONE_GAMES_MILLS_BOARD_FORMATS_H_
#define GAMESMANONE_GAMES_MILLS_BOARD_FORMATS_H_

// clang-format off

static const char *const kFormat16 =
    "\n"
    "          0 ----- 1 ----- 2    %c ----- %c ----- %c\n"
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
    "          0 ----- 1 ----- 2    %c ----- %c ----- %c\n"
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
    "        0 --------- 1 --------- 2       %c --------- %c --------- %c\n"
    "        |           |           |       |           |           |\n"
    "        |   3 ----- 4 ----- 5   |       |   %c ----- %c ----- %c   |\n"
    "        |   |       |       |   |       |   |       |       |   |     X has %d pieces left to place.\n"
    "        |   |   6 - 7 - 8   |   |       |   |   %c - %c - %c   |   |     X has %d pieces on the board.\n"
    "        |   |   |       |   |   |       |   |   |       |   |   |\n"
    "LEGEND: 9 - 10- 11      12- 13- 14      %c - %c - %c       %c - %c - %c\n"
    "        |   |   |       |   |   |       |   |   |       |   |   |\n"
    "        |   |   15- 16- 17  |   |       |   |   %c - %c - %c   |   |     O has %d left to place.\n"
    "        |   |       |       |   |       |   |       |       |   |     O has %d pieces on the board.\n"
    "        |   18 ---- 19 ---- 20  |       |   %c ----- %c ----- %c   |\n"
    "        |           |           |       |           |           |\n"
    "        21 -------- 22 -------- 23      %c --------- %c --------- %c\n\n";

static const char *const kFormat24BoardOnly =
    "\n"
    "        0 --------- 1 --------- 2       %c --------- %c --------- %c\n"
    "        |           |           |       |           |           |\n"
    "        |   3 ----- 4 ----- 5   |       |   %c ----- %c ----- %c   |\n"
    "        |   |       |       |   |       |   |       |       |   |\n"
    "        |   |   6 - 7 - 8   |   |       |   |   %c - %c - %c   |   |\n"
    "        |   |   |       |   |   |       |   |   |       |   |   |\n"
    "LEGEND: 9 - 10- 11      12- 13- 14      %c - %c - %c       %c - %c - %c\n"
    "        |   |   |       |   |   |       |   |   |       |   |   |\n"
    "        |   |   15- 16- 17  |   |       |   |   %c - %c - %c   |   |\n"
    "        |   |       |       |   |       |   |       |       |   |\n"
    "        |   18 ---- 19 ---- 20  |       |   %c ----- %c ----- %c   |\n"
    "        |           |           |       |           |           |\n"
    "        21 -------- 22 -------- 23      %c --------- %c --------- %c\n\n";

static const char *const kFormat24Plus =
    "\n"
    "        0 --------- 1 --------- 2       %c --------- %c --------- %c\n"
    "        | \\         |         / |       | \\         |         / |\n"
    "        |   3 ----- 4 ----- 5   |       |   %c ----- %c ----- %c   |\n"
    "        |   | \\     |     / |   |       |   | \\     |     / |   |     X has %d pieces left to place.\n"
    "        |   |   6 - 7 - 8   |   |       |   |   %c - %c - %c   |   |     X has %d pieces on the board.\n"
    "        |   |   |       |   |   |       |   |   |       |   |   |\n"
    "LEGEND: 9 - 10- 11      12- 13- 14      %c - %c - %c       %c - %c - %c\n"
    "        |   |   |       |   |   |       |   |   |       |   |   |\n"
    "        |   |   15- 16- 17  |   |       |   |   %c - %c - %c   |   |     O has %d left to place.\n"
    "        |   | /     |     \\ |   |       |   | /     |     \\ |   |     O has %d pieces on the board.\n"
    "        |   18 ---- 19 ---- 20  |       |   %c ----- %c ----- %c   |\n"
    "        | /         |         \\ |       | /         |         \\ |\n"
    "        21 -------- 22 -------- 23      %c --------- %c --------- %c\n\n";

static const char *const kFormat24PlusBoardOnly =
    "\n"
    "        0 --------- 1 --------- 2       %c --------- %c --------- %c\n"
    "        | \\         |         / |       | \\         |         / |\n"
    "        |   3 ----- 4 ----- 5   |       |   %c ----- %c ----- %c   |\n"
    "        |   | \\     |     / |   |       |   | \\     |     / |   |\n"
    "        |   |   6 - 7 - 8   |   |       |   |   %c - %c - %c   |   |\n"
    "        |   |   |       |   |   |       |   |   |       |   |   |\n"
    "LEGEND: 9 - 10- 11      12- 13- 14      %c - %c - %c       %c - %c - %c\n"
    "        |   |   |       |   |   |       |   |   |       |   |   |\n"
    "        |   |   15- 16- 17  |   |       |   |   %c - %c - %c   |   |\n"
    "        |   | /     |     \\ |   |       |   | /     |     \\ |   |\n"
    "        |   18 ---- 19 ---- 20  |       |   %c ----- %c ----- %c   |\n"
    "        | /         |         \\ |       | /         |         \\ |\n"
    "        21 -------- 22 -------- 23      %c --------- %c --------- %c\n\n";

static const char *const kFormat25 =
    "\n"
    "        0 --------- 1 --------- 2       %c --------- %c --------- %c\n"
    "        | \\         |         / |       | \\         |         / |\n"
    "        |   3 ----- 4 ----- 5   |       |   %c ----- %c ----- %c   |\n"
    "        |   |       |       |   |       |   |       |       |   |     X has %d pieces left to place.\n"
    "        |   |   6 - 7 - 8   |   |       |   |   %c - %c - %c   |   |     X has %d pieces on the board.\n"
    "        |   |   |   |   |   |   |       |   |   |   |   |   |   |\n"
    "LEGEND: 9 - 10- 11 -12- 13- 14- 15      %c - %c - %c - %c - %c - %c - %c\n"
    "        |   |   |   |   |   |   |       |   |   |   |   |   |   |\n"
    "        |   |   16- 17- 18  |   |       |   |   %c - %c - %c   |   |     O has %d left to place.\n"
    "        |   |       |       |   |       |   |       |       |   |     O has %d pieces on the board.\n"
    "        |   19 ---- 20 ---- 21  |       |   %c ----- %c ----- %c   |\n"
    "        | /         |         \\ |       | /         |         \\ |\n"
    "        22 -------- 23 -------- 24      %c --------- %c --------- %c\n\n";

static const char *const kFormat25BoardOnly =
    "\n"
    "        0 --------- 1 --------- 2       %c --------- %c --------- %c\n"
    "        | \\         |         / |       | \\         |         / |\n"
    "        |   3 ----- 4 ----- 5   |       |   %c ----- %c ----- %c   |\n"
    "        |   |       |       |   |       |   |       |       |   |\n"
    "        |   |   6 - 7 - 8   |   |       |   |   %c - %c - %c   |   |\n"
    "        |   |   |   |   |   |   |       |   |   |   |   |   |   |\n"
    "LEGEND: 9 - 10- 11 -12- 13- 14- 15      %c - %c - %c - %c - %c - %c - %c\n"
    "        |   |   |   |   |   |   |       |   |   |   |   |   |   |\n"
    "        |   |   16- 17- 18  |   |       |   |   %c - %c - %c   |   |\n"
    "        |   |       |       |   |       |   |       |       |   |\n"
    "        |   19 ---- 20 ---- 21  |       |   %c ----- %c ----- %c   |\n"
    "        | /         |         \\ |       | /         |         \\ |\n"
    "        22 -------- 23 -------- 24      %c --------- %c --------- %c\n\n";

// clang-format on

static const char *const kFormats[] = {
    kFormat16, kFormat16,     kFormat17,     kFormat24,
    kFormat24, kFormat24Plus, kFormat24Plus, kFormat25,
};

static const char *const kBoardOnlyFormats[] = {
    kFormat16BoardOnly,     kFormat16BoardOnly, kFormat17BoardOnly,
    kFormat24BoardOnly,     kFormat24BoardOnly, kFormat24PlusBoardOnly,
    kFormat24PlusBoardOnly, kFormat25BoardOnly,
};

// clang-format off

static const char *const kBoardIdxToFormal16[] = {
    "a5", "c5", "e5",
    "b4", "c4", "d4",
    "a3", "b3", "d3", "e3",
    "b2", "c2", "d2",
    "a1", "c1", "e1",
};

static const char *const kBoardIdxToFormal17[] = {
    "a5", "c5", "e5",
    "b4", "c4", "d4",
    "a3", "b3", "c3", "d3", "e3",
    "b2", "c2", "d2",
    "a1", "c1", "e1",
};

static const char *const kBoardIdxToFormal24[] = {
    "a7", "d7", "g7",
    "b6", "d6", "f6",
    "c5", "d5", "e5",
    "a4", "b4", "c4", "e4", "f4", "g4",
    "c3", "d3", "e3",
    "b2", "d2", "f2",
    "a1", "d1", "g1",
};

static const char *const kBoardIdxToFormal25[] = {
    "a7", "d7", "g7",
    "b6", "d6", "f6",
    "c5", "d5", "e5",
    "a4", "b4", "c4", "d4", "e4", "f4", "g4",
    "c3", "d3", "e3",
    "b2", "d2", "f2",
    "a1", "d1", "g1",
};

// clang-format on

static const char *const *const kBoardIdxToFormal[] = {
    kBoardIdxToFormal16, kBoardIdxToFormal16, kBoardIdxToFormal17,
    kBoardIdxToFormal24, kBoardIdxToFormal24, kBoardIdxToFormal24,
    kBoardIdxToFormal24, kBoardIdxToFormal25,
};

#endif  // GAMESMANONE_GAMES_MILLS_BOARD_FORMATS_H_
