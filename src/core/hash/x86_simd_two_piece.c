/**
 * @file x86_simd_two_piece.c
 * @author François Bonnet: original published version, arXiv:2007.15895v1
 * https://github.com/st34-satoshi/quixo-cpp/tree/master/others/multi-fb/codeFrancois_v7
 * @author Robert Shi (robertyishi@berkeley.edu): reimplemented with 64-bit
 * piece patterns using Intel SSE2 and BMI2 intrinsics; added support for
 * efficient board mirroring and rotation.
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Implementation of the x86 SIMD hash system for tier games with
 * rectangular boards of size 32 or less and using no more than two types of
 * pieces.
 * @version 1.0.0
 * @date 2025-01-14
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
#include "core/hash/x86_simd_two_piece.h"

#include <stdbool.h>    // bool, true, false
#include <stdint.h>     // intptr_t, int64_t, uint64_t, uint32_t
#include <stdio.h>      // fprintf, stderr
#include <stdlib.h>     // malloc, free
#include <x86intrin.h>  // __m128i

#include "core/types/gamesman_types.h"

/**
 * @brief Maximum supported board size.
 * @details The current implementation supports at most 32 board slots. It is
 * possible to expand this to 64 in theory, which would require a
 * reimplementation of the system using 64-bit tables. However, the amount of
 * memory required to cache the piece patterns doubles for each additional board
 * slot and it is not likely in the near future that we will have enough memory
 * to efficiently support common board sizes such as 36 slots, which would
 * cost ~256 GiB memory with 64-bit tables.
 */
enum { kBoardSizeMax = 32 };

static bool nCrInitialized;
static int64_t nCr[kBoardSizeMax + 1][kBoardSizeMax + 1];

static bool system_initialized;
static int board_rows;
static int board_cols;
static int curr_board_size;
static uint64_t hash_mask;
static int32_t *pattern_to_order;
static uint32_t **pop_order_to_pattern;

intptr_t X86SimdTwoPieceHashGetMemoryRequired(int rows, int cols) {
    int board_size = rows * cols;
    intptr_t ret = (1LL << board_size) * sizeof(int32_t);
    ret += (board_size + 1) * sizeof(uint32_t *);
    ret += (1LL << board_size) * sizeof(int32_t);  // Binomial theorem

    return ret;
}

static void MakeTriangle(void) {
    if (nCrInitialized) return;
    for (int i = 0; i <= kBoardSizeMax; ++i) {
        nCr[i][0] = 1;
        for (int j = 1; j <= i; ++j) {
            nCr[i][j] = nCr[i - 1][j - 1] + nCr[i - 1][j];
        }
    }
    nCrInitialized = true;
}

static void BuildHashMask(void) {
    hash_mask = 0;
    for (int i = 0; i < board_rows; ++i) {
        hash_mask |= ((1ULL << board_cols) - 1) << (i * 8);
    }
}

static int InitTables(void) {
    // Allocate space
    pattern_to_order =
        (int32_t *)malloc((1 << curr_board_size) * sizeof(int32_t));
    pop_order_to_pattern =
        (uint32_t **)malloc((curr_board_size + 1) * sizeof(uint32_t *));
    if (!pattern_to_order || !pop_order_to_pattern) return kMallocFailureError;

    for (int i = 0; i <= curr_board_size; ++i) {
        pop_order_to_pattern[i] =
            (uint32_t *)malloc(nCr[curr_board_size][i] * sizeof(uint32_t));
        if (!pop_order_to_pattern[i]) return kMallocFailureError;
    }

    // Initialize tables
    int32_t order_count[kBoardSizeMax] = {0};
    for (uint32_t i = 0; i < (1U << curr_board_size); ++i) {
        int pop = _popcnt32(i);
        int32_t order = order_count[pop]++;
        pattern_to_order[i] = order;
        pop_order_to_pattern[pop][order] = i;
    }

    return kNoError;
}

int X86SimdTwoPieceHashInit(int rows, int cols) {
    // Validate rows and cols
    if (rows <= 0 || rows > 8 || cols <= 0 || cols > 8) {
        fprintf(stderr,
                "TwoPieceHashInit: invalid number of rows or columns provided. "
                "Valid range: [1, 8]\n");
        return kIllegalArgumentError;
    }

    // Validate board size
    int board_size = rows * cols;
    if (board_size <= 0 || board_size > kBoardSizeMax) {
        fprintf(stderr,
                "TwoPieceHashInit: invalid board size (%d) provided. "
                "Valid range: [1, %d]\n",
                board_size, kBoardSizeMax);
        return kIllegalArgumentError;
    }

    // Clear previous system state if exist.
    if (system_initialized) X86SimdTwoPieceHashFinalize();

    board_rows = rows;
    board_cols = cols;
    curr_board_size = board_size;
    MakeTriangle();
    BuildHashMask();

    // Initialize the tables
    int error = InitTables();
    if (error != kNoError) X86SimdTwoPieceHashFinalize();
    system_initialized = true;

    return error;
}

void X86SimdTwoPieceHashFinalize(void) {
    // pattern_to_order
    free(pattern_to_order);
    pattern_to_order = NULL;

    // pop_order_to_pattern
    for (int i = 0; i <= curr_board_size; ++i) {
        free(pop_order_to_pattern[i]);
    }
    free(pop_order_to_pattern);
    pop_order_to_pattern = NULL;

    // Reset the board size
    board_rows = board_cols = 0;
    curr_board_size = 0;

    // Reset the hash mask
    hash_mask = 0;
}

int64_t X86SimdTwoPieceHashGetNumPositions(int num_x, int num_o) {
    return nCr[curr_board_size - num_o][num_x] * nCr[curr_board_size][num_o] *
           2;
}

Position X86SimdTwoPieceHashHash(__m128i board, int turn) {
    // Extract the two 64-bit patterns to 16-byte-aligned stack memory
    __attribute__((aligned(16))) uint64_t s[2];
    _mm_store_si128((__m128i *)s, board);

    // Convert the 8x8 padded pattern to tightly packed pattern
    s[0] = _pext_u64(s[0], hash_mask);
    s[1] = _pext_u64(s[1], hash_mask);

    // Perform the normal hashing procedure.
    s[0] = _pext_u64(s[0], ~s[1]);
    int pop_x = _popcnt64(s[0]);
    int pop_o = _popcnt64(s[1]);
    int64_t offset = nCr[curr_board_size - pop_o][pop_x];
    Position ret = offset * pattern_to_order[s[1]] + pattern_to_order[s[0]];

    return (ret << 1) | turn;
}

__m128i X86SimdTwoPieceHashUnhash(Position hash, int num_x, int num_o) {
    hash >>= 1;  // get rid of the turn bit
    int64_t offset = nCr[curr_board_size - num_o][num_x];
    __attribute__((aligned(16))) uint64_t s[2] = {
        pop_order_to_pattern[num_x][hash % offset],
        pop_order_to_pattern[num_o][hash / offset],
    };
    s[0] = _pdep_u64(s[0], ~s[1]);
    s[0] = _pdep_u64(s[0], hash_mask);
    s[1] = _pdep_u64(s[1], hash_mask);

    // TODO: benchmark _mm_stream_load_si128
    return _mm_load_si128((const __m128i *)s);
}

void X86SimdTwoPieceHashUnhashMem(Position hash, int num_x, int num_o,
                                  uint64_t patterns[2]) {
    hash >>= 1;  // get rid of the turn bit
    int64_t offset = nCr[curr_board_size - num_o][num_x];
    patterns[0] = pop_order_to_pattern[num_x][hash % offset];
    patterns[1] = pop_order_to_pattern[num_o][hash / offset];
    patterns[0] = _pdep_u64(patterns[0], ~patterns[1]);
    patterns[0] = _pdep_u64(patterns[0], hash_mask);
    patterns[1] = _pdep_u64(patterns[1], hash_mask);
}
