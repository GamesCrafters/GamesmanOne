/**
 * @file simd_two_piece.c
 * @author François Bonnet: original published version, arXiv:2007.15895v1
 * https://github.com/st34-satoshi/quixo-cpp/tree/master/others/multi-fb/codeFrancois_v7
 * @author Robert Shi (robertyishi@berkeley.edu): reimplemented with 64-bit
 * piece patterns using Intel SSE2, SSE4.1 and BMI2 intrinsics; added support
 * for efficient board mirroring and rotation.
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Implementation of the SIMD two-piece hash system.
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
#include "core/hash/simd_two_piece.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "core/gamesman_memory.h"
#include "core/types/gamesman_status.h"

size_t SimdTwoPieceHashContextMemoryRequired(int num_slots) {
    if (num_slots <= 0 || num_slots > kSimdTwoPieceHashBoardSizeMax) {
        return SIZE_MAX;
    }

    const size_t num_patterns = 1ULL << num_slots;
    const size_t pattern_to_order_size = num_patterns * sizeof(int32_t);
    const size_t pop_order_to_pattern_size = num_patterns * sizeof(uint32_t);

    return pattern_to_order_size + pop_order_to_pattern_size;
}

static Status ValidateRowsCols(int rows, int cols) {
    if (rows <= 0 || rows > 8 || cols <= 0 || cols > 8) {
        fprintf(stderr,
                "ValidateRowsCols: invalid number of rows or columns "
                "provided. Valid range: [1, 8]\n");
        return kIllegalArgumentError;
    }

    return kSuccess;
}

static Status ValidateBoardSize(int board_size) {
    if (board_size <= 0 || board_size > kSimdTwoPieceHashBoardSizeMax) {
        fprintf(stderr,
                "ValidateBoardSize: invalid board size (%d) provided. "
                "Valid range: [1, %d]\n",
                board_size, kSimdTwoPieceHashBoardSizeMax);
        return kIllegalArgumentError;
    }

    return kSuccess;
}

static void InitTriangle(SimdTwoPieceHashContext *context) {
    for (int i = 0; i <= kSimdTwoPieceHashBoardSizeMax; ++i) {
        context->nCr[i][0] = 1;
        for (int j = 1; j <= i; ++j) {
            context->nCr[i][j] =
                context->nCr[i - 1][j - 1] + context->nCr[i - 1][j];
        }
    }
}

static Status InitTables(SimdTwoPieceHashContext *context, int board_size) {
    InitTriangle(context);
    const size_t num_patterns = 1ULL << board_size;

    // 1. Allocate space
    context->pattern_to_order =
        (int32_t *)GamesmanCallocWhole(num_patterns, sizeof(int32_t));
    uint32_t *flat_pop_array =
        (uint32_t *)GamesmanCallocWhole(num_patterns, sizeof(uint32_t));

    if (!context->pattern_to_order || !flat_pop_array) {
        return kMallocFailureError;
    }

    // 2. Wire the embedded struct pointers to offsets in the single flat array
    size_t current_offset = 0;
    for (int i = 0; i <= board_size; ++i) {
        context->pop_order_to_pattern[i] = flat_pop_array + current_offset;
        current_offset += context->nCr[board_size][i];
    }

    // 3. Initialize tables
    int32_t order_count[kSimdTwoPieceHashBoardSizeMax + 1] = {0};
    for (size_t i = 0; i < num_patterns; ++i) {
        int pop = __builtin_popcountll(i);
        int32_t order = order_count[pop]++;
        context->pattern_to_order[i] = order;
        context->pop_order_to_pattern[pop][order] = (uint32_t)i;
    }

    return kSuccess;
}

static uint64_t BuildRectangularHashMask(int rows, int cols) {
    uint64_t mask = 0;
    for (int i = 0; i < rows; ++i) {
        mask |= ((1ULL << cols) - 1ULL) << (i * 8);
    }

    return mask;
}

Status SimdTwoPieceHashContextInit(SimdTwoPieceHashContext *context, int rows,
                                   int cols) {
    memset(context, 0, sizeof(*context));

    Status status = ValidateRowsCols(rows, cols);
    if (status != kSuccess) {
        goto _bailout;
    }

    context->board_size = rows * cols;
    status = ValidateBoardSize(context->board_size);
    if (status != kSuccess) {
        goto _bailout;
    }

    status = InitTables(context, context->board_size);
    if (status != kSuccess) {
        goto _bailout;
    }

    context->hash_mask = BuildRectangularHashMask(rows, cols);

_bailout:
    if (status != kSuccess) {
        SimdTwoPieceHashContextDestroy(context);
    }

    return status;
}

Status SimdTwoPieceHashContextInitIrregular(SimdTwoPieceHashContext *context,
                                            uint64_t board_mask) {
    memset(context, 0, sizeof(*context));

    // Board size is the number of set bits in board_mask
    context->board_size = __builtin_popcountll(board_mask);
    Status status = ValidateBoardSize(context->board_size);
    if (status != kSuccess) {
        goto _bailout;
    }

    status = InitTables(context, context->board_size);
    if (status != kSuccess) {
        goto _bailout;
    }

    context->hash_mask = board_mask;

_bailout:
    if (status != kSuccess) {
        SimdTwoPieceHashContextDestroy(context);
    }

    return status;
}

void SimdTwoPieceHashContextDestroy(SimdTwoPieceHashContext *context) {
    if (!context) {
        return;
    }

    GamesmanFree(context->pattern_to_order);
    GamesmanFree(context->pop_order_to_pattern[0]);
    memset(context, 0, sizeof(*context));
}
