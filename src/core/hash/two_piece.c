/**
 * @file two_piece.c
 * @author François Bonnet: original published version, arXiv:2007.15895v1
 * https://github.com/st34-satoshi/quixo-cpp/tree/master/others/multi-fb/codeFrancois_v7
 * @author Robert Shi (robertyishi@berkeley.edu): optimized using BMI2
 * intrinsics and added support for symmetry caching.
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Implementation of the hash system for tier games with boards of
 * arbitrary shapes, size 32 or less, and using no more than two types of
 * pieces.
 * @note The system assumes that the game is tiered based on the number of
 * remaining pieces of each type.
 * @note This module provides minimal safety check for inputs for performance.
 * The user should carefully read the the instructions before using this
 * library.
 * @note This module is a portable implementation of the hash system with
 * fallback methods that only use basic C language features. Users with modern
 * x86 CPUs may consider the x86 specialized library provided by
 * x86_simd_two_piece.h for higher performance.
 * @details Usage guide: this hash system provides functions to convert board
 * representations to position hash values within each tier (hashing) and to
 * convert hash values back to boards (unhashing). The tiers are defined using
 * the numbers of the two types of pieces on the board. The boards are
 * represented as unsigned 64-bit integers (uint64_t) containing two bit boards
 * each of length 32 describing the locations of the pieces. The lower 32 bits
 * (0-31) show the locations of the second type of piece (O) and the upper 32
 * bits (32-63) show the first type of piece (X). Note that this mapping matches
 * the original design by François Bonnet but is different from the x86
 * specialized version in x86_simd_two_piece.h. If the board size is smaller
 * than 32, then only the lower BOARD_SIZE bits of each 32-bit range contains
 * useful information and the upper (32-BOARD_SIZE) bits should be all zeros.
 * @version 1.0.1
 * @date 2025-03-30
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
#include "core/hash/two_piece.h"

#ifdef GAMESMAN_HAS_BMI2
#include <immintrin.h>  // _pdep_u32, _pext_u32
#endif                  // GAMESMAN_HAS_BMI2
#include <stdint.h>     // int64_t, uint32_t, uint64_t
#include <stdio.h>      // fprintf, stderr

#include "core/gamesman_memory.h"
#include "core/misc.h"
#include "core/types/gamesman_types.h"

enum { kBoardSizeMax = 32 };

static int curr_board_size;
static int32_t *pattern_to_order;
static uint32_t **pop_order_to_pattern;

static int curr_num_symmetries;
static uint32_t **pattern_symmetries;  // [symm][pattern]

size_t TwoPieceHashGetMemoryRequired(int board_size, int num_symmetries) {
    size_t ret = (1ULL << board_size) * sizeof(int32_t);
    ret += (board_size + 1) * sizeof(uint32_t *);
    ret += (1ULL << board_size) * sizeof(int32_t);  // Binomial theorem

    if (num_symmetries > 1) {
        ret += num_symmetries * (1ULL << board_size) * sizeof(uint32_t);
    }

    return ret;
}

static int InitTables(void) {
    // Allocate space
    pattern_to_order =
        (int32_t *)GamesmanMalloc((1 << curr_board_size) * sizeof(int32_t));
    pop_order_to_pattern = (uint32_t **)GamesmanCallocWhole(
        (curr_board_size + 1), sizeof(uint32_t *));
    if (!pattern_to_order || !pop_order_to_pattern) return kMallocFailureError;

    for (int i = 0; i <= curr_board_size; ++i) {
        pop_order_to_pattern[i] = (uint32_t *)GamesmanMalloc(
            NChooseR(curr_board_size, i) * sizeof(uint32_t));
        if (!pop_order_to_pattern[i]) return kMallocFailureError;
    }

    // Initialize tables
    int32_t order_count[kBoardSizeMax] = {0};
    for (uint32_t i = 0; i < (1U << curr_board_size); ++i) {
        int pop = Popcount32(i);
        int32_t order = order_count[pop]++;
        pattern_to_order[i] = order;
        pop_order_to_pattern[pop][order] = i;
    }

    return kNoError;
}

static int InitSymmetries(const int *const *symmetry_matrix) {
    // Allocate space
    pattern_symmetries = (uint32_t **)GamesmanCallocWhole(curr_num_symmetries,
                                                          sizeof(uint32_t *));
    if (!pattern_symmetries) return kMallocFailureError;
    for (int i = 0; i < curr_num_symmetries; ++i) {
        pattern_symmetries[i] = (uint32_t *)GamesmanMalloc(
            (1 << curr_board_size) * sizeof(uint32_t));
        if (!pattern_symmetries[i]) return kMallocFailureError;
    }

    for (uint32_t pattern = 0; pattern < (1U << curr_board_size); ++pattern) {
        for (int i = 0; i < curr_num_symmetries; ++i) {
            uint32_t sym = 0;
            for (int j = 0; j < curr_board_size; ++j) {
                sym |= ((pattern >> j) & 1U) << symmetry_matrix[i][j];
            }
            pattern_symmetries[i][pattern] = sym;
        }
    }

    return kNoError;
}

int TwoPieceHashInit(int board_size, const int *const *symmetry_matrix,
                     int num_symmetries) {
    // Validate board size
    if (board_size <= 0 || board_size > kBoardSizeMax) {
        fprintf(stderr,
                "TwoPieceHashInit: invalid board size (%d) provided. "
                "Valid range: [1, kBoardSizeMax]\n",
                board_size);
        return kIllegalArgumentError;
    }
    curr_board_size = board_size;

    // Initialize the tables
    int error = InitTables();
    if (error != kNoError) {
        TwoPieceHashFinalize();
        return error;
    }

    // Initialize the symmetry lookup table if requested
    if (symmetry_matrix && num_symmetries > 1) {
        if (num_symmetries > INT8_MAX) {
            fprintf(stderr,
                    "TwoPieceHashInit: too many symmetries (%d) provided. At "
                    "most %d are supported\n",
                    num_symmetries, INT8_MAX);
            TwoPieceHashFinalize();
            return kIllegalArgumentError;
        }

        curr_num_symmetries = num_symmetries;
        int error = InitSymmetries(symmetry_matrix);
        if (error != kNoError) {
            TwoPieceHashFinalize();
            return error;
        }
    }

    return kNoError;
}

void TwoPieceHashFinalize(void) {
    // pattern_to_order
    GamesmanFree(pattern_to_order);
    pattern_to_order = NULL;

    // pop_order_to_pattern
    if (pop_order_to_pattern != NULL) {
        for (int i = 0; i <= curr_board_size; ++i) {
            GamesmanFree(pop_order_to_pattern[i]);
        }
        GamesmanFree(pop_order_to_pattern);
        pop_order_to_pattern = NULL;
    }

    // pattern_symmetries
    if (pattern_symmetries != NULL) {
        for (int i = 0; i < curr_num_symmetries; ++i) {
            GamesmanFree(pattern_symmetries[i]);
        }
        GamesmanFree(pattern_symmetries);
        pattern_symmetries = NULL;
    }

    // Reset the board size
    curr_board_size = 0;

    // Reset number of symmetries.
    curr_num_symmetries = 0;
}

int64_t TwoPieceHashGetNumPositions(int num_x, int num_o) {
    return NChooseR(curr_board_size - num_x, num_o) *
           NChooseR(curr_board_size, num_x) * 2;
}

Position TwoPieceHashHash(uint64_t board, int turn) {
    uint32_t s_x = (uint32_t)(board >> 32);
#ifdef GAMESMAN_HAS_BMI2
    // Extract bits from the lower 32 bits of board where the bits at the
    // same positions are 0 in s_x, and pack them into the lowest bits of s_o.
    uint32_t s_o = _pext_u32((uint32_t)board, ~s_x);
#else   // GAMESMAN_HAS_BMI2 not defined
    uint32_t s_o = 0;
    for (uint64_t mask = 1 << (curr_board_size - 1); mask; mask >>= 1) {
        if (board & mask) {
            s_o = (s_o << 1) | 1;
        } else if (!(s_x & mask)) {
            s_o <<= 1;
        }
    }
#endif  // GAMESMAN_HAS_BMI2
    int pop_x = Popcount32(s_x);
    int pop_o = Popcount32(s_o);
    int64_t offset = NChooseR(curr_board_size - pop_x, pop_o);
    Position ret = offset * pattern_to_order[s_x] + pattern_to_order[s_o];

    return (ret << 1) | turn;
}

uint64_t TwoPieceHashUnhash(Position hash, int num_x, int num_o) {
    hash >>= 1;  // get rid of the turn bit
    int64_t offset = NChooseR(curr_board_size - num_x, num_o);
    uint32_t s_x = pop_order_to_pattern[num_x][hash / offset];
    uint32_t s_o = pop_order_to_pattern[num_o][hash % offset];

#ifdef GAMESMAN_HAS_BMI2
    // Deposit bits from s_o into the zero positions of s_x.
    uint32_t deposit_result = _pdep_u32(s_o, ~s_x);
    uint64_t ret = ((uint64_t)s_x << 32) | deposit_result;
#else   // GAMESMAN_HAS_BMI2 not defined
    uint64_t ret = ((uint64_t)s_x) << 32;
    uint32_t mask_o = 1;
    for (uint32_t mask_x = 1; mask_x != (1U << curr_board_size); mask_x <<= 1) {
        if (!(s_x & mask_x)) {
            if (s_o & mask_o) ret |= mask_x;
            mask_o <<= 1;
        }
    }
#endif  // GAMESMAN_HAS_BMI2

    return ret;
}

int TwoPieceHashGetTurn(Position hash) { return hash & 1; }

uint64_t TwoPieceHashGetCanonicalBoard(uint64_t board) {
    uint64_t min_board = board;
    for (int i = 1; i < curr_num_symmetries; ++i) {
        uint32_t s_x = (uint32_t)(board >> 32);
        uint32_t s_o = (uint32_t)board;
        uint32_t c_x = pattern_symmetries[i][s_x];
        uint32_t c_o = pattern_symmetries[i][s_o];
        uint64_t new_board = (((uint64_t)c_x) << 32) | (uint64_t)c_o;
        if (new_board < min_board) min_board = new_board;
    }

    return min_board;
}
