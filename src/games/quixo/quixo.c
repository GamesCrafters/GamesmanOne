/**
 * @file quixo.c
 * @author François Bonnet: original published version,
 *         arXiv:2007.15895v1
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @author Maria Rufova
 * @author Benji Xu
 * @author Angela He
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Quixo implementation.
 * @version 2.1.0
 * @date 2025-03-15
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

#include "games/quixo/quixo.h"

#include <assert.h>     // assert
#include <stddef.h>     // NULL
#include <stdio.h>      // sprintf
#include <stdlib.h>     // atoi
#include <string.h>     // strtok_r, strlen, strcpy
#include <x86intrin.h>  // __m128i

#include "core/constants.h"
#include "core/hash/x86_simd_two_piece.h"
#include "core/solvers/tier_solver/tier_solver.h"
#include "core/types/gamesman_types.h"

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

static const uint64_t kLines[kNumVariants][kNumLinesMax] = {
    // =========
    // == 5x5 ==
    // =========
    {0x1f00000000, 0x1f000000, 0x1f0000, 0x1f00, 0x1f, 0x1010101010,
     0x808080808, 0x404040404, 0x202020202, 0x101010101, 0x1008040201,
     0x102040810},
    // =========
    // == 4x4 ==
    // =========
    {0xf000000, 0xf0000, 0xf00, 0xf, 0x8080808, 0x4040404, 0x2020202, 0x1010101,
     0x8040201, 0x1020408},
    // =========
    // == 3x3 ==
    // =========
    {0x70000, 0x700, 0x7,

     0x40404, 0x20202, 0x10101,

     0x40201, 0x10204},
};

static const uint64_t kEdges[kNumVariants] = {
    0x1f1111111f,
    0xf09090f,
    0x70507,
};

static const uint64_t kMoveLeft[kNumVariants][kNumMovesPerDirMax][3] = {
    // =========
    // == 5x5 ==
    // =========
    {{0x1000000000, 0x100000000, 0x1f00000000},
     {0x800000000, 0x100000000, 0xf00000000},
     {0x400000000, 0x100000000, 0x700000000},
     {0x200000000, 0x100000000, 0x300000000},
     {0x10000000, 0x1000000, 0x1f000000},
     {0x100000, 0x10000, 0x1f0000},
     {0x1000, 0x100, 0x1f00},
     {0x10, 0x1, 0x1f},
     {0x8, 0x1, 0xf},
     {0x4, 0x1, 0x7},
     {
         0x2,
         0x1,
         0x3,
     }},
    // =========
    // == 4x4 ==
    // =========
    {{0x8000000, 0x1000000, 0xf000000},
     {0x4000000, 0x1000000, 0x7000000},
     {0x2000000, 0x1000000, 0x3000000},
     {0x80000, 0x10000, 0xf0000},
     {0x800, 0x100, 0xf00},
     {0x8, 0x1, 0xf},
     {0x4, 0x1, 0x7},
     {
         0x2,
         0x1,
         0x3,
     }},
    // =========
    // == 3x3 ==
    // =========
    {{0x40000, 0x10000, 0x70000},
     {0x20000, 0x10000, 0x30000},
     {0x400, 0x100, 0x700},
     {0x4, 0x1, 0x7},
     {
         0x2,
         0x1,
         0x3,
     }},
};

static const uint64_t kMoveRight[kNumVariants][kNumMovesPerDirMax][3] = {
    // =========
    // == 5x5 ==
    // =========
    {{0x800000000, 0x1000000000, 0x1800000000},
     {0x400000000, 0x1000000000, 0x1c00000000},
     {0x200000000, 0x1000000000, 0x1e00000000},
     {0x100000000, 0x1000000000, 0x1f00000000},
     {0x1000000, 0x10000000, 0x1f000000},
     {0x10000, 0x100000, 0x1f0000},
     {0x100, 0x1000, 0x1f00},
     {0x8, 0x10, 0x18},
     {0x4, 0x10, 0x1c},
     {0x2, 0x10, 0x1e},
     {
         0x1,
         0x10,
         0x1f,
     }},
    // =========
    // == 4x4 ==
    // =========
    {{0x4000000, 0x8000000, 0xc000000},
     {0x2000000, 0x8000000, 0xe000000},
     {0x1000000, 0x8000000, 0xf000000},
     {0x10000, 0x80000, 0xf0000},
     {0x100, 0x800, 0xf00},
     {0x4, 0x8, 0xc},
     {0x2, 0x8, 0xe},
     {
         0x1,
         0x8,
         0xf,
     }},
    // =========
    // == 3x3 ==
    // =========
    {{0x20000, 0x40000, 0x60000},
     {0x10000, 0x40000, 0x70000},
     {0x100, 0x400, 0x700},
     {0x2, 0x4, 0x6},
     {
         0x1,
         0x4,
         0x7,
     }},
};

static const uint64_t kMoveUp[kNumVariants][kNumMovesPerDirMax][3] = {
    // =========
    // == 5x5 ==
    // =========
    {{0x1000000000, 0x10, 0x1010101010},
     {0x800000000, 0x8, 0x808080808},
     {0x400000000, 0x4, 0x404040404},
     {0x200000000, 0x2, 0x202020202},
     {0x100000000, 0x1, 0x101010101},
     {0x10000000, 0x10, 0x10101010},
     {0x1000000, 0x1, 0x1010101},
     {0x100000, 0x10, 0x101010},
     {0x10000, 0x1, 0x10101},
     {0x1000, 0x10, 0x1010},
     {
         0x100,
         0x1,
         0x101,
     }},
    // =========
    // == 4x4 ==
    // =========
    {{0x8000000, 0x8, 0x8080808},
     {0x4000000, 0x4, 0x4040404},
     {0x2000000, 0x2, 0x2020202},
     {0x1000000, 0x1, 0x1010101},
     {0x80000, 0x8, 0x80808},
     {0x10000, 0x1, 0x10101},
     {0x800, 0x8, 0x808},
     {
         0x100,
         0x1,
         0x101,
     }},
    // =========
    // == 3x3 ==
    // =========
    {{0x40000, 0x4, 0x40404},
     {0x20000, 0x2, 0x20202},
     {0x10000, 0x1, 0x10101},
     {0x400, 0x4, 0x404},
     {
         0x100,
         0x1,
         0x101,
     }},
};

static const uint64_t kMoveDown[kNumVariants][kNumMovesPerDirMax][3] = {
    // =========
    // == 5x5 ==
    // =========
    {{0x10000000, 0x1000000000, 0x1010000000},
     {0x1000000, 0x100000000, 0x101000000},
     {0x100000, 0x1000000000, 0x1010100000},
     {0x10000, 0x100000000, 0x101010000},
     {0x1000, 0x1000000000, 0x1010101000},
     {0x100, 0x100000000, 0x101010100},
     {0x10, 0x1000000000, 0x1010101010},
     {0x8, 0x800000000, 0x808080808},
     {0x4, 0x400000000, 0x404040404},
     {0x2, 0x200000000, 0x202020202},
     {
         0x1,
         0x100000000,
         0x101010101,
     }},
    // =========
    // == 4x4 ==
    // =========
    {{0x80000, 0x8000000, 0x8080000},
     {0x10000, 0x1000000, 0x1010000},
     {0x800, 0x8000000, 0x8080800},
     {0x100, 0x1000000, 0x1010100},
     {0x8, 0x8000000, 0x8080808},
     {0x4, 0x4000000, 0x4040404},
     {0x2, 0x2000000, 0x2020202},
     {
         0x1,
         0x1000000,
         0x1010101,
     }},
    // =========
    // == 3x3 ==
    // =========
    {{0x400, 0x40000, 0x40400},
     {0x100, 0x10000, 0x10100},
     {0x4, 0x40000, 0x40404},
     {0x2, 0x20000, 0x20202},
     {
         0x1,
         0x10000,
         0x10101,
     }},
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

static Position QuixoGetInitialPosition(void) {
    __m128i board = _mm_set1_epi64x(0);

    return X86SimdTwoPieceHashHash(board, 0);
}

static int64_t QuixoGetTierSize(Tier tier) {
    QuixoTier t = {.hash = tier};

    return X86SimdTwoPieceHashGetNumPositions(t.unpacked[0], t.unpacked[1]);
}

static int GenerateMovesInternal(uint64_t patterns[2], int turn,
                                 Move moves[static kTierSolverNumMovesMax]) {
    QuixoMove m = kQuixoMoveInit;
    int opp_turn = !turn;
    int ret = 0;
    for (int i = 0; i < kNumMovesPerDir[curr_variant_idx]; ++i) {
        m.unpacked.idx = i;
        uint64_t src_mask = kMoveLeft[curr_variant_idx][i][0];
        if (!(patterns[opp_turn] & src_mask)) {
            m.unpacked.dir = kLeft;
            moves[ret++] = m.hash;
        }
        src_mask = kMoveRight[curr_variant_idx][i][0];
        if (!(patterns[opp_turn] & src_mask)) {
            m.unpacked.dir = kRight;
            moves[ret++] = m.hash;
        }
        src_mask = kMoveUp[curr_variant_idx][i][0];
        if (!(patterns[opp_turn] & src_mask)) {
            m.unpacked.dir = kUp;
            moves[ret++] = m.hash;
        }
        src_mask = kMoveDown[curr_variant_idx][i][0];
        if (!(patterns[opp_turn] & src_mask)) {
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
    uint64_t patterns[2];
    X86SimdTwoPieceHashUnhashMem(tier_position.position, t.unpacked[0],
                                 t.unpacked[1], patterns);
    int turn = X86SimdTwoPieceHashGetTurn(tier_position.position);

    return GenerateMovesInternal(patterns, turn, moves);
}

static Value QuixoPrimitive(TierPosition tier_position) {
    // Unhash
    QuixoTier t = {.hash = tier_position.tier};
    uint64_t patterns[2];
    X86SimdTwoPieceHashUnhashMem(tier_position.position, t.unpacked[0],
                                 t.unpacked[1], patterns);
    int turn = X86SimdTwoPieceHashGetTurn(tier_position.position);
    int opp_turn = !turn;

    for (int i = 0; i < kNumLines[curr_variant_idx]; ++i) {
        // The current player wins if there is a k in a row of the current
        // player's pieces, regardless of whether there is a k in a row of the
        // opponent's pieces.
        uint64_t line = kLines[curr_variant_idx][i];
        if ((patterns[turn] & line) == line) return kWin;
    }

    for (int i = 0; i < kNumLines[curr_variant_idx]; ++i) {
        // If the current player is not winning but there's a k in a row of the
        // opponent's pieces, then the current player loses.
        uint64_t line = kLines[curr_variant_idx][i];
        if ((patterns[opp_turn] & line) == line) return kLose;
    }

    // Neither side is winning.
    return kUndecided;
}

static inline __m128i DoMoveShiftLeft(__m128i board, uint64_t shift,
                                      uint64_t dest, int turn) {
    __m128i shift_mask = _mm_set1_epi64x(shift);
    __m128i dest_mask = _mm_set_epi64x(dest * turn, dest * (!turn));
    __m128i seg1 = _mm_and_si128(
        _mm_slli_epi64(_mm_and_si128(board, shift_mask), 1), shift_mask);
    __m128i seg2 = _mm_andnot_si128(shift_mask, board);

    return _mm_or_si128(_mm_or_si128(seg1, seg2), dest_mask);
}

static inline __m128i DoMoveShiftRight(__m128i board, uint64_t shift,
                                       uint64_t dest, int turn) {
    __m128i shift_mask = _mm_set1_epi64x(shift);
    __m128i dest_mask = _mm_set_epi64x(dest * turn, dest * (!turn));
    __m128i seg1 = _mm_and_si128(
        _mm_srli_epi64(_mm_and_si128(board, shift_mask), 1), shift_mask);
    __m128i seg2 = _mm_andnot_si128(shift_mask, board);

    return _mm_or_si128(_mm_or_si128(seg1, seg2), dest_mask);
}

static inline __m128i DoMoveShiftUp(__m128i board, uint64_t shift,
                                    uint64_t dest, int turn) {
    __m128i shift_mask = _mm_set1_epi64x(shift);
    __m128i dest_mask = _mm_set_epi64x(dest * turn, dest * (!turn));
    __m128i seg1 = _mm_and_si128(
        _mm_slli_epi64(_mm_and_si128(board, shift_mask), 8), shift_mask);
    __m128i seg2 = _mm_andnot_si128(shift_mask, board);

    return _mm_or_si128(_mm_or_si128(seg1, seg2), dest_mask);
}

static inline __m128i DoMoveShiftDown(__m128i board, uint64_t shift,
                                      uint64_t dest, int turn) {
    __m128i shift_mask = _mm_set1_epi64x(shift);
    __m128i dest_mask = _mm_set_epi64x(dest * turn, dest * (!turn));
    __m128i seg1 = _mm_and_si128(
        _mm_srli_epi64(_mm_and_si128(board, shift_mask), 8), shift_mask);
    __m128i seg2 = _mm_andnot_si128(shift_mask, board);

    return _mm_or_si128(_mm_or_si128(seg1, seg2), dest_mask);
}

static TierPosition DoMoveInternal(QuixoTier t, __m128i board,
                                   const uint64_t patterns[2], int turn,
                                   QuixoMove m) {
    uint64_t src = 0, shift, dest;
    switch (m.unpacked.dir) {
        case kLeft:
            src = kMoveLeft[curr_variant_idx][m.unpacked.idx][0];
            dest = kMoveLeft[curr_variant_idx][m.unpacked.idx][1];
            shift = kMoveLeft[curr_variant_idx][m.unpacked.idx][2];
            board = DoMoveShiftLeft(board, shift, dest, turn);
            break;

        case kRight:
            src = kMoveRight[curr_variant_idx][m.unpacked.idx][0];
            dest = kMoveRight[curr_variant_idx][m.unpacked.idx][1];
            shift = kMoveRight[curr_variant_idx][m.unpacked.idx][2];
            board = DoMoveShiftRight(board, shift, dest, turn);
            break;

        case kUp:
            src = kMoveUp[curr_variant_idx][m.unpacked.idx][0];
            dest = kMoveUp[curr_variant_idx][m.unpacked.idx][1];
            shift = kMoveUp[curr_variant_idx][m.unpacked.idx][2];
            board = DoMoveShiftUp(board, shift, dest, turn);
            break;

        case kDown:
            src = kMoveDown[curr_variant_idx][m.unpacked.idx][0];
            dest = kMoveDown[curr_variant_idx][m.unpacked.idx][1];
            shift = kMoveDown[curr_variant_idx][m.unpacked.idx][2];
            board = DoMoveShiftDown(board, shift, dest, turn);
            break;
    }

    // Adjust tier if source tile is flipped
    t.unpacked[turn] += !(patterns[turn] & src);

    return (TierPosition){.tier = t.hash,
                          .position = X86SimdTwoPieceHashHash(board, !turn)};
}

static TierPosition QuixoDoMove(TierPosition tier_position, Move move) {
    QuixoTier t = {.hash = tier_position.tier};
    __m128i board = X86SimdTwoPieceHashUnhash(tier_position.position,
                                              t.unpacked[0], t.unpacked[1]);
    int turn = X86SimdTwoPieceHashGetTurn(tier_position.position);
    QuixoMove m = {.hash = move};
    __attribute__((aligned(16))) uint64_t patterns[2];
    _mm_store_si128((__m128i *)patterns, board);

    return DoMoveInternal(t, board, patterns, turn, m);
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
    uint64_t patterns[2];
    X86SimdTwoPieceHashUnhashMem(tier_position.position, t.unpacked[0],
                                 t.unpacked[1], patterns);
    int turn = X86SimdTwoPieceHashGetTurn(tier_position.position);

    // Non-zero if there is at least one opponent's piece on the edges.
    return patterns[!turn] & kEdges[curr_variant_idx];
}

static inline __m128i GetCanonicalBoard(__m128i board) {
    // 8 symmetries
    __m128i min_board = board;
    board = X86SimdTwoPieceHashFlipVertical(board, side_length);
    if (X86SimdTwoPieceHashBoardLessThan(board, min_board)) min_board = board;
    board = X86SimdTwoPieceHashFlipDiag(board);
    if (X86SimdTwoPieceHashBoardLessThan(board, min_board)) min_board = board;
    board = X86SimdTwoPieceHashFlipVertical(board, side_length);
    if (X86SimdTwoPieceHashBoardLessThan(board, min_board)) min_board = board;
    board = X86SimdTwoPieceHashFlipDiag(board);
    if (X86SimdTwoPieceHashBoardLessThan(board, min_board)) min_board = board;
    board = X86SimdTwoPieceHashFlipVertical(board, side_length);
    if (X86SimdTwoPieceHashBoardLessThan(board, min_board)) min_board = board;
    board = X86SimdTwoPieceHashFlipDiag(board);
    if (X86SimdTwoPieceHashBoardLessThan(board, min_board)) min_board = board;
    board = X86SimdTwoPieceHashFlipVertical(board, side_length);
    if (X86SimdTwoPieceHashBoardLessThan(board, min_board)) min_board = board;

    return min_board;
}

static Position QuixoGetCanonicalPosition(TierPosition tier_position) {
    // Unhash
    QuixoTier t = {.hash = tier_position.tier};
    __m128i board = X86SimdTwoPieceHashUnhash(tier_position.position,
                                              t.unpacked[0], t.unpacked[1]);
    int turn = X86SimdTwoPieceHashGetTurn(tier_position.position);

    return X86SimdTwoPieceHashHash(GetCanonicalBoard(board), turn);
}

static int QuixoGetNumberOfCanonicalChildPositions(TierPosition tier_position) {
    // Unhash
    QuixoTier t = {.hash = tier_position.tier};
    __m128i board = X86SimdTwoPieceHashUnhash(tier_position.position,
                                              t.unpacked[0], t.unpacked[1]);
    int turn = X86SimdTwoPieceHashGetTurn(tier_position.position);
    __attribute__((aligned(16))) uint64_t patterns[2];
    _mm_store_si128((__m128i *)patterns, board);

    // Generate all moves
    Move moves[kTierSolverNumMovesMax];
    int num_moves = GenerateMovesInternal(patterns, turn, moves);

    // Collect all unique child positions
    TierPositionHashSet dedup;
    TierPositionHashSetInit(&dedup, 0.5);
    TierPositionHashSetReserve(&dedup, 64);
    for (int i = 0; i < num_moves; ++i) {
        QuixoMove m = {.hash = moves[i]};
        TierPosition child = DoMoveInternal(t, board, patterns, turn, m);
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
    __m128i board = X86SimdTwoPieceHashUnhash(tier_position.position,
                                              t.unpacked[0], t.unpacked[1]);
    int turn = X86SimdTwoPieceHashGetTurn(tier_position.position);
    __attribute__((aligned(16))) uint64_t patterns[2];
    _mm_store_si128((__m128i *)patterns, board);

    // Generate all moves
    Move moves[kTierSolverNumMovesMax];
    int num_moves = GenerateMovesInternal(patterns, turn, moves);

    // Collect all unique child positions
    TierPositionHashSet dedup;
    TierPositionHashSetInit(&dedup, 0.5);
    TierPositionHashSetReserve(&dedup, 64);
    int ret = 0;
    for (int i = 0; i < num_moves; ++i) {
        QuixoMove m = {.hash = moves[i]};
        TierPosition child = DoMoveInternal(t, board, patterns, turn, m);
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
    int turn = X86SimdTwoPieceHashGetTurn(tier_position.position);
    if (!IsCorrectFlipping(child_t, parent_t, turn)) return 0;

    __m128i board = X86SimdTwoPieceHashUnhash(
        tier_position.position, child_t.unpacked[0], child_t.unpacked[1]);
    __attribute__((aligned(16))) uint64_t patterns[2];
    _mm_store_si128((__m128i *)patterns, board);
    int opp_turn = !turn;
    uint64_t shift, src;
    bool same_tier = (child_t.hash == parent_t.hash);
    PositionHashSet dedup;
    PositionHashSetInit(&dedup, 0.5);
    PositionHashSetReserve(&dedup, 128);
    int ret = 0;
    for (int i = 0; i < kNumMovesPerDir[curr_variant_idx]; ++i) {
        // Revert a left shifting move
        uint64_t dest = kMoveLeft[curr_variant_idx][i][1];
        if (patterns[opp_turn] & dest) {
            shift = kMoveLeft[curr_variant_idx][i][2];
            src = kMoveLeft[curr_variant_idx][i][0] * same_tier;
            __m128i new_board = DoMoveShiftRight(board, shift, src, opp_turn);
            new_board = GetCanonicalBoard(new_board);
            Position new_pos = X86SimdTwoPieceHashHash(new_board, opp_turn);
            if (!PositionHashSetContains(&dedup, new_pos)) {
                PositionHashSetAdd(&dedup, new_pos);
                parents[ret++] = new_pos;
            }
        }

        // Revert a right shifting move
        dest = kMoveRight[curr_variant_idx][i][1];
        if (patterns[opp_turn] & dest) {
            shift = kMoveRight[curr_variant_idx][i][2];
            src = kMoveRight[curr_variant_idx][i][0] * same_tier;
            __m128i new_board = DoMoveShiftLeft(board, shift, src, opp_turn);
            new_board = GetCanonicalBoard(new_board);
            Position new_pos = X86SimdTwoPieceHashHash(new_board, opp_turn);
            if (!PositionHashSetContains(&dedup, new_pos)) {
                PositionHashSetAdd(&dedup, new_pos);
                parents[ret++] = new_pos;
            }
        }

        // Revert a upward shifting move
        dest = kMoveUp[curr_variant_idx][i][1];
        if (patterns[opp_turn] & dest) {
            shift = kMoveUp[curr_variant_idx][i][2];
            src = kMoveUp[curr_variant_idx][i][0] * same_tier;
            __m128i new_board = DoMoveShiftDown(board, shift, src, opp_turn);
            new_board = GetCanonicalBoard(new_board);
            Position new_pos = X86SimdTwoPieceHashHash(new_board, opp_turn);
            if (!PositionHashSetContains(&dedup, new_pos)) {
                PositionHashSetAdd(&dedup, new_pos);
                parents[ret++] = new_pos;
            }
        }

        // Revert a downward shifting move
        dest = kMoveDown[curr_variant_idx][i][1];
        if (patterns[opp_turn] & dest) {
            shift = kMoveDown[curr_variant_idx][i][2];
            src = kMoveDown[curr_variant_idx][i][0] * same_tier;
            __m128i new_board = DoMoveShiftUp(board, shift, src, opp_turn);
            new_board = GetCanonicalBoard(new_board);
            Position new_pos = X86SimdTwoPieceHashHash(new_board, opp_turn);
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

static void BoardToStr(__m128i board, char *buffer) {
    uint64_t x_pattern = _mm_extract_epi64(board, 0);
    uint64_t o_pattern = _mm_extract_epi64(board, 1);
    for (int i = 0; i < side_length; ++i) {
        for (int j = 0; j < side_length; ++j) {
            uint64_t mask = (1ULL << (8 * i + j));
            bool is_x = x_pattern & mask;
            bool is_o = o_pattern & mask;
            int buff_index = board_size - (i * side_length + j) - 1;
            buffer[buff_index] = is_x ? 'X' : is_o ? 'O' : '-';
        }
    }

    buffer[board_size] = '\0';
}

static int QuixoTierPositionToString(TierPosition tier_position, char *buffer) {
    // Unhash
    QuixoTier t = {.hash = tier_position.tier};
    __m128i board = X86SimdTwoPieceHashUnhash(tier_position.position,
                                              t.unpacked[0], t.unpacked[1]);
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

    return X86SimdTwoPieceHashInit(side_length, side_length);
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
    X86SimdTwoPieceHashFinalize();

    return kNoError;
}

// ================================ kQuixoUwapi ================================

static bool IsValidPieceConfig(int num_blanks, int num_x, int num_o) {
    if (num_blanks < 0 || num_x < 0 || num_o < 0) return false;
    if (num_blanks + num_x + num_o != board_size) return false;

    return true;
}

static __m128i StrToBoard(const char *board) {
    // Translate board
    uint64_t x_pattern = 0, o_pattern = 0;
    for (int i = 0; i < side_length; ++i) {
        for (int j = 0; j < side_length; ++j) {
            uint64_t mask = (1ULL << (8 * i + j));
            int buff_index = board_size - (i * side_length + j) - 1;
            char piece = board[buff_index];
            if (piece == 'X') {
                x_pattern |= mask;
            } else if (piece == 'O') {
                o_pattern |= mask;
            }
        }
    }

    return _mm_set_epi64x(o_pattern, x_pattern);
}

static bool QuixoIsLegalFormalPosition(ReadOnlyString formal_position) {
    // Check if FORMAL_POSITION is of the correct format.
    if (formal_position[0] != '1' && formal_position[0] != '2') return false;
    if (formal_position[1] != '_') return false;
    if (strlen(formal_position) != (size_t)(2 + board_size)) return false;

    // Check if the board corresponds to a valid tier.
    ConstantReadOnlyString board = formal_position + 2;
    int num_blanks = 0, num_x = 0, num_o = 0;
    for (int i = 0; i < board_size; ++i) {
        if (board[i] != '-' && board[i] != 'X' && board[i] != 'O') return false;
        num_blanks += (board[i] == '-');
        num_x += (board[i] == 'X');
        num_o += (board[i] == 'O');
    }
    if (!IsValidPieceConfig(num_blanks, num_x, num_o)) return false;

    // Check if the board is a valid position.
    int turn = formal_position[0] - '1';
    QuixoTier t = {.unpacked = {num_x, num_o}};
    TierPosition tier_position = {
        .tier = t.hash,
        .position = X86SimdTwoPieceHashHash(StrToBoard(board), turn),
    };
    if (!QuixoIsLegalPosition(tier_position)) return false;

    return true;
}

static TierPosition QuixoFormalPositionToTierPosition(
    ReadOnlyString formal_position) {
    //
    ConstantReadOnlyString board = formal_position + 2;
    int turn = formal_position[0] - '1';
    int num_blanks = 0, num_x = 0, num_o = 0;
    for (int i = 0; i < board_size; ++i) {
        num_blanks += (board[i] == '-');
        num_x += (board[i] == 'X');
        num_o += (board[i] == 'O');
    }

    QuixoTier t = {.unpacked = {num_x, num_o}};
    TierPosition ret = {
        .tier = t.hash,
        .position = X86SimdTwoPieceHashHash(StrToBoard(board), turn),
    };

    return ret;
}

static CString QuixoTierPositionToFormalPosition(TierPosition tier_position) {
    // Unhash
    QuixoTier t = {.hash = tier_position.tier};
    __m128i board = X86SimdTwoPieceHashUnhash(tier_position.position,
                                              t.unpacked[0], t.unpacked[1]);
    char board_str[kBoardSizeMax + 1];
    BoardToStr(board, board_str);
    int turn = X86SimdTwoPieceHashGetTurn(tier_position.position) + 1;

    return AutoGuiMakePosition(turn, board_str);
}

static CString QuixoTierPositionToAutoGuiPosition(TierPosition tier_position) {
    // AutoGUI currently does not support mapping '-' (the blank piece) to
    // images. Must use a different character here.
    CString ret = QuixoTierPositionToFormalPosition(tier_position);
    for (int64_t i = 0; i < ret.length; ++i) {
        if (ret.str[i] == '-') ret.str[i] = 'B';
    }

    return ret;
}

static CString QuixoMoveToFormalMove(TierPosition tier_position, Move move) {
    (void)tier_position;  // Unused;
    char buf[5];
    QuixoMoveToString(move, buf);
    CString ret;
    CStringInitCopyCharArray(&ret, buf);

    return ret;
}

static CString QuixoMoveToAutoGuiMove(TierPosition tier_position, Move move) {
    // Format: M_<src>_<dest>_x, where src is the center of the source tile and
    // dest is an invisible center inside the source tile in the move's
    // direction. The first board_size centers (indexed from 0 to board_size -
    // 1) correspond to the centers of the tiles. The next board_size centers
    // (board_size - 2 * board_size - 1) correspond to the destinations of all
    // left arrows. Then follow right, upward, and downward arrows. The sound
    // effect character is hard coded as an 'x'.
    (void)tier_position;  // Unused;
    QuixoMove m = {.hash = move};
    int src = kDirIndexToSrc[curr_variant_idx][m.unpacked.dir][m.unpacked.idx];
    int dest_center = src + (1 + m.unpacked.dir) * board_size;

    char buf[16];
    sprintf(buf, "M_%d_%d", src, dest_center);
    sprintf(buf, "M_%d_%d_x", src, dest_center);
    CString ret;
    CStringInitCopyCharArray(&ret, buf);

    return ret;
}

static const UwapiTier kQuixoUwapiTier = {
    .GetInitialTier = &QuixoGetInitialTier,
    .GetInitialPosition = &QuixoGetInitialPosition,
    .GetRandomLegalTierPosition = NULL,

    .GenerateMoves = &QuixoGenerateMovesGameplay,
    .DoMove = &QuixoDoMove,
    .Primitive = &QuixoPrimitive,

    .IsLegalFormalPosition = &QuixoIsLegalFormalPosition,
    .FormalPositionToTierPosition = &QuixoFormalPositionToTierPosition,
    .TierPositionToFormalPosition = &QuixoTierPositionToFormalPosition,
    .TierPositionToAutoGuiPosition = &QuixoTierPositionToAutoGuiPosition,
    .MoveToFormalMove = &QuixoMoveToFormalMove,
    .MoveToAutoGuiMove = &QuixoMoveToAutoGuiMove,
};

static const Uwapi kQuixoUwapi = {.tier = &kQuixoUwapiTier};

// ================================== kQuixo ==================================

const Game kQuixo = {
    .name = "quixo",
    .formal_name = "Quixo",
    .solver = &kTierSolver,
    .solver_api = &kQuixoSolverApi,
    .gameplay_api = &kQuixoGameplayApi,
    .uwapi = &kQuixoUwapi,  // TODO

    .Init = QuixoInit,
    .Finalize = QuixoFinalize,

    .GetCurrentVariant = QuixoGetCurrentVariant,
    .SetVariantOption = QuixoSetVariantOption,
};
