/**
 * @file x86_simd_two_piece.h
 * @author François Bonnet: original published version, arXiv:2007.15895v1
 * https://github.com/st34-satoshi/quixo-cpp/tree/master/others/multi-fb/codeFrancois_v7
 * @author Robert Shi (robertyishi@berkeley.edu): reimplemented with 64-bit
 * piece patterns using Intel SSE2 and BMI2 intrinsics; added support for
 * efficient board mirroring and rotation.
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Hash system for tier games with rectangular boards of size 32 or less
 * and using no more than two types of pieces.
 * @note The system assumes that the game is tiered based on the number of
 * remaining pieces of each type.
 * @note This module provides minimal safety check for inputs for performance.
 * The user should carefully read the the instructions before using this
 * library.
 * @note This module requires Intel SSE2 and BMI2 instruction sets.
 * @details Usage guide: this hash system provides functions to convert board
 * representations to position hash values within each tier (hashing) and to
 * convert hash values back to boards (unhashing). The tiers are defined using
 * the numbers of the two types of pieces on the board. The boards are
 * represented as __m128i variables containing two bit boards each of length 64
 * describing the locations of the pieces. The lower 64 bits show the locations
 * of the first type of piece (X) and the upper 64 bits show the second type of
 * piece (O). Furthermore, the patterns are padded with zeros at the end of each
 * row and column so that the board is mapped to the bottom right corner of a
 * 8x8 bit grid. The number of rows and columns of the original board is
 * referred to as the numbers of "effective rows" and "effective columns."
 *
 * The above definitions are better illustrated using the example below, in
 * which we use 'X' to represent the first type of piece, 'O' to represent the
 * second, and '-' to represent blank slots. We 0-index the board slots from
 * bottom right to top left in row-major order as follows (showing 3x3 for
 * brevity but generalizes to all possible board dimensions):
 *
 *     8 7 6
 *     5 4 3
 *     2 1 0
 *
 * @example A 3x3 board
 *
 *     X O -
 *     - X X
 *     O - O
 *
 * which is equivalent to the result of overlapping the following two boards
 *
 *     X - -    - O -
 *     - X X    - - -
 *     - - -    O - O
 *
 * is first mapped to the following 8x8 grids
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
#ifndef GAMESMANONE_CORE_HASH_X86_SIMD_TWO_PIECE_H_
#define GAMESMANONE_CORE_HASH_X86_SIMD_TWO_PIECE_H_

#include <stdbool.h>    // bool
#include <stdint.h>     // intptr_t, int64_t
#include <x86intrin.h>  // __m128i, _mm_*

#include "core/types/gamesman_types.h"

/**
 * @brief Returns the amount of memory in bytes required to initialize the hash
 * system with \p rows effective board rows and \p cols effective board columns.
 * It is recommended to check memory usage using this function before calling
 * X86SimdTwoPieceHashInit.
 *
 * @param rows Number of effective board rows.
 * @param cols Number of effective board cols.
 * @return Amount of memory required to initialize the hash system.
 */
intptr_t X86SimdTwoPieceHashGetMemoryRequired(int rows, int cols);

/**
 * @brief Initializes the hash system, setting effective board rows to \p rows
 * and effective board columns to \p cols. This function is required to be
 * called before all others except X86SimdTwoPieceHashGetMemoryRequired.
 *
 * @param rows Number of effective board rows.
 * @param cols Number of effective board cols.
 * @return kNoError on success,
 * @return kIllegalArgumentError if either \p rows or \p cols is less than 1 or
 * greater than 8; or if \p rows * \p cols is greater than 32.
 */
int X86SimdTwoPieceHashInit(int rows, int cols);

/**
 * @brief Finalizes the hash system and clears frees allocated space.
 */
void X86SimdTwoPieceHashFinalize(void);

/**
 * @brief Returns the number of positions in total with \p num_x X's and
 * \p num_o O's on the board.
 * @note X is the first player, and O is the second player.
 *
 * @param num_x Number of X's on the board.
 * @param num_o Number of O's on the board.
 * @return Total number of positions in the tier with \p num_x X's and
 * \p num_o O's on the board.
 */
int64_t X86SimdTwoPieceHashGetNumPositions(int num_x, int num_o);

/**
 * @brief Returns the hash for the given position represented as 64-bit piece
 * patterns packed in a 128-bit XMM register \p board with the given \p turn.
 * The \p board must be packed in the following way:
 *     board[63:0] := bit pattern of X
 *     board[127:64] := bit pattern of O
 * @param board Board to hash.
 * @param turn 0 if it is the first player's turn, 1 if it is the second
 * player's turn.
 * @return Hash of the given position.
 */
Position X86SimdTwoPieceHashHash(__m128i board, int turn);

/**
 * @brief Unhash the given position with \p num_x X's and \p num_o O's and whose
 * hash value is given by \p hash to a __m128i register. The format for the
 * return value matches the format of the input to X86SimdTwoPieceHashHash.
 * @note X is the first player, and O is the second player.
 * @note Use X86SimdTwoPieceHashUnhashMem instead to unhash to a 128-bit space
 * in memory.
 *
 * @param hash Hash value of the position to unhash.
 * @param num_x Number of X's on the board.
 * @param num_o Number of O's on the board.
 * @return Unhashed board represented as two 64-bit piece patterns packed into a
 * 128-bit XMM register.
 */
__m128i X86SimdTwoPieceHashUnhash(Position hash, int num_x, int num_o);

/**
 * @brief Unhash the given position with \p num_x X's and \p num_o O's and whose
 * hash value is given by \p hash to the 128-bit space at \p patterns. This
 * function is equivalent to first calling X86SimdTwoPieceHashUnhash and then
 * storing the contents of the return value to \p patterns, but is more
 * efficient.
 * @note X is the first player, and O is the second player.
 * @note Use X86SimdTwoPieceHashUnhash instead to unhash to a __m128i register.
 *
 * @param hash Hash value of the position to unhash.
 * @param num_x Number of X's on the board.
 * @param num_o Number of O's on the board.
 * @param patterns Output parameter, unhashed piece patterns.
 */
void X86SimdTwoPieceHashUnhashMem(Position hash, int num_x, int num_o,
                                  uint64_t patterns[2]);

/**
 * @brief Returns whose turn it is (0-indexed) at the given position with hash
 * value \p hash.
 *
 * @param hash Hash value of the position.
 * @return 0 if it is the first player's turn, or
 * @return 1 if it is the second player's turn.
 */
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
 * https://www.chessprogramming.org/Flipping_Mirroring_and_Rotating
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
 * https://www.chessprogramming.org/Flipping_Mirroring_and_Rotating
 */
static inline __m128i X86SimdTwoPieceHashFlipVertical(__m128i board, int rows) {
    // Extract the two 64-bit patterns to 16-byte-aligned stack memory
    __attribute__((aligned(16))) uint64_t s[2];
    _mm_store_si128((__m128i *)s, board);

    // Byte swap flips the board vertically
    s[0] = _bswap64(s[0]);
    s[1] = _bswap64(s[1]);

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
 * https://www.chessprogramming.org/Flipping_Mirroring_and_Rotating
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
