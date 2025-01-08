#include "core/generic_hash/two_piece.h"

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

intptr_t TwoPieceHashGetMemoryRequired(int board_size) {
    intptr_t ret = (1 << board_size) * sizeof(int32_t);
    ret += (board_size + 1) * sizeof(uint32_t *);
    for (int i = 0; i <= board_size; ++i) {
        ret += NChooseR(curr_board_size, i) * sizeof(uint32_t);
    }

    return ret;
}

static int InitTables(void) {
    // Allocate space
    pattern_to_order =
        (int32_t *)malloc((1 << curr_board_size) * sizeof(int32_t));
    pop_order_to_pattern =
        (uint32_t **)malloc((curr_board_size + 1) * sizeof(uint32_t *));
    if (!pattern_to_order || !pop_order_to_pattern) {
        fprintf(stderr, "TwoPieceHashInit: failed to allocate memory\n");
        free(pattern_to_order);
        free(pop_order_to_pattern);
        return kMallocFailureError;
    }
    for (int i = 0; i <= curr_board_size; ++i) {
        pop_order_to_pattern[i] =
            (uint32_t *)malloc(NChooseR(curr_board_size, i) * sizeof(uint32_t));
        if (!pop_order_to_pattern[i]) {
            fprintf(stderr, "TwoPieceHashInit: failed to allocate memory\n");
            free(pattern_to_order);
            for (int j = 0; j < i; ++j) {
                free(pop_order_to_pattern[j]);
            }
            free(pop_order_to_pattern);
            return kMallocFailureError;
        }
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

int TwoPieceHashInit(int board_size) {
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
    return InitTables();
}

void TwoPieceHashFinalize(void) {
    free(pattern_to_order);
    pattern_to_order = NULL;
    for (int i = 0; i <= curr_board_size; ++i) {
        free(pop_order_to_pattern[i]);
    }
    free(pop_order_to_pattern);
    pop_order_to_pattern = NULL;
    curr_board_size = 0;
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

    return (ret << 1) | (turn - 1);
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

int TwoPieceHashGetTurn(Position hash) { return (hash & 1) + 1; }
