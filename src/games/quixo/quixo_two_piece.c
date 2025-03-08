/**
 * @file quixo_two_piece.c
 * @author François Bonnet: original published version,
 *         arXiv:2007.15895v1
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @brief DEPRECATED module for testing two piece hash.
 * @version 2.0.0
 * @date 2025-01-07
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

#include <assert.h>  // assert
#include <stddef.h>  // NULL
#include <stdio.h>   // sprintf
#include <stdlib.h>  // atoi
#include <string.h>  // strtok_r, strlen, strcpy

#include "core/hash/two_piece.h"
#include "core/solvers/tier_solver/tier_solver.h"
#include "core/types/gamesman_types.h"
#include "games/quixo/quixo.h"

// =================================== Types ===================================

typedef union {
    int8_t unpacked[2];  // 0: num_x, 1: num_o
    Tier hash;
} QuixoTier;

enum {
    kLeft,
    kRight,
    kUp,
    kDown,
} QuixoMoveDirections;

typedef union {
    struct {
        int8_t dir;
        int8_t idx;
    } unpacked;
    Move hash;
} QuixoMove;

// ================================= Constants =================================

enum {
    kNumPlayers = 2,
    kNumVariants = 3,
    kSideLengthMax = 5,
    kBoardSizeMax = kSideLengthMax * kSideLengthMax,
    kNumLinesMax = kSideLengthMax * 2 + 2,
    kNumMovesPerDirMax = kSideLengthMax * 3 - 4,
};

static const QuixoMove kQuixoMoveInit = {.hash = 0};

static const char kDirToChar[] = {'R', 'L', 'D', 'U'};

static const int kNumLines[kNumVariants] = {
    5 * 2 + 2,
    4 * 2 + 2,
    3 * 2 + 2,
};

static const int kNumMovesPerDir[kNumVariants] = {
    5 * 3 - 4,
    4 * 3 - 4,
    3 * 3 - 4,
};

static const uint64_t kLines[kNumVariants][kNumPlayers][kNumLinesMax] = {
    // =========
    // == 5x5 ==
    // =========
    {
        // 5x5 X
        {0x1f0000000000000, 0xf800000000000, 0x7c0000000000, 0x3e000000000,
         0x1f00000000, 0x108421000000000, 0x84210800000000, 0x42108400000000,
         0x21084200000000, 0x10842100000000, 0x104104100000000,
         0x11111000000000},
        // 5x5 O
        {0x1f00000, 0xf8000, 0x7c00, 0x3e0, 0x1f, 0x1084210, 0x842108, 0x421084,
         0x210842, 0x108421, 0x1041041, 0x111110},
    },
    // =========
    // == 4x4 ==
    // =========
    {
        // 4x4 X
        {0xf00000000000, 0xf0000000000, 0xf000000000, 0xf00000000,
         0x888800000000, 0x444400000000, 0x222200000000, 0x111100000000,
         0x842100000000, 0x124800000000},
        // 4x4 O
        {0xf000, 0xf00, 0xf0, 0xf, 0x8888, 0x4444, 0x2222, 0x1111, 0x8421,
         0x1248},
    },
    // =========
    // == 3x3 ==
    // =========
    {
        // 3x3 X
        {0x1c000000000, 0x3800000000, 0x700000000, 0x12400000000, 0x9200000000,
         0x4900000000, 0x11100000000, 0x5400000000},
        // 3x3 O
        {0x1c0, 0x38, 0x7, 0x124, 0x92, 0x49, 0x111, 0x54},
    },
};

static const uint64_t kEdges[kNumVariants][kNumPlayers] = {
    // 5x5
    {0x1f8c63f00000000, 0x1f8c63f},
    // 4x4
    {0xf99f00000000, 0xf99f},
    // 3x3
    {0x1ef00000000, 0x1ef},
};

static const uint64_t kMoveLeft[kNumVariants][kNumMovesPerDirMax][4] = {
    // =========
    // == 5x5 ==
    // =========
    {
        {0x100000000000000, 0x1000000, 0x10000000000000, 0x1f0000001f00000},
        {0x80000000000000, 0x800000, 0x10000000000000, 0xf0000000f00000},
        {0x40000000000000, 0x400000, 0x10000000000000, 0x70000000700000},
        {0x20000000000000, 0x200000, 0x10000000000000, 0x30000000300000},
        {0x8000000000000, 0x80000, 0x800000000000, 0xf8000000f8000},
        {0x400000000000, 0x4000, 0x40000000000, 0x7c0000007c00},
        {0x20000000000, 0x200, 0x2000000000, 0x3e0000003e0},
        {0x1000000000, 0x10, 0x100000000, 0x1f0000001f},
        {0x800000000, 0x8, 0x100000000, 0xf0000000f},
        {0x400000000, 0x4, 0x100000000, 0x700000007},
        {0x200000000, 0x2, 0x100000000, 0x300000003},
    },
    // =========
    // == 4x4 ==
    // =========
    {
        {0x800000000000, 0x8000, 0x100000000000, 0xf0000000f000},
        {0x400000000000, 0x4000, 0x100000000000, 0x700000007000},
        {0x200000000000, 0x2000, 0x100000000000, 0x300000003000},
        {0x80000000000, 0x800, 0x10000000000, 0xf0000000f00},
        {0x8000000000, 0x80, 0x1000000000, 0xf0000000f0},
        {0x800000000, 0x8, 0x100000000, 0xf0000000f},
        {0x400000000, 0x4, 0x100000000, 0x700000007},
        {0x200000000, 0x2, 0x100000000, 0x300000003},
    },
    // =========
    // == 3x3 ==
    // =========
    {
        {0x10000000000, 0x100, 0x4000000000, 0x1c0000001c0},
        {0x8000000000, 0x80, 0x4000000000, 0xc0000000c0},
        {0x2000000000, 0x20, 0x800000000, 0x3800000038},
        {0x400000000, 0x4, 0x100000000, 0x700000007},
        {0x200000000, 0x2, 0x100000000, 0x300000003},
    },
};

static const uint64_t kMoveRight[kNumVariants][kNumMovesPerDirMax][4] = {
    // =========
    // == 5x5 ==
    // =========
    {
        {0x80000000000000, 0x800000, 0x100000000000000, 0x180000001800000},
        {0x40000000000000, 0x400000, 0x100000000000000, 0x1c0000001c00000},
        {0x20000000000000, 0x200000, 0x100000000000000, 0x1e0000001e00000},
        {0x10000000000000, 0x100000, 0x100000000000000, 0x1f0000001f00000},
        {0x800000000000, 0x8000, 0x8000000000000, 0xf8000000f8000},
        {0x40000000000, 0x400, 0x400000000000, 0x7c0000007c00},
        {0x2000000000, 0x20, 0x20000000000, 0x3e0000003e0},
        {0x800000000, 0x8, 0x1000000000, 0x1800000018},
        {0x400000000, 0x4, 0x1000000000, 0x1c0000001c},
        {0x200000000, 0x2, 0x1000000000, 0x1e0000001e},
        {0x100000000, 0x1, 0x1000000000, 0x1f0000001f},
    },
    // =========
    // == 4x4 ==
    // =========
    {
        {0x400000000000, 0x4000, 0x800000000000, 0xc0000000c000},
        {0x200000000000, 0x2000, 0x800000000000, 0xe0000000e000},
        {0x100000000000, 0x1000, 0x800000000000, 0xf0000000f000},
        {0x10000000000, 0x100, 0x80000000000, 0xf0000000f00},
        {0x1000000000, 0x10, 0x8000000000, 0xf0000000f0},
        {0x400000000, 0x4, 0x800000000, 0xc0000000c},
        {0x200000000, 0x2, 0x800000000, 0xe0000000e},
        {0x100000000, 0x1, 0x800000000, 0xf0000000f},
    },
    // =========
    // == 3x3 ==
    // =========
    {
        {0x8000000000, 0x80, 0x10000000000, 0x18000000180},
        {0x4000000000, 0x40, 0x10000000000, 0x1c0000001c0},
        {0x800000000, 0x8, 0x2000000000, 0x3800000038},
        {0x200000000, 0x2, 0x400000000, 0x600000006},
        {0x100000000, 0x1, 0x400000000, 0x700000007},
    },
};

static const uint64_t kMoveUp[kNumVariants][kNumMovesPerDirMax][4] = {
    // =========
    // == 5x5 ==
    // =========
    {
        {0x100000000000000, 0x1000000, 0x1000000000, 0x108421001084210},
        {0x80000000000000, 0x800000, 0x800000000, 0x84210800842108},
        {0x40000000000000, 0x400000, 0x400000000, 0x42108400421084},
        {0x20000000000000, 0x200000, 0x200000000, 0x21084200210842},
        {0x10000000000000, 0x100000, 0x100000000, 0x10842100108421},
        {0x8000000000000, 0x80000, 0x1000000000, 0x8421000084210},
        {0x800000000000, 0x8000, 0x100000000, 0x842100008421},
        {0x400000000000, 0x4000, 0x1000000000, 0x421000004210},
        {0x40000000000, 0x400, 0x100000000, 0x42100000421},
        {0x20000000000, 0x200, 0x1000000000, 0x21000000210},
        {0x2000000000, 0x20, 0x100000000, 0x2100000021},
    },
    // =========
    // == 4x4 ==
    // =========
    {
        {0x800000000000, 0x8000, 0x800000000, 0x888800008888},
        {0x400000000000, 0x4000, 0x400000000, 0x444400004444},
        {0x200000000000, 0x2000, 0x200000000, 0x222200002222},
        {0x100000000000, 0x1000, 0x100000000, 0x111100001111},
        {0x80000000000, 0x800, 0x800000000, 0x88800000888},
        {0x10000000000, 0x100, 0x100000000, 0x11100000111},
        {0x8000000000, 0x80, 0x800000000, 0x8800000088},
        {0x1000000000, 0x10, 0x100000000, 0x1100000011},
    },
    // =========
    // == 3x3 ==
    // =========
    {
        {0x10000000000, 0x100, 0x400000000, 0x12400000124},
        {0x8000000000, 0x80, 0x200000000, 0x9200000092},
        {0x4000000000, 0x40, 0x100000000, 0x4900000049},
        {0x2000000000, 0x20, 0x400000000, 0x2400000024},
        {0x800000000, 0x8, 0x100000000, 0x900000009},
    },
};

static const uint64_t kMoveDown[kNumVariants][kNumMovesPerDirMax][4] = {
    // =========
    // == 5x5 ==
    // =========
    {
        {0x8000000000000, 0x80000, 0x100000000000000, 0x108000001080000},
        {0x800000000000, 0x8000, 0x10000000000000, 0x10800000108000},
        {0x400000000000, 0x4000, 0x100000000000000, 0x108400001084000},
        {0x40000000000, 0x400, 0x10000000000000, 0x10840000108400},
        {0x20000000000, 0x200, 0x100000000000000, 0x108420001084200},
        {0x2000000000, 0x20, 0x10000000000000, 0x10842000108420},
        {0x1000000000, 0x10, 0x100000000000000, 0x108421001084210},
        {0x800000000, 0x8, 0x80000000000000, 0x84210800842108},
        {0x400000000, 0x4, 0x40000000000000, 0x42108400421084},
        {0x200000000, 0x2, 0x20000000000000, 0x21084200210842},
        {0x100000000, 0x1, 0x10000000000000, 0x10842100108421},
    },
    // =========
    // == 4x4 ==
    // =========
    {
        {0x80000000000, 0x800, 0x800000000000, 0x880000008800},
        {0x10000000000, 0x100, 0x100000000000, 0x110000001100},
        {0x8000000000, 0x80, 0x800000000000, 0x888000008880},
        {0x1000000000, 0x10, 0x100000000000, 0x111000001110},
        {0x800000000, 0x8, 0x800000000000, 0x888800008888},
        {0x400000000, 0x4, 0x400000000000, 0x444400004444},
        {0x200000000, 0x2, 0x200000000000, 0x222200002222},
        {0x100000000, 0x1, 0x100000000000, 0x111100001111},
    },
    // =========
    // == 3x3 ==
    // =========
    {
        {0x2000000000, 0x20, 0x10000000000, 0x12000000120},
        {0x800000000, 0x8, 0x4000000000, 0x4800000048},
        {0x400000000, 0x4, 0x10000000000, 0x12400000124},
        {0x200000000, 0x2, 0x8000000000, 0x9200000092},
        {0x100000000, 0x1, 0x4000000000, 0x4900000049},
    },
};

static const int kDirIndexToSrc[kNumVariants][4][kNumMovesPerDirMax] = {
    // 5x5
    {
        // 5x5 Left
        {0, 1, 2, 3, 5, 10, 15, 20, 21, 22, 23},
        // 5x5 Right
        {1, 2, 3, 4, 9, 14, 19, 21, 22, 23, 24},
        // 5x5 Up
        {0, 1, 2, 3, 4, 5, 9, 10, 14, 15, 19},
        // 5x5 Down
        {5, 9, 10, 14, 15, 19, 20, 21, 22, 23, 24},
    },
    // 4x4
    {
        // 4x4 Left
        {0, 1, 2, 4, 8, 12, 13, 14},
        // 4x4 Right
        {1, 2, 3, 7, 11, 13, 14, 15},
        // 4x4 Up
        {0, 1, 2, 3, 4, 7, 8, 11},
        // 4x4 Down
        {4, 7, 8, 11, 12, 13, 14, 15},
    },
    // 3x3
    {
        // 3x3 Left
        {0, 1, 3, 6, 7},
        // 3x3 Right
        {1, 2, 5, 7, 8},
        // 3x3 Up
        {0, 1, 2, 3, 5},
        // 3x3 Down
        {3, 5, 6, 7, 8},
    },
};

static const int kDirSrcToIndex[kNumVariants][4][kBoardSizeMax] = {
    // 5x5
    {
        // 5x5 Left
        {0, 1, 2, 3, 0, 4, 0, 0, 0, 0, 5,  0, 0,
         0, 0, 6, 0, 0, 0, 0, 7, 8, 9, 10, 0},
        // 5x5 Right
        {0, 0, 1, 2, 3, 0, 0, 0, 0, 4, 0, 0, 0,
         0, 5, 0, 0, 0, 0, 6, 0, 7, 8, 9, 10},
        // 5x5 Up
        {0, 1, 2, 3, 4, 5, 0,  0, 0, 6, 7, 0, 0,
         0, 8, 9, 0, 0, 0, 10, 0, 0, 0, 0, 0},
        // 5x5 Down
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 0, 0,
         0, 3, 4, 0, 0, 0, 5, 6, 7, 8, 9, 10},
    },
    // 4x4
    {
        // 4x4 Left
        {0, 1, 2, 0, 3, 0, 0, 0, 4, 0, 0, 0, 5, 6, 7, 0},
        // 4x4 Right
        {0, 0, 1, 2, 0, 0, 0, 3, 0, 0, 0, 4, 0, 5, 6, 7},
        // 4x4 Up
        {0, 1, 2, 3, 4, 0, 0, 5, 6, 0, 0, 6, 0, 0, 0, 0},
        // 4x4 Down
        {0, 0, 0, 0, 0, 0, 0, 1, 2, 0, 0, 3, 4, 5, 6, 7},
    },
    // 3x3
    {
        // 3x3 Left
        {0, 1, 0, 2, 0, 0, 3, 4, 0},
        // 3x3 Right
        {0, 0, 1, 0, 0, 2, 0, 3, 4},
        // 3x3 Up
        {0, 1, 2, 3, 0, 4, 0, 0, 0},
        // 3x3 Down
        {0, 0, 0, 0, 0, 1, 2, 3, 4},
    },
};

static const int kSymmetryMatrix[kNumVariants][8][kBoardSizeMax] = {
    // 5x5
    {
        {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12,
         13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24},
        {20, 15, 10, 5,  0,  21, 16, 11, 6,  1,  22, 17, 12,
         7,  2,  23, 18, 13, 8,  3,  24, 19, 14, 9,  4},
        {24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12,
         11, 10, 9,  8,  7,  6,  5,  4,  3,  2,  1,  0},
        {4,  9,  14, 19, 24, 3,  8,  13, 18, 23, 2,  7, 12,
         17, 22, 1,  6,  11, 16, 21, 0,  5,  10, 15, 20},
        {4,  3,  2,  1,  0,  9,  8,  7,  6,  5,  14, 13, 12,
         11, 10, 19, 18, 17, 16, 15, 24, 23, 22, 21, 20},
        {24, 19, 14, 9,  4,  23, 18, 13, 8,  3,  22, 17, 12,
         7,  2,  21, 16, 11, 6,  1,  20, 15, 10, 5,  0},
        {20, 21, 22, 23, 24, 15, 16, 17, 18, 19, 10, 11, 12,
         13, 14, 5,  6,  7,  8,  9,  0,  1,  2,  3,  4},
        {0,  5,  10, 15, 20, 1,  6,  11, 16, 21, 2,  7, 12,
         17, 22, 3,  8,  13, 18, 23, 4,  9,  14, 19, 24},
    },
    // 4x4
    {
        {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15},
        {12, 8, 4, 0, 13, 9, 5, 1, 14, 10, 6, 2, 15, 11, 7, 3},
        {15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0},
        {3, 7, 11, 15, 2, 6, 10, 14, 1, 5, 9, 13, 0, 4, 8, 12},
        {3, 2, 1, 0, 7, 6, 5, 4, 11, 10, 9, 8, 15, 14, 13, 12},
        {15, 11, 7, 3, 14, 10, 6, 2, 13, 9, 5, 1, 12, 8, 4, 0},
        {12, 13, 14, 15, 8, 9, 10, 11, 4, 5, 6, 7, 0, 1, 2, 3},
        {0, 4, 8, 12, 1, 5, 9, 13, 2, 6, 10, 14, 3, 7, 11, 15},
    },
    // 3x3
    {
        {0, 1, 2, 3, 4, 5, 6, 7, 8},
        {6, 3, 0, 7, 4, 1, 8, 5, 2},
        {8, 7, 6, 5, 4, 3, 2, 1, 0},
        {2, 5, 8, 1, 4, 7, 0, 3, 6},
        {2, 1, 0, 5, 4, 3, 8, 7, 6},
        {8, 5, 2, 7, 4, 1, 6, 3, 0},
        {6, 7, 8, 3, 4, 5, 0, 1, 2},
        {0, 3, 6, 1, 4, 7, 2, 5, 8},
    },
};

// ============================= Variant Settings =============================

static ConstantReadOnlyString kQuixoRuleChoices[] = {
    "5x5 5-in-a-row",
    "4x4 4-in-a-row",
    "3x3 3-in-a-row",
};

static const GameVariantOption kQuixoRules = {
    .name = "rules",
    .num_choices = sizeof(kQuixoRuleChoices) / sizeof(kQuixoRuleChoices[0]),
    .choices = kQuixoRuleChoices,
};

static GameVariantOption quixo_variant_options[2];
static int quixo_variant_option_selections[2] = {0, 0};  // 5x5
static int curr_variant_idx = 0;
static int side_length = 5;
static int board_size = 25;
static QuixoTier initial_tier = {.unpacked = {0, 0}};

static GameVariant current_variant = {
    .options = quixo_variant_options,
    .selections = quixo_variant_option_selections,
};

// ============================== kQuixoSolverApi ==============================

static Tier QuixoGetInitialTier(void) { return initial_tier.hash; }

static Position QuixoGetInitialPosition(void) { return TwoPieceHashHash(0, 0); }

static int64_t QuixoGetTierSize(Tier tier) {
    QuixoTier t = {.hash = tier};

    return TwoPieceHashGetNumPositions(t.unpacked[0], t.unpacked[1]);
}

static int GenerateMovesInternal(uint64_t board, int turn,
                                 Move moves[static kTierSolverNumMovesMax]) {
    QuixoMove m = kQuixoMoveInit;
    int opp_turn = !turn;
    int ret = 0;
    for (int i = 0; i < kNumMovesPerDir[curr_variant_idx]; ++i) {
        m.unpacked.idx = i;
        uint64_t src_mask = kMoveLeft[curr_variant_idx][i][opp_turn];
        if (!(board & src_mask)) {
            m.unpacked.dir = kLeft;
            moves[ret++] = m.hash;
        }
        src_mask = kMoveRight[curr_variant_idx][i][opp_turn];
        if (!(board & src_mask)) {
            m.unpacked.dir = kRight;
            moves[ret++] = m.hash;
        }
        src_mask = kMoveUp[curr_variant_idx][i][opp_turn];
        if (!(board & src_mask)) {
            m.unpacked.dir = kUp;
            moves[ret++] = m.hash;
        }
        src_mask = kMoveDown[curr_variant_idx][i][opp_turn];
        if (!(board & src_mask)) {
            m.unpacked.dir = kDown;
            moves[ret++] = m.hash;
        }
    }

    return ret;
}

static int QuixoGenerateMoves(TierPosition tier_position,
                              Move moves[static kTierSolverNumMovesMax]) {
    // Unhash
    QuixoTier t = {.hash = tier_position.tier};
    uint64_t board = TwoPieceHashUnhash(tier_position.position, t.unpacked[0],
                                        t.unpacked[1]);
    int turn = TwoPieceHashGetTurn(tier_position.position);

    return GenerateMovesInternal(board, turn, moves);
}

static Value QuixoPrimitive(TierPosition tier_position) {
    // Unhash
    QuixoTier t = {.hash = tier_position.tier};
    uint64_t board = TwoPieceHashUnhash(tier_position.position, t.unpacked[0],
                                        t.unpacked[1]);
    int turn = TwoPieceHashGetTurn(tier_position.position);

    for (int i = 0; i < kNumLines[curr_variant_idx]; ++i) {
        // The current player wins if there is a k in a row of the current
        // player's pieces, regardless of whether there is a k in a row of the
        // opponent's pieces.
        uint64_t line = kLines[curr_variant_idx][turn][i];
        if ((board & line) == line) return kWin;
    }

    for (int i = 0; i < kNumLines[curr_variant_idx]; ++i) {
        // If the current player is not winning but there's a k in a row of the
        // opponent's pieces, then the current player loses.
        uint64_t line = kLines[curr_variant_idx][!turn][i];
        if ((board & line) == line) return kLose;
    }

    // Neither side is winning.
    return kUndecided;
}

static TierPosition DoMoveInternal(QuixoTier t, uint64_t board, int turn,
                                   QuixoMove m) {
    bool flipped = false;
    uint64_t A, B, C;
    switch (m.unpacked.dir) {
        case kLeft:
            A = kMoveLeft[curr_variant_idx][m.unpacked.idx][turn];
            flipped = !(board & A);
            B = kMoveLeft[curr_variant_idx][m.unpacked.idx][3];
            C = kMoveLeft[curr_variant_idx][m.unpacked.idx][2] >> (turn * 32);
            board = (((board & B) << 1) & B) | (board & ~B) | C;
            break;

        case kRight:
            A = kMoveRight[curr_variant_idx][m.unpacked.idx][turn];
            flipped = !(board & A);
            B = kMoveRight[curr_variant_idx][m.unpacked.idx][3];
            C = kMoveRight[curr_variant_idx][m.unpacked.idx][2] >> (turn * 32);
            board = (((board & B) >> 1) & B) | (board & ~B) | C;
            break;

        case kUp:
            A = kMoveUp[curr_variant_idx][m.unpacked.idx][turn];
            flipped = !(board & A);
            B = kMoveUp[curr_variant_idx][m.unpacked.idx][3];
            C = kMoveUp[curr_variant_idx][m.unpacked.idx][2] >> (turn * 32);
            board = (((board & B) << side_length) & B) | (board & ~B) | C;
            break;

        case kDown:
            A = kMoveDown[curr_variant_idx][m.unpacked.idx][turn];
            flipped = !(board & A);
            B = kMoveDown[curr_variant_idx][m.unpacked.idx][3];
            C = kMoveDown[curr_variant_idx][m.unpacked.idx][2] >> (turn * 32);
            board = (((board & B) >> side_length) & B) | (board & ~B) | C;
            break;
    }

    // Adjust tier if source tile is flipped
    t.unpacked[turn] += flipped;

    return (TierPosition){.tier = t.hash,
                          .position = TwoPieceHashHash(board, !turn)};
}

static TierPosition QuixoDoMove(TierPosition tier_position, Move move) {
    QuixoTier t = {.hash = tier_position.tier};
    uint64_t board = TwoPieceHashUnhash(tier_position.position, t.unpacked[0],
                                        t.unpacked[1]);
    int turn = TwoPieceHashGetTurn(tier_position.position);
    QuixoMove m = {.hash = move};

    return DoMoveInternal(t, board, turn, m);
}

// Performs a weak test on the position's legality. Will not misidentify legal
// as illegal, but might misidentify illegal as legal.
// In X's turn, returns illegal if there are no border Os, and vice versa
static bool QuixoIsLegalPosition(TierPosition tier_position) {
    // The initial position is always legal but does not follow the rules
    // below.
    if (tier_position.tier == initial_tier.hash &&
        tier_position.position == 0) {
        return true;
    }

    // Unhash
    QuixoTier t = {.hash = tier_position.tier};
    uint64_t board = TwoPieceHashUnhash(tier_position.position, t.unpacked[0],
                                        t.unpacked[1]);
    int turn = TwoPieceHashGetTurn(tier_position.position);

    return board & kEdges[curr_variant_idx][!turn];
}

static Position QuixoGetCanonicalPosition(TierPosition tier_position) {
    // Unhash
    QuixoTier t = {.hash = tier_position.tier};
    uint64_t board = TwoPieceHashUnhash(tier_position.position, t.unpacked[0],
                                        t.unpacked[1]);
    int turn = TwoPieceHashGetTurn(tier_position.position);

    return TwoPieceHashHash(TwoPieceHashGetCanonicalBoard(board), turn);
}

static int QuixoGetNumberOfCanonicalChildPositions(TierPosition tier_position) {
    // Unhash
    QuixoTier t = {.hash = tier_position.tier};
    uint64_t board = TwoPieceHashUnhash(tier_position.position, t.unpacked[0],
                                        t.unpacked[1]);
    int turn = TwoPieceHashGetTurn(tier_position.position);

    // Generate all moves
    Move moves[kTierSolverNumMovesMax];
    int num_moves = GenerateMovesInternal(board, turn, moves);

    // Collect all unique child positions
    TierPositionHashSet dedup;
    TierPositionHashSetInit(&dedup, 0.5);
    TierPositionHashSetReserve(&dedup, 64);
    for (int i = 0; i < num_moves; ++i) {
        QuixoMove m = {.hash = moves[i]};
        TierPosition child = DoMoveInternal(t, board, turn, m);
        child.position = QuixoGetCanonicalPosition(child);
        if (TierPositionHashSetContains(&dedup, child)) continue;
        TierPositionHashSetAdd(&dedup, child);
    }
    int ret = (int)dedup.size;
    TierPositionHashSetDestroy(&dedup);

    return ret;
}

static int QuixoGetCanonicalChildPositions(
    TierPosition tier_position,
    TierPosition children[static kTierSolverNumChildPositionsMax]) {
    // Unhash
    QuixoTier t = {.hash = tier_position.tier};
    uint64_t board = TwoPieceHashUnhash(tier_position.position, t.unpacked[0],
                                        t.unpacked[1]);
    int turn = TwoPieceHashGetTurn(tier_position.position);

    // Generate all moves
    Move moves[kTierSolverNumMovesMax];
    int num_moves = GenerateMovesInternal(board, turn, moves);

    // Collect all unique child positions
    TierPositionHashSet dedup;
    TierPositionHashSetInit(&dedup, 0.5);
    TierPositionHashSetReserve(&dedup, 64);
    int ret = 0;
    for (int i = 0; i < num_moves; ++i) {
        QuixoMove m = {.hash = moves[i]};
        TierPosition child = DoMoveInternal(t, board, turn, m);
        child.position = QuixoGetCanonicalPosition(child);
        if (TierPositionHashSetContains(&dedup, child)) continue;
        TierPositionHashSetAdd(&dedup, child);
        children[ret++] = child;
    }
    TierPositionHashSetDestroy(&dedup);

    return ret;
}

/**
 * @brief This function tests if a position in the \p child tier whose turn is
 * \p child_turn can be reached from a position in the \p parent tier. The tile
 * flipped, which is reflected by the change in tier, must be consistent with
 * the turn of the child position.
 */
static bool IsCorrectFlipping(QuixoTier child, QuixoTier parent,
                              int child_turn) {
    return (child.hash == parent.hash) ||
           !((child.unpacked[0] == parent.unpacked[0] + 1) ^ child_turn);
}

static int QuixoGetCanonicalParentPositions(
    TierPosition tier_position, Tier parent_tier,
    Position parents[static kTierSolverNumParentPositionsMax]) {
    // Unhash
    QuixoTier child_t = {.hash = tier_position.tier};
    QuixoTier parent_t = {.hash = parent_tier};
    int turn = TwoPieceHashGetTurn(tier_position.position);
    if (!IsCorrectFlipping(child_t, parent_t, turn)) return 0;

    uint64_t board = TwoPieceHashUnhash(
        tier_position.position, child_t.unpacked[0], child_t.unpacked[1]);
    int opp_turn = !turn;
    uint64_t B, C;
    bool same_tier = (child_t.hash == parent_t.hash);
    PositionHashSet dedup;
    PositionHashSetInit(&dedup, 0.5);
    PositionHashSetReserve(&dedup, 128);
    int ret = 0;
    for (int i = 0; i < kNumMovesPerDir[curr_variant_idx]; ++i) {
        // Revert a left shifting move
        uint64_t dest_mask =
            kMoveLeft[curr_variant_idx][i][2] >> (opp_turn * 32);
        if (board & dest_mask) {
            B = kMoveLeft[curr_variant_idx][i][3];
            C = kMoveLeft[curr_variant_idx][i][opp_turn] * same_tier;
            uint64_t new_board = (((board & B) >> 1) & B) | (board & ~B) | C;
            new_board = TwoPieceHashGetCanonicalBoard(new_board);
            Position new_pos = TwoPieceHashHash(new_board, opp_turn);
            if (!PositionHashSetContains(&dedup, new_pos)) {
                PositionHashSetAdd(&dedup, new_pos);
                parents[ret++] = new_pos;
            }
        }

        // Revert a right shifting move
        dest_mask = kMoveRight[curr_variant_idx][i][2] >> (opp_turn * 32);
        if (board & dest_mask) {
            B = kMoveRight[curr_variant_idx][i][3];
            C = kMoveRight[curr_variant_idx][i][opp_turn] * same_tier;
            uint64_t new_board = (((board & B) << 1) & B) | (board & ~B) | C;
            new_board = TwoPieceHashGetCanonicalBoard(new_board);
            Position new_pos = TwoPieceHashHash(new_board, opp_turn);
            if (!PositionHashSetContains(&dedup, new_pos)) {
                PositionHashSetAdd(&dedup, new_pos);
                parents[ret++] = new_pos;
            }
        }

        // Revert a upward shifting move
        dest_mask = kMoveUp[curr_variant_idx][i][2] >> (opp_turn * 32);
        if (board & dest_mask) {
            B = kMoveUp[curr_variant_idx][i][3];
            C = kMoveUp[curr_variant_idx][i][opp_turn] * same_tier;
            uint64_t new_board =
                (((board & B) >> side_length) & B) | (board & ~B) | C;
            new_board = TwoPieceHashGetCanonicalBoard(new_board);
            Position new_pos = TwoPieceHashHash(new_board, opp_turn);
            if (!PositionHashSetContains(&dedup, new_pos)) {
                PositionHashSetAdd(&dedup, new_pos);
                parents[ret++] = new_pos;
            }
        }

        // Revert a downward shifting move
        dest_mask = kMoveDown[curr_variant_idx][i][2] >> (opp_turn * 32);
        if (board & dest_mask) {
            B = kMoveDown[curr_variant_idx][i][3];
            C = kMoveDown[curr_variant_idx][i][opp_turn] * same_tier;
            uint64_t new_board =
                (((board & B) << side_length) & B) | (board & ~B) | C;
            new_board = TwoPieceHashGetCanonicalBoard(new_board);
            Position new_pos = TwoPieceHashHash(new_board, opp_turn);
            if (!PositionHashSetContains(&dedup, new_pos)) {
                PositionHashSetAdd(&dedup, new_pos);
                parents[ret++] = new_pos;
            }
        }
    }
    PositionHashSetDestroy(&dedup);

    return ret;
}

static int8_t GetNumBlanks(QuixoTier t) {
    return board_size - t.unpacked[0] - t.unpacked[1];
}

static int QuixoGetChildTiers(
    Tier tier, Tier children[static kTierSolverNumChildTiersMax]) {
    //
    QuixoTier t = {.hash = tier};
    int ret = 0;
    if (GetNumBlanks(t) > 0) {
        ++t.unpacked[0];
        children[ret++] = t.hash;
        --t.unpacked[0];
        ++t.unpacked[1];
        children[ret++] = t.hash;
    }  // else: no children for tiers with no blanks

    return ret;
}

static int QuixoGetTierName(Tier tier,
                            char name[static kDbFileNameLengthMax + 1]) {
    QuixoTier t = {.hash = tier};
    sprintf(name, "%dBlank_%dX_%dO", GetNumBlanks(t), t.unpacked[0],
            t.unpacked[1]);

    return kNoError;
}

static const TierSolverApi kQuixoSolverApi = {
    .GetInitialTier = QuixoGetInitialTier,
    .GetInitialPosition = QuixoGetInitialPosition,
    .GetTierSize = QuixoGetTierSize,

    .GenerateMoves = QuixoGenerateMoves,
    .Primitive = QuixoPrimitive,
    .DoMove = QuixoDoMove,
    .IsLegalPosition = QuixoIsLegalPosition,
    .GetCanonicalPosition = QuixoGetCanonicalPosition,
    .GetNumberOfCanonicalChildPositions =
        QuixoGetNumberOfCanonicalChildPositions,
    .GetCanonicalChildPositions = QuixoGetCanonicalChildPositions,
    .GetCanonicalParentPositions = QuixoGetCanonicalParentPositions,

    .GetChildTiers = QuixoGetChildTiers,
    .GetTierName = QuixoGetTierName,
};

// ============================= kQuixoGameplayApi =============================

MoveArray QuixoGenerateMovesGameplay(TierPosition tier_position) {
    Move moves[kTierSolverNumMovesMax];
    int num_moves = QuixoGenerateMoves(tier_position, moves);
    MoveArray ret;
    MoveArrayInit(&ret);
    for (int i = 0; i < num_moves; ++i) {
        MoveArrayAppend(&ret, moves[i]);
    }

    return ret;
}

static void BoardToStr(uint64_t board, char *buffer) {
    for (int i = 0; i < board_size; ++i) {
        uint64_t x_mask = (1ULL << (32 + board_size - i - 1));
        uint64_t o_mask = (1ULL << (board_size - i - 1));
        bool is_x = board & x_mask;
        bool is_o = board & o_mask;
        buffer[i] = is_x ? 'X' : is_o ? 'O' : '-';
    }

    buffer[board_size] = '\0';
}

static int QuixoTierPositionToString(TierPosition tier_position, char *buffer) {
    // Unhash
    QuixoTier t = {.hash = tier_position.tier};
    uint64_t board = TwoPieceHashUnhash(tier_position.position, t.unpacked[0],
                                        t.unpacked[1]);
    char board_str[kBoardSizeMax + 1];
    BoardToStr(board, board_str);
    int offset = 0;
    for (int r = 0; r < side_length; ++r) {
        if (r == (side_length - 1) / 2) {
            offset += sprintf(buffer + offset, "LEGEND: ");
        } else {
            offset += sprintf(buffer + offset, "        ");
        }

        for (int c = 0; c < side_length; ++c) {
            int index = r * side_length + c + 1;
            if (c == 0) {
                offset += sprintf(buffer + offset, "(%2d", index);
            } else {
                offset += sprintf(buffer + offset, " %2d", index);
            }
        }
        offset += sprintf(buffer + offset, ")");

        if (r == (side_length - 1) / 2) {
            offset += sprintf(buffer + offset, "    BOARD: : ");
        } else {
            offset += sprintf(buffer + offset, "           : ");
        }

        for (int c = 0; c < side_length; ++c) {
            int index = r * side_length + c;
            offset += sprintf(buffer + offset, "%c ", board_str[index]);
        }
        offset += sprintf(buffer + offset, "\n");
    }

    return kNoError;
}

static int QuixoMoveToString(Move move, char *buffer) {
    QuixoMove m = {.hash = move};
    sprintf(
        buffer, "%d %c",
        kDirIndexToSrc[curr_variant_idx][m.unpacked.dir][m.unpacked.idx] + 1,
        kDirToChar[m.unpacked.dir]);

    return kNoError;
}

static bool QuixoIsValidMoveString(ReadOnlyString move_string) {
    // Strings of format "source direction". Ex: "6 R", "3 D".
    // Only "1" - "<board_size>" are valid sources.
    if (move_string == NULL) return false;
    if (strlen(move_string) < 3 || strlen(move_string) > 4) return false;

    // Make a copy of move_string bc strtok_r mutates the original move_string
    char copy_string[6];
    strcpy(copy_string, move_string);

    char *saveptr;
    char *token = strtok_r(copy_string, " ", &saveptr);
    if (token == NULL) return false;
    int src = atoi(token);

    token = strtok_r(NULL, " ", &saveptr);
    if (token == NULL) return false;

    if (src < 1) return false;
    if (src > board_size) return false;
    if (strlen(token) != 1) return false;
    if (token[0] != 'L' && token[0] != 'R' && token[0] != 'U' &&
        token[0] != 'D') {
        return false;
    }

    return true;
}

static Move QuixoStringToMove(ReadOnlyString move_string) {
    char copy_string[6];
    strcpy(copy_string, move_string);
    QuixoMove m = kQuixoMoveInit;
    char *saveptr;
    char *token = strtok_r(copy_string, " ", &saveptr);
    int src = atoi(token) - 1;

    token = strtok_r(NULL, " ", &saveptr);
    if (token[0] == 'R') {
        m.unpacked.dir = kLeft;
    } else if (token[0] == 'L') {
        m.unpacked.dir = kRight;
    } else if (token[0] == 'D') {
        m.unpacked.dir = kUp;
    } else {
        m.unpacked.dir = kDown;
    }
    m.unpacked.idx = kDirSrcToIndex[curr_variant_idx][m.unpacked.dir][src];

    return m.hash;
}

static const GameplayApiCommon QuixoGameplayApiCommon = {
    .GetInitialPosition = QuixoGetInitialPosition,
    .position_string_length_max = 512,

    .move_string_length_max = 4,
    .MoveToString = QuixoMoveToString,

    .IsValidMoveString = QuixoIsValidMoveString,
    .StringToMove = QuixoStringToMove,
};

static const GameplayApiTier QuixoGameplayApiTier = {
    .GetInitialTier = QuixoGetInitialTier,

    .TierPositionToString = QuixoTierPositionToString,

    .GenerateMoves = QuixoGenerateMovesGameplay,
    .DoMove = QuixoDoMove,
    .Primitive = QuixoPrimitive,
};

static const GameplayApi kQuixoGameplayApi = {
    .common = &QuixoGameplayApiCommon,
    .tier = &QuixoGameplayApiTier,
};

// ========================== QuixoGetCurrentVariant ==========================

static const GameVariant *QuixoGetCurrentVariant(void) {
    return &current_variant;
}

// =========================== QuixoSetVariantOption ===========================

static int QuixoInitVariant(int selection) {
    side_length = 5 - selection;
    board_size = side_length * side_length;
    const int *symmetry_matrix[8];
    for (int i = 0; i < 8; ++i) {
        symmetry_matrix[i] = kSymmetryMatrix[curr_variant_idx][i];
    }
    int ret = TwoPieceHashInit(side_length, side_length, symmetry_matrix, 8);
    if (ret != kNoError) return ret;

    return kNoError;
}

static int QuixoSetVariantOption(int option, int selection) {
    // There is only one option in the game, and the selection must be between 0
    // to num_choices - 1 inclusive.
    if (option != 0 || selection < 0 || selection >= kQuixoRules.num_choices) {
        return kRuntimeError;
    }

    curr_variant_idx = quixo_variant_option_selections[0] = selection;

    return QuixoInitVariant(selection);
}

// ================================= QuixoInit =================================

static int QuixoInit(void *aux) {
    (void)aux;  // Unused.
    quixo_variant_options[0] = kQuixoRules;

    return QuixoSetVariantOption(0, 0);  // Initialize the default variant.
}

// =============================== QuixoFinalize ===============================

static int QuixoFinalize(void) {
    TwoPieceHashFinalize();

    return kNoError;
}

// ================================== kQuixo ==================================

const Game kQuixo = {
    .name = "quixo",
    .formal_name = "Quixo",
    .solver = &kTierSolver,
    .solver_api = &kQuixoSolverApi,
    .gameplay_api = &kQuixoGameplayApi,
    .uwapi = NULL,  // TODO

    .Init = QuixoInit,
    .Finalize = QuixoFinalize,

    .GetCurrentVariant = QuixoGetCurrentVariant,
    .SetVariantOption = QuixoSetVariantOption,
};
