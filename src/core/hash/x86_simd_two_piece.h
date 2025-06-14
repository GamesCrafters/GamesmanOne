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
 * @version 2.0.0
 * @date 2025-04-28
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

#include <immintrin.h>  // __m128i, _mm_*
#include <stdalign.h>   // alignas
#include <stdbool.h>    // bool
#include <stddef.h>     // size_t
#include <stdint.h>     // int64_t, uint64_t

#include "core/types/gamesman_types.h"

/**
 * @brief Returns the amount of memory in bytes required to initialize the hash
 * system for a game using a board with \p num_slots effective slots. Use this
 * function to check memory usage before calling X86SimdTwoPieceHashInit or
 * X86SimdTwoPieceHashInitIrregular to avoid running out of memory.
 *
 * @param slots Number of effective slots. If the board is rectangular, this
 * parameter should be set equal to the number of rows times the number of
 * columns.
 * @return Amount of memory required to initialize the hash system.
 */
size_t X86SimdTwoPieceHashGetMemoryRequired(int num_slots);

/**
 * @brief Initializes the hash system, setting effective board rows to \p rows
 * and effective board columns to \p cols.
 *
 * @param rows Number of effective board rows.
 * @param cols Number of effective board cols.
 * @return \c kNoError on success,
 * @return \c kIllegalArgumentError if either \p rows or \p cols is less than 1
 * or greater than 8; or if \p rows * \p cols is greater than 32.
 */
int X86SimdTwoPieceHashInit(int rows, int cols);

/**
 * @brief Initializes the hash system for an irregular board specified through
 * the \p board_mask parameter.
 *
 * @param board_mask A bit mask where set bits mark effective board slots. See
 * the instruction manual at the beginning of this header for a detailed
 * explanation.
 * @return \c kNoError on success,
 * @return \c kIllegalArgumentError if the mask contains no set bits.
 */
int X86SimdTwoPieceHashInitIrregular(uint64_t board_mask);

/**
 * @brief Finalizes the hash system and clears frees allocated space.
 */
void X86SimdTwoPieceHashFinalize(void);

/**
 * @brief Returns the number of positions in total with \p num_x X's and
 * \p num_o O's on the board, including either player's turn.
 * @note X is the first player, and O is the second player.
 *
 * @param num_x Number of X's on the board.
 * @param num_o Number of O's on the board.
 * @return Total number of positions in the tier with \p num_x X's and
 * \p num_o O's on the board, including either player's turn.
 */
int64_t X86SimdTwoPieceHashGetNumPositions(int num_x, int num_o);

/**
 * @brief Returns the number of positions in total with \p num_x X's and
 * \p num_o O's on the board, assuming it is always one of the players' turn.
 * @note X is the first player, and O is the second player.
 *
 * @param num_x Number of X's on the board.
 * @param num_o Number of O's on the board.
 * @return Total number of positions in the tier with \p num_x X's and
 * \p num_o O's on the board, assuming it is always one of the players' turn.
 */
int64_t X86SimdTwoPieceHashGetNumPositionsFixedTurn(int num_x, int num_o);

/**
 * @brief Returns the hash for the given position represented as 64-bit piece
 * patterns packed in a 128-bit XMM register \p board with the given \p turn .
 * The \p board must be packed in the following way:
 *     board[63:0] := bit pattern of X
 *     board[127:64] := bit pattern of O
 *
 * @param board Board to hash.
 * @param turn 0 if it is the first player's turn, 1 if it is the second
 * player's turn.
 * @return Hash value of the given position.
 */
Position X86SimdTwoPieceHashHash(__m128i board, int turn);

/**
 * @brief Returns the hash for the given position represented as two 64-bit
 * piece patterns packed in a uint64_t array \p patterns with the given \p turn
 * . The \p patterns must be packed in the following way:
 *     patterns[0] := bit pattern of X
 *     patterns[1] := bit pattern of O
 *
 * @param patterns Board represented as bit patterns.
 * @param turn 0 if it is the first player's turn, 1 if it is the second
 * player's turn.
 * @return Hash value of the given position.
 */
Position X86SimdTwoPieceHashHashMem(const uint64_t patterns[2], int turn);

/**
 * @brief Returns the hash for the given position represented as 64-bit piece
 * patterns packed in a 128-bit XMM register \p board , assuming the given
 * position is from a tier in which all positions are one of the players' turn.
 * The \p board must be packed in the following way:
 *     board[63:0] := bit pattern of X
 *     board[127:64] := bit pattern of O
 *
 * @param board Board to hash.
 * @return Hash value of the given position.
 */
Position X86SimdTwoPieceHashHashFixedTurn(__m128i board);

/**
 * @brief Returns the hash for the given position represented as two 64-bit
 * piece patterns packed in a uint64_t array \p patterns , assuming the given
 * position is from a tier in which all positions are one of the players' turn.
 * The \p patterns must be packed in the following way:
 *     patterns[0] := bit pattern of X
 *     patterns[1] := bit pattern of O
 *
 * @param board Board to hash.
 * @return Hash value of the given position.
 */
Position X86SimdTwoPieceHashHashFixedTurnMem(const uint64_t patterns[2]);

/**
 * @brief Unhash the given position with \p num_x X's and \p num_o O's and whose
 * hash value is given by \p hash to a __m128i register, assuming \p hash was
 * previously obtained using X86SimdTwoPieceHashHash that accounts for turns.
 * The format for the return value matches the format of the input to
 * X86SimdTwoPieceHashHash.
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
 * hash value is given by \p hash to a __m128i register, assuming \p hash was
 * previously obtained using X86SimdTwoPieceHashHashFixedTurn that does not
 * account for turns. The format for the return value matches the format of the
 * input to X86SimdTwoPieceHashHashFixedTurn.
 * @note X is the first player, and O is the second player.
 * @note Use X86SimdTwoPieceHashUnhashFixedTurnMem instead to unhash to a
 * 128-bit space in memory.
 *
 * @param hash Hash value of the position to unhash.
 * @param num_x Number of X's on the board.
 * @param num_o Number of O's on the board.
 * @return Unhashed board represented as two 64-bit piece patterns packed into a
 * 128-bit XMM register.
 */
__m128i X86SimdTwoPieceHashUnhashFixedTurn(Position hash, int num_x, int num_o);

/**
 * @brief Unhash the given position with \p num_x X's and \p num_o O's and whose
 * hash value is given by \p hash to the 128-bit space at \p patterns . This
 * function is equivalent to first calling X86SimdTwoPieceHashUnhash and then
 * storing the contents of the return value to \p patterns , but is more
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
 * @brief Unhash the given position with \p num_x X's and \p num_o O's and whose
 * hash value is given by \p hash to the 128-bit space at \p patterns , assuming
 * \p hash was previously obtained using X86SimdTwoPieceHashHashFixedTurn that
 * does not account for turns. This function is equivalent to first calling
 * X86SimdTwoPieceHashUnhashFixedTurn and then storing the contents of the
 * return value to \p patterns , but is more efficient.
 * @note X is the first player, and O is the second player.
 * @note Use X86SimdTwoPieceHashUnhashFixedTurn instead to unhash to a __m128i
 * register.
 *
 * @param hash Hash value of the position to unhash.
 * @param num_x Number of X's on the board.
 * @param num_o Number of O's on the board.
 * @param patterns Output parameter, unhashed piece patterns.
 */
void X86SimdTwoPieceHashUnhashFixedTurnMem(Position hash, int num_x, int num_o,
                                           uint64_t patterns[2]);

/**
 * @brief Returns whose turn it is (0-indexed) at the given position with hash
 * value \p hash, assuming it was previously obtained from
 * X86SimdTwoPieceHashHash that accounts for turns.
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
    alignas(16) uint64_t s[2];
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
