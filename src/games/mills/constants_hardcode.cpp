#include <cassert>
#include <cstdint>
#include <iostream>
#include <string>

#include "masks.h"

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

static const char *const kBoardOnlyFormats[] = {
    kFormat16BoardOnly,     kFormat16BoardOnly, kFormat17BoardOnly,
    kFormat24BoardOnly,     kFormat24BoardOnly, kFormat24PlusBoardOnly,
    kFormat24PlusBoardOnly, kFormat25BoardOnly,
};

static int grid_idx_to_board_idx[NUM_BOARD_AND_PIECES_CHOICES][64];

static void BuildGridIdxToBoardIdx(void) {
    for (int i = 0; i < NUM_BOARD_AND_PIECES_CHOICES; ++i) {
        for (int j = 0; j < kNumSlots[i]; ++j) {
            grid_idx_to_board_idx[i][kBoardIdxToGridIdx[i][j]] = j;
        }
    }
}

static void PrintMask(uint64_t mask, int board_id) {
    std::string s(kBoardOnlyFormats[board_id]);
    for (int i = 0; i < kNumSlots[board_id]; ++i) {
        s.replace(
            s.find("%c"), 2,
            ((mask >> kBoardIdxToGridIdx[board_id][i]) & 1ULL) ? "X" : " ");
    }
    std::cout << s;
}

static void PrintDestMasks(int board_id) {
    std::cout << "PRINTING BOARD ID == " << board_id << "\n";
    int j = 0;
    for (int i = 0; i < 56; ++i) {
        if (!kDestMasks[board_id][i]) continue;
        std::cout << "grid index: " << i << ", board index: " << j++ << "\n";
        PrintMask(kDestMasks[board_id][i], board_id);
    }
}

static void PrintBoardMask(int board_id) {
    PrintMask(kBoardMasks[board_id], board_id);
}

static void PrintInnerRingMask(int board_id) {
    PrintMask(kInnerRingMasks[board_id], board_id);
}

static void PrintOuterRingMask(int board_id) {
    PrintMask(kOuterRingMasks[board_id], board_id);
}

static void PrintLineMasks(int board_id) {
    std::cout << "PRINTING BOARD ID == " << board_id << "\n";
    for (int i = 0; i < kNumLines[board_id]; ++i) {
        PrintMask(kLineMasks[board_id][i], board_id);
    }
}

static void PrintParticipatingLines(int board_id) {
    std::cout << "PRINTING PARTICIPATING LINES MASK FOR BOARD ID == "
              << board_id << "\n";
    for (int i = 0; i < 56; ++i) {
        for (int j = 0; j < kNumParticipatingLines[board_id][i]; ++j) {
            PrintMask(kParticipatingLines[board_id][i][j], board_id);
        }
        if (kNumParticipatingLines[board_id][i]) {
            std::cout << "grid index " << i << " has "
                      << kNumParticipatingLines[board_id][i]
                      << " line completions\n\n";
        }
    }
}

int main(void) {
    BuildGridIdxToBoardIdx();

    for (int i = 0; i < 8; ++i) {
        PrintParticipatingLines(i);
    }

    return 0;
}
