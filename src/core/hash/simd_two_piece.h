/**
 * @file simd_two_piece.h
 * @author François Bonnet: original published version, arXiv:2007.15895v1
 * https://github.com/st34-satoshi/quixo-cpp/tree/master/others/multi-fb/codeFrancois_v7
 * @author Robert Shi (robertyishi@berkeley.edu): reimplemented with 64-bit
 * piece patterns using SIMD vectors and added support for efficient board
 * mirroring and rotation.
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Hash system for tier games with boards of size 32 or less and using no
 * more than two types of pieces.
 *
 * The following restrictions apply to the game:
 *
 * 1. The tier definition of the game must be based on the number of remaining
 * pieces of each player. The hash functions provided in this library returns
 * the hash value of positions within the corresponding tier with the above
 * definition. For example, when using this library to hash positions in
 * Tic-Tac-Toe, the tiers must be defined as [0X 0O], [1X 0O], [1X 1O], [2X 1O],
 * [2X 2O], ..., [5X 4O]. Subdivision or merging of tiers are currently
 * unsupported.
 *
 * 2. There must exist a way to map the board onto a 8x8 grid. Examples of valid
 * game boards include all rectangular/square game boards with both dimensions
 * smaller than 8 and the board for Nine Men's Morris, which can be mapped onto
 * a 7x7 grid.
 *
 * @note This library only provides minimal safety checks on input values for
 * performance.
 *
 * @details Usage guide: this hash system provides functions to convert board
 * representations to position hash values within each tier (hashing) and to
 * convert hash values back to boards (unhashing). The tiers are defined using
 * the numbers of the two types of pieces on the board. The boards are
 * represented as U64x2 SIMD vectors containing two bit boards each of length 64
 * describing the locations of the pieces. The lower 64 bits represent the
 * locations of the first type of piece (X) and the upper 64 bits represent the
 * locations of the second type of piece (O).
 *
 * When initialized with a rectangular board layout, the patterns are padded
 * with zeros at the end of each row and column so that the board is mapped to
 * the bottom right corner of a 8x8 bit grid. The number of rows and columns of
 * the original board is referred to as the numbers of "effective rows" and
 * "effective columns."
 *
 * When initialized with a custom board mask, the effective board slots are
 * those that correspond to the set bits (1 bits) in the mask.
 *
 * The above definitions are better illustrated using the 2 examples below.
 * In both examples, we use 'X' to represent the first type of piece, 'O' to
 * represent the second, and '-' to represent blank slots. We 0-index the board
 * slots from the bottom right to the top left in row-major order as follows
 * (showing 3x3 for brevity but generalizes to all valid board dimensions):
 *
 *     8 7 6
 *     5 4 3
 *     2 1 0
 *
 * @example 1. Rectangular/Square board initialized using the
 * SimdTwoPieceHashContextInit function
 *
 * The following example position in a Tic-Tac-Toe game represented using the
 * 3x3 board
 *
 *     X O -
 *     - X X
 *     O - O
 *
 * is equivalent to the result of overlapping the following two boards
 *
 *     X - -    - O -
 *     - X X    - - -
 *     - - -    O - O
 *
 * The boards are first mapped to the following 8x8 grids
 *
 *     - - - - - - - -    - - - - - - - -
 *     - - - - - - - -    - - - - - - - -
 *     - - - - - - - -    - - - - - - - -
 *     - - - - - - - -    - - - - - - - -
 *     - - - - - - - -    - - - - - - - -
 *     - - - - - X - -    - - - - - - O -
 *     - - - - - - X X    - - - - - - - -
 *     - - - - - - - -    - - - - - O - O
 *
 * and then represented as
 *
 *     // Using C++ notation for binary numbers, not valid syntax in C.
 *     U64x2 board = {
 *         0b0000000000000000000000000000000000000000'00000100'00000011'00000000,
 *         0b0000000000000000000000000000000000000000'00000010'00000000'00000101,
 *     };
 *
 * The boards are mapped to 64-bit grids to allow efficient flipping, mirroring,
 * and rotating, for which the algorithms can be found on Chess Programming Wiki
 * https://www.chessprogramming.org/Flipping_Mirroring_and_Rotating.
 * Methods to perform these operations are provided in this library for
 * efficient symmetry removal.
 *
 * @example 2. Irregular board initialized using the
 * SimdTwoPieceHashInitIrregular function.
 *
 * The game of Nine Men's Morris uses the following irregular board:
 *
 *    ( )---------( ) --------( )
 *     |           |           |
 *     |  ( )-----( )-----( )  |
 *     |   |       |       |   |
 *     |   |  ( )-( )-( )  |   |
 *     |   |   |       |   |   |
 *    ( )-( )-( )     ( )-( )-( )
 *     |   |   |       |   |   |
 *     |   |  ( )-( )-( )  |   |
 *     |   |       |       |   |
 *     |  ( ) ----( )---- ( )  |
 *     |           |           |
 *    ( )---------( ) --------( )
 *
 * Notice if we allow gaps between neighboring intersections, the board
 * intersections can be mapped onto a 7x7 grid:
 *
 *     1 0 0 1 0 0 1
 *     0 1 0 1 0 1 0
 *     0 0 1 1 1 0 0
 *     1 0 1 0 1 0 1
 *     0 0 1 1 1 0 0
 *     0 1 0 1 0 1 0
 *     1 0 0 1 0 0 1
 *
 * The "board mask" for this game is therefore the result of mapping this 7x7
 * grid onto the bottom right corner of an 8x8 grid:
 *
 *     // Using C++ notation for binary numbers, not valid syntax in C.
 *     uint64_t board_mask =
 *         0b00000000'01001001'00101010'00011100'01010101'00011100'00101010'01001001
 *
 * The following example position in a Nine Men's Morris game represented using
 * the irregular board
 *
 *    ( )---------( ) --------( )
 *     |           |           |
 *     |  ( )----- X -----( )  |
 *     |   |       |       |   |
 *     |   |   X -( )-( )  |   |
 *     |   |   |       |   |   |
 *    ( )- O -( )     ( )-( )-( )
 *     |   |   |       |   |   |
 *     |   |  ( )-( ) -X   |   |
 *     |   |       |       |   |
 *     |  ( ) ---- O ---- ( )  |
 *     |           |           |
 *    ( )---------( ) --------( )
 *
 * is equivalent to the result of overlapping the following two boards
 *
 *    ( )---------( ) --------( )   ( )---------( ) --------( )
 *     |           |           |     |           |           |
 *     |  ( )----- X -----( )  |     |  ( )-----( )-----( )  |
 *     |   |       |       |   |     |   |       |       |   |
 *     |   |   X -( )-( )  |   |     |   |  ( )-( )-( )  |   |
 *     |   |   |       |   |   |     |   |   |       |   |   |
 *    ( )-( )-( )     ( )-( )-( )   ( )- O -( )     ( )-( )-( )
 *     |   |   |       |   |   |     |   |   |       |   |   |
 *     |   |  ( )-( ) -X   |   |     |   |  ( )-( )-( )  |   |
 *     |   |       |       |   |     |   |       |       |   |
 *     |  ( ) ----( )---- ( )  |     |  ( ) ---- O ---- ( )  |
 *     |           |           |     |           |           |
 *    ( )---------( ) --------( )   ( )---------( ) --------( )
 *
 *
 * The boards are first mapped onto the following 8x8 grids
 *
 *     - - - - - - - -    - - - - - - - -
 *     - - - - - - - -    - - - - - - - -
 *     - - - - X - - -    - - - - - - - -
 *     - - - X - - - -    - - - - - - - -
 *     - - - - - - - -    - - O - - - - -
 *     - - - - - X - -    - - - - - - - -
 *     - - - - - - - -    - - - - O - - -
 *     - - - - - - - -    - - - - - - - -
 *
 * and then represented as
 *
 *     // Using C++ notation for binary numbers, not valid syntax in C.
 *     U64x2 board = {
 *         0b00000000'00000000'00001000'00010000'00000000'00000100'00000000'00000000,
 *         0b00000000'00000000'00000000'00000000'00100000'00000000'00001000'00000000,
 *     };
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

#ifndef GAMESMANONE_CORE_HASH_SIMD_TWO_PIECE_H_
#define GAMESMANONE_CORE_HASH_SIMD_TWO_PIECE_H_

#include <stdalign.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "config.h"
#include "core/types/base.h"
#include "core/types/gamesman_status.h"
#include "core/types/simd.h"

#ifdef GAMESMAN_HAS_SSE2
#include <immintrin.h>
#endif  // GAMESMAN_HAS_SSE2

enum {
    kSimdTwoPieceHashBoardSizeMax = 32 /**< Maximum supported board size. */
};

/**
 * @brief Hash context containing precomputed tables and metadata.
 *
 * This structure stores the state needed to hash and unhash board positions,
 * including binomial coefficients, pattern mappings, and a valid board mask.
 */
typedef struct {
    /**
     * Bitmask of valid board slots. Aligned to `GM_CACHE_LINE_SIZE` to ensure
     * that the first few elements reside on the same cache line.
     */
    alignas(GM_CACHE_LINE_SIZE) uint64_t hash_mask;

    /**
     * Array mapping a bit pattern to its lexicographical order index.
     */
    int32_t *__restrict pattern_to_order;

    /**
     * Number of effective slots on the board.
     */
    int32_t board_size;

    /**
     * Explicit padding included for alignment.
     */
    uint32_t padding;

    /**
     * Array mapping a piece count and order index back to a bit pattern.
     * All pointers share the same underlying array.
     */
    uint32_t
        *__restrict pop_order_to_pattern[kSimdTwoPieceHashBoardSizeMax + 1];

    /**
     * Precomputed combinations table (n choose r) for calculating offsets.
     */
    int32_t nCr[kSimdTwoPieceHashBoardSizeMax + 1]
               [kSimdTwoPieceHashBoardSizeMax + 1];
} SimdTwoPieceHashContext;

// Adding this exclusion here because some intrisic functions report to have
// branches when compiled with -O0.

// LCOV_EXCL_BR_START

/**
 * @brief Returns the amount of memory in bytes required to initialize the hash
 * system for a game using a board with `num_slots` effective slots.
 *
 * @details Use this function to check memory usage before calling
 * `SimdTwoPieceHashContextInit` or
 * `SimdTwoPieceHashContextInitIrregular` to avoid running out of memory.
 *
 * @param[in] num_slots Number of effective slots.
 *
 * @returns Amount of memory required to initialize the hash system.
 * @retval SIZE_MAX if `num_slots` is negative, zero, or greater than
 * `kSimdTwoPieceHashBoardSizeMax`.
 */
size_t SimdTwoPieceHashContextMemoryRequired(int num_slots);

/**
 * @brief Initializes the hash context for a rectangular board with `rows` rows
 * and `cols` columns.
 *
 * @details The number of effective rows and columns are also `rows` and `cols`,
 * respectively.
 *
 * @param[out] context Hash context to initialize.
 * @param[in] rows Number of board rows.
 * @param[in] cols Number of board columns.
 *
 * @retval kSuccess On success.
 * @retval kIllegalArgumentError If either `rows` or `cols` is less than 1 or
 * greater than 8, or if `rows` * `cols` is greater than 32.
 * @retval kMallocFailureError On memory allocation failure.
 */
Status SimdTwoPieceHashContextInit(SimdTwoPieceHashContext *context, int rows,
                                   int cols);

/**
 * @brief Initializes the hash context for an irregular board specified through
 * the `board_mask` parameter.
 *
 * @param[out] context Hash context to initialize.
 * @param[in] board_mask A bit mask where set bits specify effective board
 * slots. See the instruction manual at the beginning of this header for a
 * detailed explanation.
 *
 * @retval kSuccess On success.
 * @retval kIllegalArgumentError If the mask contains no set bits.
 * @retval kMallocFailureError On memory allocation failure.
 */
Status SimdTwoPieceHashContextInitIrregular(SimdTwoPieceHashContext *context,
                                            uint64_t board_mask);

/**
 * @brief Deallocates the hash context.
 *
 * @details Does nothing if `context` is `NULL` or has previously been
 * deallocated.
 *
 * @param[in,out] context Hash context to deallocate.
 */
void SimdTwoPieceHashContextDestroy(SimdTwoPieceHashContext *context);

/**
 * @brief Returns the total number of positions with `num_x` X's and `num_o`
 * O's on the board, assuming it is always one of the players' turn.
 *
 * @note X is the first player, and O is the second player.
 *
 * @param[in] context Hash context.
 * @param[in] num_x Number of X's on the board.
 * @param[in] num_o Number of O's on the board.
 *
 * @returns Total number of positions in the tier with `num_x` X's and
 * `num_o` O's on the board, assuming it is always one of the players' turn.
 */
static inline int64_t SimdTwoPieceHashGetNumPositionsFixedTurn(
    const SimdTwoPieceHashContext *context, int num_x, int num_o) {
    const int32_t board_size = context->board_size;
    return context->nCr[board_size - num_o][num_x] *
           context->nCr[board_size][num_o];
}

/**
 * @brief Returns the total number of positions with `num_x` X's and `num_o`
 * O's on the board, including both players' turns.
 *
 * @note X is the first player, and O is the second player.
 *
 * @param[in] context Hash context.
 * @param[in] num_x Number of X's on the board.
 * @param[in] num_o Number of O's on the board.
 *
 * @returns Total number of positions in the tier with `num_x` X's and
 * `num_o` O's on the board, including either player's turn.
 */
static inline int64_t SimdTwoPieceHashGetNumPositions(
    const SimdTwoPieceHashContext *context, int num_x, int num_o) {
    return SimdTwoPieceHashGetNumPositionsFixedTurn(context, num_x, num_o) * 2;
}

/**
 * @brief Returns the hash for the given position.
 *
 * @details The position is represented as 64-bit piece patterns packed in a
 * 128-bit SIMD vector `board` consisting of two unsigned 64-bit integers. It
 * assumes the given position is from a tier in which all positions are one of
 * the players' turn. The `board` must be packed in the following way:
 *     `board[0]` := bit pattern of X
 *     `board[1]` := bit pattern of O
 *
 * @param[in] context Hash context.
 * @param[in] board Board to hash.
 *
 * @returns Hash value of the given position.
 */
static inline Position SimdTwoPieceHashHashFixedTurn(
    const SimdTwoPieceHashContext *context, U64x2 board) {
    // Convert the 8x8 padded pattern to tightly packed pattern
    const uint64_t hash_mask = context->hash_mask;
    // LCOV_EXCL_START
    alignas(16) uint64_t patterns[2] = {
        PextU64(board[0], hash_mask),
        PextU64(board[1], hash_mask),
    };
    // LCOV_EXCL_STOP

    // Perform the normal hashing procedure.
    patterns[0] = PextU64(patterns[0], ~patterns[1]);
    int pop_x = __builtin_popcountll(patterns[0]);
    int pop_o = __builtin_popcountll(patterns[1]);
    int64_t offset = context->nCr[context->board_size - pop_o][pop_x];

    return offset * context->pattern_to_order[patterns[1]] +
           context->pattern_to_order[patterns[0]];
}

/**
 * @brief Returns the hash for the given position, accounting for the turn.
 *
 * @details The position is represented as 64-bit piece patterns packed in a
 * 128-bit SIMD vector `board` consisting of two unsigned 64-bit integers, with
 * the given `turn`. The `board` must be packed in the following way:
 *     `board[0]` := bit pattern of X
 *     `board[1]` := bit pattern of O
 *
 * @param[in] context Hash context.
 * @param[in] board Board to hash.
 * @param[in] turn 0 if it is the first player's turn, 1 if it is the second
 * player's turn.
 *
 * @returns Hash value of the given position.
 */
static inline Position SimdTwoPieceHashHash(
    const SimdTwoPieceHashContext *context, U64x2 board, int turn) {
    return (SimdTwoPieceHashHashFixedTurn(context, board) << 1) | turn;
}

/**
 * @brief Unhashes the given position to a `U64x2` SIMD vector.
 *
 * @details Reconstructs a position with `num_x` X's and `num_o` O's from the
 * given `hash`. It assumes `hash` was previously obtained using
 * `SimdTwoPieceHashHashFixedTurn` that does not account for turns. The
 * format for the return value matches the format of the input to
 * `SimdTwoPieceHashHashFixedTurn`.
 *
 * @note X is the first player, and O is the second player.
 *
 * @param[in] context Hash context.
 * @param[in] hash Hash value of the position to unhash.
 * @param[in] num_x Number of X's on the board.
 * @param[in] num_o Number of O's on the board.
 *
 * @returns Unhashed board represented as two 64-bit piece patterns packed into
 * a 128-bit SIMD vector consisting of two unsigned 64-bit integers.
 */
static inline U64x2 SimdTwoPieceHashUnhashFixedTurn(
    const SimdTwoPieceHashContext *context, Position hash, int num_x,
    int num_o) {
    U64x2 board;
    int64_t offset = context->nCr[context->board_size - num_o][num_x];
    board[0] = context->pop_order_to_pattern[num_x][hash % offset];
    board[1] = context->pop_order_to_pattern[num_o][hash / offset];
    board[0] = PdepU64(board[0], ~board[1]);
    board[0] = PdepU64(board[0], context->hash_mask);
    board[1] = PdepU64(board[1], context->hash_mask);

    return board;
}

/**
 * @brief Unhashes the given position, accounting for turns, to a `U64x2`
 * SIMD vector.
 *
 * @details Reconstructs a position with `num_x` X's and `num_o` O's from the
 * given `hash`. It assumes `hash` was previously obtained using
 * `SimdTwoPieceHashHash` that accounts for turns. The format for the return
 * value matches the format of the input to `SimdTwoPieceHashHash`.
 *
 * @note X is the first player, and O is the second player.
 *
 * @param[in] context Hash context.
 * @param[in] hash Hash value of the position to unhash.
 * @param[in] num_x Number of X's on the board.
 * @param[in] num_o Number of O's on the board.
 *
 * @returns Unhashed board represented as two 64-bit piece patterns packed into
 * a 128-bit SIMD vector consisting of two unsigned 64-bit integers.
 */
static inline U64x2 SimdTwoPieceHashUnhash(
    const SimdTwoPieceHashContext *context, Position hash, int num_x,
    int num_o) {
    // Get rid of the turn bit and then use the same algorithm.
    return SimdTwoPieceHashUnhashFixedTurn(context, hash >> 1, num_x, num_o);
}

/**
 * @brief Returns whose turn it is (0-indexed) at the given position.
 *
 * @details Extracts the turn information from the `hash` value, assuming it was
 * previously obtained from `SimdTwoPieceHashHash` that accounts for turns.
 *
 * @param[in] hash Hash value of the position.
 *
 * @retval 0 If it is the first player's turn.
 * @retval 1 If it is the second player's turn.
 */
static inline int SimdTwoPieceHashGetTurn(Position hash) { return hash & 1; }

/**
 * @brief Flips the board across the diagonal going from top left to bottom
 * right.
 *
 * @details
 *
 *     \ 1 1 1 1 . . .    . . . . . . . .
 *     . 1 . . . 1 . .    1 1 1 1 1 1 1 1
 *     . 1 . . . 1 . .    1 . . . 1 . . .
 *     . 1 . . 1 . . .    1 . . . 1 1 . .
 *     . 1 1 1 . . . .    1 . . 1 . . 1 .
 *     . 1 . 1 . . . .    . 1 1 . . . . 1
 *     . 1 . . 1 . . .    . . . . . . . .
 *     . 1 . . . 1 . \    . . . . . . . .
 *
 * @note Flipping a rectangular board whose row and column numbers do not match
 * results in a new board with the numbers of rows and columns swapped and
 * cannot be mapped to the original.
 *
 * @param[in] board Packed 64-bit patterns.
 *
 * @returns Flipped board.
 *
 * @see
 * https://www.chessprogramming.org/Flipping_Mirroring_and_Rotating#Diagonal
 */
static inline U64x2 SimdTwoPieceHashFlipDiag(U64x2 board) {
    U64x2 t;
    const U64x2 k1 = {0x5500550055005500ULL, 0x5500550055005500ULL};
    const U64x2 k2 = {0x3333000033330000ULL, 0x3333000033330000ULL};
    const U64x2 k4 = {0x0f0f0f0f00000000ULL, 0x0f0f0f0f00000000ULL};

    t = k4 & (board ^ (board << 28));
    board = board ^ t ^ (t >> 28);
    t = k2 & (board ^ (board << 14));
    board = board ^ t ^ (t >> 14);
    t = k1 & (board ^ (board << 7));
    board = board ^ t ^ (t >> 7);

    return board;
}

/**
 * @brief Flips the board vertically.
 *
 * @details
 *
 *     . 1 1 1 1 . . .     . 1 . . . 1 . .
 *     . 1 . . . 1 . .     . 1 . . 1 . . .
 *     . 1 . . . 1 . .     . 1 . 1 . . . .
 *     . 1 . . 1 . . .     . 1 1 1 . . . .
 *     . 1 1 1 . . . .     . 1 . . 1 . . .
 *     . 1 . 1 . . . .     . 1 . . . 1 . .
 *     . 1 . . 1 . . .     . 1 . . . 1 . .
 *     . 1 . . . 1 . .     . 1 1 1 1 . . .
 *
 * @param[in] board Packed 64-bit patterns.
 * @param[in] rows Number of effective rows in the board.
 *
 * @returns Flipped board.
 *
 * @see
 * https://www.chessprogramming.org/Flipping_Mirroring_and_Rotating#Vertical
 */
static inline U64x2 SimdTwoPieceHashFlipVertical(U64x2 board, int rows) {
    // 1. Cast to byte-vector to perform the byte swap
    U8x16 bytes = (U8x16)board;

    // 2. Reverse the first 8 bytes and the last 8 bytes directly in the
    // SIMD vector. This perfectly mimics bswap64 (from x86) on two 64-bit
    // integers.
    bytes = __builtin_shufflevector(
        bytes, bytes, 7, 6, 5, 4, 3, 2, 1, 0,  // bswap64 for s[0]
        15, 14, 13, 12, 11, 10, 9, 8           // bswap64 for s[1]
    );

    // 3. Cast back to 64-bit vector for the shift
    U64x2 b64 = (U64x2)bytes;

    // 4. Shift to the bottom right corner.
    int shift = (8 - rows) << 3;
    b64 = b64 >> shift;

    return b64;
}

/**
 * @brief Mirrors the board horizontally.
 *
 * @details
 *
 *     . 1 1 1|1 . . .     . . . 1 1 1 1 .
 *     . 1 . . . 1 . .     . . 1 . . . 1 .
 *     . 1 . . . 1 . .     . . 1 . . . 1 .
 *     . 1 . . 1 . . .     . . . 1 . . 1 .
 *     . 1 1 1 . . . .     . . . . 1 1 1 .
 *     . 1 . 1 . . . .     . . . . 1 . 1 .
 *     . 1 . . 1 . . .     . . . 1 . . 1 .
 *     . 1 . .|. 1 . .     . . 1 . . . 1 .
 *
 * @param[in] board Packed 64-bit patterns.
 * @param[in] cols Number of effective columns in the board.
 *
 * @returns Mirrored board.
 *
 * @see
 * https://www.chessprogramming.org/Flipping_Mirroring_and_Rotating#Horizontal
 */
static inline U64x2 SimdTwoPieceHashMirrorHorizontal(U64x2 board, int cols) {
#ifdef GAMESMAN_HAS_SSSE3
    // lut_low: bit-reverses the nibble and keeps it in the low 4 bits.
    // E.g., index 1 (0001) maps to 8 (1000).
    const U8x16 lut_low = {0x00, 0x08, 0x04, 0x0c, 0x02, 0x0a, 0x06, 0x0e,
                           0x01, 0x09, 0x05, 0x0d, 0x03, 0x0b, 0x07, 0x0f};

    // lut_high: bit-reverses the nibble and shifts it to the high 4 bits.
    // E.g., index 1 (0001) maps to 128 (1000 0000).
    const U8x16 lut_high = {0x00, 0x80, 0x40, 0xc0, 0x20, 0xa0, 0x60, 0xe0,
                            0x10, 0x90, 0x50, 0xd0, 0x30, 0xb0, 0x70, 0xf0};

    const U8x16 mask = {0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f,
                        0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f};

    // Isolate the low and high nibbles for every byte simultaneously
    U8x16 b8 = (U8x16)board;
    U8x16 low_nibbles = b8 & mask;
    U8x16 high_nibbles = (b8 >> 4) & mask;

    // Look up the bit-reversed nibbles.
    // The previously 'low' nibble reversed becomes the 'high' nibble and vice
    // versa.
    U8x16 rev_low =
        (U8x16)_mm_shuffle_epi8((__m128i)lut_high, (__m128i)low_nibbles);
    U8x16 rev_high =
        (U8x16)_mm_shuffle_epi8((__m128i)lut_low, (__m128i)high_nibbles);

    // Recombine the reversed halves
    board = (U64x2)(rev_low | rev_high);
#else   // Without SSSE3
    const U64x2 k1 = {0x5555555555555555LL, 0x5555555555555555LL};
    const U64x2 k2 = {0x3333333333333333LL, 0x3333333333333333LL};
    const U64x2 k4 = {0x0f0f0f0f0f0f0f0fLL, 0x0f0f0f0f0f0f0f0fLL};

    board = ((board >> 1) & k1) | ((board & k1) << 1);
    board = ((board >> 2) & k2) | ((board & k2) << 2);
    board = ((board >> 4) & k4) | ((board & k4) << 4);
#endif  // GAMESMAN_HAS_SSSE3

    // Move the board to the correct location for valid bit alignment
    return board >> (8 - cols);
}

/**
 * @brief Swaps the X and O pieces on the given `board`.
 *
 * @param[in] board Packed 64-bit patterns.
 *
 * @returns Board after swapping X and O pieces.
 */
static inline U64x2 SimdTwoPieceHashSwapPieces(U64x2 board) {
    U64x2 swapped = {board[1], board[0]};
    return swapped;
}

#ifdef GAMESMAN_HAS_SSE2
/**
 * @brief For boards with <= 7 rows only, returns `true` iff the hash value for
 * board `a` is strictly less than the hash value for board `b`.
 *
 * @note This function is more efficient than `cmplt_u128` but only gives the
 * correct comparison result if the board has 7 or fewer effective rows. This is
 * because the `_mm_cmplt_epi8` intrinsic treats each byte inside the `U64x2`
 * variables as signed integers. Using this function for symmetry removal when
 * there are 8 effective rows does not lead to an error in the solver result but
 * may result in suboptimal database compression. In this case, consider using
 * `cmplt_u128` instead for board comparison.
 *
 * @param[in] a Left hand side of the comparison.
 * @param[in] b Right hand side of the comparison.
 *
 * @retval true If `a` is strictly less than `b`.
 * @retval false Otherwise.
 *
 * @see https://stackoverflow.com/a/56346628
 */
static inline bool SimdTwoPieceHashBoardLessThan(U64x2 a, U64x2 b) {
    const int less = _mm_movemask_epi8(_mm_cmplt_epi8((__m128i)a, (__m128i)b));
    const int greater =
        _mm_movemask_epi8(_mm_cmpgt_epi8((__m128i)a, (__m128i)b));

    return less > greater;
}
#else   // No SSE2
/**
 * @brief Returns whether the hash value for board `a` is strictly less than the
 * hash value for board `b`.
 *
 * @param[in] a Left hand side of the comparison.
 * @param[in] b Right hand side of the comparison.
 *
 * @retval true If `a` is strictly less than `b`.
 * @retval false Otherwise.
 */
static inline bool SimdTwoPieceHashBoardLessThan(U64x2 a, U64x2 b) {
    if (a[1] != b[1]) {
        return a[1] < b[1];
    }

    return a[0] < b[0];
}
#endif  // GAMESMAN_HAS_SSE2

/**
 * @brief For boards with <= 7 rows only, returns min(`a`, `b`).
 *
 * @note Empirical data suggests that a branch-free solution for selecting the
 * minimum board using this function does not outperform the solution using
 * the replace-if-smaller logic on Alder Lake; that is, the following logic:
 * `if (SimdTwoPieceHashBoardLessThan(next, min)) { min = next; }`
 * may actually be faster in a hot loop even if branch mispredictions are
 * possible. Benchmarking is therefore strongly recommended before picking a
 * solution.
 *
 * @param[in] a Board A.
 * @param[in] b Board B.
 *
 * @returns Either `a` or `b`, whichever is smaller.
 */
static inline U64x2 SimdTwoPieceHashMinBoard(U64x2 a, U64x2 b) {
    // Cast the boolean result to an unsigned 64-bit integer.
    // Negating 1 gives 0xFFFFFFFFFFFFFFFF; negating 0 gives 0x0.
    uint64_t mask_val = -(uint64_t)SimdTwoPieceHashBoardLessThan(a, b);

    // Broadcast the 64-bit scalar mask into a 128-bit vector mask
    U64x2 mask = {mask_val, mask_val};

    // Bitwise blend: selects 'a' if mask is all 1s (a < b), otherwise selects
    // 'b'
    return (a & mask) | (b & ~mask);
}

/**
 * @brief Returns `true` iff `a` < `b` with `a` and `b` treated as unsigned
 * 128-bit integers.
 *
 * @param[in] a Left hand side of the comparison.
 * @param[in] b Right hand side of the comparison.
 *
 * @retval true If `a` is strictly less than `b`.
 * @retval false Otherwise.
 *
 * @see https://stackoverflow.com/a/56346628
 */
static inline bool cmplt_u128(U64x2 a, U64x2 b) {
#ifdef GAMESMAN_HAS_SSE2
    // Flip the sign bits in both arguments.
    // Transforms 0 into -128 = minimum for signed bytes,
    // 0xFF into +127 = maximum for signed bytes
    const uint64_t lane = 0x8080808080808080ULL;
    const U64x2 signBits = {lane, lane};
    a = a ^ signBits;
    b = b ^ signBits;
    // Now the signed byte comparisons will give the correct order
#endif  // GAMESMAN_HAS_SSE2

    return SimdTwoPieceHashBoardLessThan(a, b);
}

#endif  // GAMESMANONE_CORE_HASH_SIMD_TWO_PIECE_H_
