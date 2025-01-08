/**
 * @file test_two_piece.c
 * @brief Unit tests for the Two-Piece Hashing module.
 */

#include <inttypes.h>  // PRIdPtr
#include <stdint.h>    // intptr_t, int64_t, uint64_t
#include <stdio.h>     // fprintf, stderr
#include <stdlib.h>    // EXIT_FAILURE, EXIT_SUCCESS

#include "core/generic_hash/two_piece.h"
#include "core/misc.h"
#include "core/types/gamesman_types.h"

static int TestTwoPieceHashInit(void) {
    // Test invalid board sizes
    if (TwoPieceHashInit(0) != kIllegalArgumentError) return 1;
    if (TwoPieceHashInit(33) != kIllegalArgumentError) return 1;

    // Test valid board sizes
    for (int size = 1; size <= 25; ++size) {
        if (TwoPieceHashInit(size) != kNoError) return 1;
        TwoPieceHashFinalize();
    }

    return 0;
}

static int TestTwoPieceHashHash(void) {
    for (int side_len = 1; side_len < 5; ++side_len) {
        int board_size = side_len * side_len;
        if (TwoPieceHashInit(board_size) != kNoError) return 1;
        for (int x = 0; x <= board_size; ++x) {
            for (int o = 0; o <= board_size - x; ++o) {
                int64_t hash_max =
                    NChooseR(board_size, x) * NChooseR(board_size - x, o) * 2;
                for (Position hash = 0; hash < hash_max; ++hash) {
                    uint64_t board = TwoPieceHashUnhash(hash, x, o);
                    int turn = TwoPieceHashGetTurn(hash);
                    Position hash2 = TwoPieceHashHash(board, turn);
                    if (hash != hash2) {
                        fprintf(stderr,
                                "TwoPieceHashHash: failed with side length %d, "
                                "x = %d, o = %d, position = %" PRIPos " \n",
                                side_len, x, o, hash);
                        return 1;
                    }
                }
            }
        }
        TwoPieceHashFinalize();
    }

    return 0;
}

int main(void) {
    if (TestTwoPieceHashInit()) return EXIT_FAILURE;
    if (TestTwoPieceHashHash()) return EXIT_FAILURE;

    return EXIT_SUCCESS;
}
