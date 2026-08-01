/**
 * @file x86_simd_two_piece.h
 * @author François Bonnet: original published version, arXiv:2007.15895v1
 * https://github.com/st34-satoshi/quixo-cpp/tree/master/others/multi-fb/codeFrancois_v7
 * @author Robert Shi (robertyishi@berkeley.edu): reimplemented with 64-bit
 * piece patterns using Intel SSE2, SSE4.1 and BMI2 intrinsics; added support
 * for efficient board mirroring and rotation.
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Hash system for tier games with boards of size 32 or less and using no
 * more than two types of pieces. The following restrictions apply to the game:
 *
 * 1. The tier definition of the game must be based on the number of remaining
 * pieces each player. The hash functions provided in this library returns the
 * hash value of positions within the corresponding tier with the above
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
 * @note This library requires Intel SSE2, SSE4.1 and BMI2 instruction sets.
 * @details Usage guide: this hash system provides functions to convert board
 * representations to position hash values within each tier (hashing) and to
 * convert hash values back to boards (unhashing). The tiers are defined using
 * the numbers of the two types of pieces on the board. The boards are
 * represented as __m128i variables containing two bit boards each of length 64
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
 * X86SimdTwoPieceHashInit function.
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
 *     uint64_t raw[2] = {
 *         0b0000000000000000000000000000000000000000'00000100'00000011'00000000,
 *         0b0000000000000000000000000000000000000000'00000010'00000000'00000101,
 *     };
 *     __m128i board = _mm_loadu_si128(raw);
 *
 * The boards are mapped to 64-bit grids to allow efficient flipping, mirroring,
 * and rotating, for which the algorithms can be found on Chess Programming Wiki
 * https://www.chessprogramming.org/Flipping_Mirroring_and_Rotating.
 * Methods to perform these operations are provided in this library for
 * efficient symmetry removal.
 *
 * @example 2. Irregular board initialized using the
 * X86SimdTwoPieceHashInitIrregular function.
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
 *     uint64_t raw[2] = {
 *         0b00000000'00000000'00001000'00010000'00000000'00000100'00000000'00000000,
 *         0b00000000'00000000'00000000'00000000'00100000'00000000'00001000'00000000,
 *     };
 *     __m128i board = _mm_loadu_si128(raw);
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

#ifndef GAMESMANONE_CORE_HASH_X86_SIMD_TWO_PIECE_H_
#define GAMESMANONE_CORE_HASH_X86_SIMD_TWO_PIECE_H_

#include <emmintrin.h>
#include <immintrin.h>
#include <smmintrin.h>
#include <stdalign.h>
#include <stdbool.h>
#include <stdint.h>
#include <xmmintrin.h>

#include "config.h"
#include "core/types/base.h"
#include "core/types/gamesman_error.h"

enum { kX86SimdTwoPieceHashBoardSizeMax = 32 };

typedef struct {
    // Aligned to the beginning of a cache line ensures that the first few
    // elements are on the same cache line
    alignas(GM_CACHE_LINE_SIZE) uint64_t hash_mask;
    int32_t *__restrict pattern_to_order;
    int32_t board_size;
    uint32_t padding;  // Explicit padding; included by compiler anyway
    uint32_t
        *__restrict pop_order_to_pattern[kX86SimdTwoPieceHashBoardSizeMax + 1];
    int32_t nCr[kX86SimdTwoPieceHashBoardSizeMax + 1]
               [kX86SimdTwoPieceHashBoardSizeMax + 1];
} X86SimdTwoPieceHashContext;

Status X86SimdTwoPieceHashContextInit(X86SimdTwoPieceHashContext *context,
                                      int rows, int cols);

Status X86SimdTwoPieceHashContextInitIrregular(
    X86SimdTwoPieceHashContext *context, uint64_t board_mask);

void X86SimdTwoPieceHashContextDestroy(X86SimdTwoPieceHashContext *context);

static inline int64_t X86SimdTwoPieceHashGetNumPositionsFixedTurn(
    const X86SimdTwoPieceHashContext *context, int num_x, int num_o) {
    const int32_t board_size = context->board_size;
    return context->nCr[board_size - num_o][num_x] *
           context->nCr[board_size][num_o];
}

static inline int64_t X86SimdTwoPieceHashGetNumPositions(
    const X86SimdTwoPieceHashContext *context, int num_x, int num_o) {
    return X86SimdTwoPieceHashGetNumPositionsFixedTurn(context, num_x, num_o) *
           2;
}

static inline Position X86SimdTwoPieceHashHashFixedTurnMem(
    const X86SimdTwoPieceHashContext *context, const uint64_t _patterns[2]) {
    // Convert the 8x8 padded pattern to tightly packed pattern
    const uint64_t hash_mask = context->hash_mask;
    uint64_t patterns[2] = {
        _pext_u64(_patterns[0], hash_mask),
        _pext_u64(_patterns[1], hash_mask),
    };

    // Perform the normal hashing procedure.
    patterns[0] = _pext_u64(patterns[0], ~patterns[1]);
    int pop_x = __builtin_popcountll(patterns[0]);
    int pop_o = __builtin_popcountll(patterns[1]);
    int64_t offset = context->nCr[context->board_size - pop_o][pop_x];

    return offset * context->pattern_to_order[patterns[1]] +
           context->pattern_to_order[patterns[0]];
}

static inline Position X86SimdTwoPieceHashHashFixedTurn(
    const X86SimdTwoPieceHashContext *context, __m128i board) {
    // Extract the two 64-bit patterns to 16-byte-aligned stack memory as
    // required by _mm_store_si128
    alignas(16) uint64_t s[2];
    _mm_store_si128((__m128i *)s, board);

    return X86SimdTwoPieceHashHashFixedTurnMem(context, s);
}

static inline Position X86SimdTwoPieceHashHashMem(
    const X86SimdTwoPieceHashContext *context, const uint64_t patterns[2],
    int turn) {
    return (X86SimdTwoPieceHashHashFixedTurnMem(context, patterns) << 1) | turn;
}

static inline Position X86SimdTwoPieceHashHash(
    const X86SimdTwoPieceHashContext *context, __m128i board, int turn) {
    return (X86SimdTwoPieceHashHashFixedTurn(context, board) << 1) | turn;
}

static inline void X86SimdTwoPieceHashUnhashFixedTurnMem(
    const X86SimdTwoPieceHashContext *context, Position hash, int num_x,
    int num_o, uint64_t patterns[2]) {
    int64_t offset = context->nCr[context->board_size - num_o][num_x];
    patterns[0] = context->pop_order_to_pattern[num_x][hash % offset];
    patterns[1] = context->pop_order_to_pattern[num_o][hash / offset];
    patterns[0] = _pdep_u64(patterns[0], ~patterns[1]);
    patterns[0] = _pdep_u64(patterns[0], context->hash_mask);
    patterns[1] = _pdep_u64(patterns[1], context->hash_mask);
}

static inline __m128i X86SimdTwoPieceHashUnhashFixedTurn(
    const X86SimdTwoPieceHashContext *context, Position hash, int num_x,
    int num_o) {
    alignas(16) uint64_t s[2];
    X86SimdTwoPieceHashUnhashFixedTurnMem(context, hash, num_x, num_o, s);

    return _mm_load_si128((const __m128i *)s);
}

static inline void X86SimdTwoPieceHashUnhashMem(
    const X86SimdTwoPieceHashContext *context, Position hash, int num_x,
    int num_o, uint64_t patterns[2]) {
    // Get rid of the turn bit and then use the same algorithm.
    X86SimdTwoPieceHashUnhashFixedTurnMem(context, hash >> 1, num_x, num_o,
                                          patterns);
}

static inline __m128i X86SimdTwoPieceHashUnhash(
    const X86SimdTwoPieceHashContext *context, Position hash, int num_x,
    int num_o) {
    // Get rid of the turn bit and then use the same algorithm.
    return X86SimdTwoPieceHashUnhashFixedTurn(context, hash >> 1, num_x, num_o);
}

static inline int X86SimdTwoPieceHashGetTurn(Position hash) { return hash & 1; }

/**
 * @brief Flips the board across the diagonal going from top left to bottom
 * right.
 * @note Flipping a rectangular board whose row and column numbers do not match
 * results in a new board with the numbers of rows and columns swapped and
 * cannot be mapped to the original.
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
 * @param board Packed 64-bit patterns.
 * @return Flipped board.
 *
 * @ref Chess Programming Wiki (note that their indexing is different)
 * https://www.chessprogramming.org/Flipping_Mirroring_and_Rotating#Diagonal
 */
static inline __m128i X86SimdTwoPieceHashFlipDiag(__m128i board) {
    __m128i t;
    const __m128i k1 = _mm_set1_epi64x(0x5500550055005500LL);
    const __m128i k2 = _mm_set1_epi64x(0x3333000033330000LL);
    const __m128i k4 = _mm_set1_epi64x(0x0f0f0f0f00000000LL);
    t = _mm_and_si128(k4, _mm_xor_si128(board, _mm_slli_epi64(board, 28)));
    board = _mm_xor_si128(board, _mm_xor_si128(t, _mm_srli_epi64(t, 28)));
    t = _mm_and_si128(k2, _mm_xor_si128(board, _mm_slli_epi64(board, 14)));
    board = _mm_xor_si128(board, _mm_xor_si128(t, _mm_srli_epi64(t, 14)));
    t = _mm_and_si128(k1, _mm_xor_si128(board, _mm_slli_epi64(board, 7)));
    board = _mm_xor_si128(board, _mm_xor_si128(t, _mm_srli_epi64(t, 7)));

    return board;
}

/**
 * @brief Flips the board vertically.
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
 * @param board Packed 64-bit patterns.
 * @param rows Number of effective rows in the board.
 * @return Flipped board.
 *
 * @ref Chess Programming Wiki
 * https://www.chessprogramming.org/Flipping_Mirroring_and_Rotating#Vertical
 */
static inline __m128i X86SimdTwoPieceHashFlipVertical(__m128i board, int rows) {
    // Extract the two 64-bit patterns to 16-byte-aligned stack memory as
    // required by _mm_store_si128
    alignas(16) uint64_t s[2];
    _mm_store_si128((__m128i *)s, board);

    // Byte swap flips the board vertically
    s[0] = __builtin_bswap64(s[0]);
    s[1] = __builtin_bswap64(s[1]);

    // Pack the values back into the __m128i register
    board = _mm_load_si128((const __m128i *)s);

    // Move the board to the correct location
    int shift = (8 - rows) << 3;
    board = _mm_srli_epi64(board, shift);

    return board;
}

/**
 * @brief Mirrors the board horizontally.
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
 * @param board Packed 64-bit patterns.
 * @param cols Number of effective columns in the board.
 * @return Flipped board.
 *
 * @ref Chess Programming Wiki
 * https://www.chessprogramming.org/Flipping_Mirroring_and_Rotating#Horizontal
 */
static inline __m128i X86SimdTwoPieceHashMirrorHorizontal(__m128i board,
                                                          int cols) {
    const __m128i k1 = _mm_set1_epi64x(0x5555555555555555LL);
    const __m128i k2 = _mm_set1_epi64x(0x3333333333333333LL);
    const __m128i k4 = _mm_set1_epi64x(0x0f0f0f0f0f0f0f0fLL);
    board = _mm_or_si128(_mm_and_si128(_mm_srli_epi64(board, 1), k1),
                         _mm_slli_epi64(_mm_and_si128(board, k1), 1));
    board = _mm_or_si128(_mm_and_si128(_mm_srli_epi64(board, 2), k2),
                         _mm_slli_epi64(_mm_and_si128(board, k2), 2));
    board = _mm_or_si128(_mm_and_si128(_mm_srli_epi64(board, 4), k4),
                         _mm_slli_epi64(_mm_and_si128(board, k4), 4));

    // Move the board to the correct location
    board = _mm_srli_epi64(board, 8 - cols);

    return board;
}

/**
 * @brief Swaps the X and O pieces on the given \p board .
 *
 * @param board Packed 64-bit patterns.
 * @return Board after swapping X and O pieces.
 */
static inline __m128i X86SimdTwoPieceHashSwapPieces(__m128i board) {
    return _mm_shuffle_epi32(board, _MM_SHUFFLE(1, 0, 3, 2));
}

/**
 * @brief For boards with <= 7 rows only, returns true iff the hash value for
 * board \p a is strictly less than the hash value for board \p b.
 * @note This function is more efficient than cmplt_u128 but only gives the
 * correct comparison result if the board has 7 or fewer effective rows. This is
 * because the _mm_cmplt_epi8 intrinsic treats each byte inside the __m128i
 * variables as signed integers. Using this function for symmetry removal when
 * there are 8 effective rows does not lead to an error in the solver result but
 * may result in suboptimal database compression. In this case, consider using
 * cmplt_u128 instead for board comparison.
 *
 * @param a Left hand side of the comparison.
 * @param b Right hand side of the comparison.
 * @return \c true if a < b,
 * @return \c false otherwise.
 *
 * @ref https://stackoverflow.com/a/56346628
 */
static inline bool X86SimdTwoPieceHashBoardLessThan(__m128i a, __m128i b) {
    const int less = _mm_movemask_epi8(_mm_cmplt_epi8(a, b));
    const int greater = _mm_movemask_epi8(_mm_cmpgt_epi8(a, b));

    return less > greater;
}

/**
 * @brief For boards with <= 7 rows only, returns min( \p a , \p b ).
 * @note Empirical data suggests that a branch-free solution for selecting the
 * minimum board using this function does not outperform the solution using
 * the replace-if-smaller logic on Alder Lake; that is, the following logic
 * if (X86SimdTwoPieceHashBoardLessThan(next, min)) {
 *     min = next;
 * }
 * may actually be faster in a hot loop even if branch mispredictions are
 * possible. Benchmarking is therefore strongly recommended before picking a
 * solution.
 *
 * @param a Board A.
 * @param b Board B.
 * @return Either \p a or \p b , which ever is smaller.
 */
static inline __m128i X86SimdTwoPieceHashMinBoard(__m128i a, __m128i b) {
    // 0xFF in each i8 if a < b, 0x0 otherwise.
    __m128i mask = _mm_set1_epi8(-(int)X86SimdTwoPieceHashBoardLessThan(a, b));

    // Selects the second parameter if mask is set to all 0xFF, in which case
    // a is smaller than b.
    return _mm_blendv_epi8(b, a, mask);
}

/**
 * @brief Returns true iff \p a < \p b with \p a and \p b treated as unsigned
 * 128-bit integers.
 *
 * @param a Left hand side of the comparison.
 * @param b Right hand side of the comparison.
 * @return \c true if a < b,
 * @return \c false otherwise.
 *
 * @ref https://stackoverflow.com/a/56346628
 */
static inline bool cmplt_u128(__m128i a, __m128i b) {
    // Flip the sign bits in both arguments.
    // Transforms 0 into -128 = minimum for signed bytes,
    // 0xFF into +127 = maximum for signed bytes
    const __m128i signBits = _mm_set1_epi8((char)0x80);
    a = _mm_xor_si128(a, signBits);
    b = _mm_xor_si128(b, signBits);

    // Now the signed byte comparisons will give the correct order
    const int less = _mm_movemask_epi8(_mm_cmplt_epi8(a, b));
    const int greater = _mm_movemask_epi8(_mm_cmpgt_epi8(a, b));

    return less > greater;
}

#endif  // GAMESMANONE_CORE_HASH_X86_SIMD_TWO_PIECE_H_
