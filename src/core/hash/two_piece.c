#include "core/hash/two_piece.h"

#ifdef GAMESMAN_HAS_BMI2
#include <immintrin.h>  // _pdep_u32, _pext_u32
#endif                  // GAMESMAN_HAS_BMI2
#include <stdint.h>     // int64_t, uint32_t, uint64_t
#include <stdio.h>      // fprintf, stderr
#include <stdlib.h>     // malloc, free

#include "core/misc.h"
#include "core/types/gamesman_types.h"

enum { kBoardSizeMax = 32 };

static int curr_board_size;
static int32_t *pattern_to_order;
static uint32_t **pop_order_to_pattern;

static int curr_num_symmetries;
static uint32_t **pattern_symmetries;  // [symm][pattern]

intptr_t TwoPieceHashGetMemoryRequired(int rows, int cols, int num_symmetries) {
    int board_size = rows * cols;
    intptr_t ret = (1 << board_size) * sizeof(int32_t);
    ret += (board_size + 1) * sizeof(uint32_t *);
    ret += (1 << board_size) * sizeof(int32_t);  // Binomial theorem

    if (num_symmetries > 1) {
        ret += num_symmetries * (1 << board_size) * sizeof(uint32_t);
    }

    return ret;
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
            (uint32_t *)malloc(NChooseR(curr_board_size, i) * sizeof(uint32_t));
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
    pattern_symmetries =
        (uint32_t **)malloc(curr_num_symmetries * sizeof(uint32_t *));
    if (!pattern_symmetries) return kMallocFailureError;
    for (int i = 0; i < curr_num_symmetries; ++i) {
        pattern_symmetries[i] =
            (uint32_t *)malloc((1 << curr_board_size) * sizeof(uint32_t));
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

int TwoPieceHashInit(int rows, int cols, const int *const *symmetry_matrix,
                     int num_symmetries) {
    // Validate board size
    int board_size = rows * cols;
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
    free(pattern_to_order);
    pattern_to_order = NULL;

    // pop_order_to_pattern
    for (int i = 0; i <= curr_board_size; ++i) {
        free(pop_order_to_pattern[i]);
    }
    free(pop_order_to_pattern);
    pop_order_to_pattern = NULL;

    // pattern_symmetries
    for (int i = 0; i < curr_num_symmetries; ++i) {
        free(pattern_symmetries[i]);
    }
    free(pattern_symmetries);
    pattern_symmetries = NULL;

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
