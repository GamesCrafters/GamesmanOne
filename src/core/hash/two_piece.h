/**
 * @file two_piece.h
 * @author François Bonnet: original published version, arXiv:2007.15895v1
 * https://github.com/st34-satoshi/quixo-cpp/tree/master/others/multi-fb/codeFrancois_v7
 * @author Robert Shi (robertyishi@berkeley.edu): optimized using BMI2
 * intrinsics and added support for symmetry caching.
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Hash system for tier games with boards of arbitrary shapes, size 32
 * or less, and using no more than two types of pieces.
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

#ifndef GAMESMANONE_CORE_HASH_TWO_PIECE_H_
#define GAMESMANONE_CORE_HASH_TWO_PIECE_H_

#include <stdbool.h>  // bool
#include <stdint.h>   // int64_t, uint64_t

#include "core/types/gamesman_types.h"

/**
 * @brief Returns the amount of memory required in bytes to initialize the hash
 * system.
 *
 * @param board_size Size of the board in number of slots.
 * @param num_symmetries Number of symmetries in total, including the identity.
 * Set this value to 1 if you wish to turn symmetries off.
 * @return Amount of memory required in bytes to initialize the hash system.
 */
intptr_t TwoPieceHashGetMemoryRequired(int board_size, int num_symmetries);

/**
 * @brief Initializes the hash system.
 *
 * @param board_size Size of the board in number of slots.
 * @param symmetry_matrix A 2D array containing reordered indicies in each
 * symmetry mapping. The first dimension should be equal to \p num_symmetries
 * such that each row of the matrix contains the reordered indices of the
 * original board. Furthermore, each row should contain values 0 thru
 * \p board_size - 1. The second dimension should be equal to \p board_size.
 * Refer to the definition of kSymmetryMatrix in mtttier.c as an example.
 * @param num_symmetries Number of symmetries in total, including the identity.
 * Set this value to 1 if you wish to turn symmetries off.
 * @return \c kNoError on success, or
 * @return any non-zero error code defined by GAMESMAN otherwise.
 */
int TwoPieceHashInit(int board_size, const int *const *symmetry_matrix,
                     int num_symmetries);

/**
 * @brief Finalizes the hash system.
 */
void TwoPieceHashFinalize(void);

/**
 * @brief Returns the number of positions in the tier with \p num_x X pieces and
 * \p num_o O pieces remaining on the board.
 *
 * @param num_x Number of X pieces on the board.
 * @param num_o Number of O pieces on the board.
 * @return Number of positions in the tier with \p num_x X pieces and \p num_o
 * O pieces remaining on the board.
 */
int64_t TwoPieceHashGetNumPositions(int num_x, int num_o);

/**
 * @brief Returns the hash of the given \p board and \p turn.
 * @note Turn is either 0 or 1, which is different from the design of Generic
 * Hash.
 *
 * @param board Board represented as an unsigned 64-bit integer. Refer to the
 * documentation in the header of this library for the format.
 * @param turn 0 if it is the first player (X)'s turn, or 1 if it is the second
 * player (O)'s turn.
 * @return Hash value of the given \p board and \p turn.
 */
Position TwoPieceHashHash(uint64_t board, int turn);

/**
 * @brief Unhashes the given position \p hash into a board represented as an
 * unsigned 64-bit integer. Refer to the documentation in the header of this
 * library for the format of the board.
 *
 * @param hash Hash value of the position.
 * @param num_x Number of X pieces on the board.
 * @param num_o Number of O pieces on the board.
 * @return Board unhashed.
 */
uint64_t TwoPieceHashUnhash(Position hash, int num_x, int num_o);

/**
 * @brief Returns whose turn it is at the given position with hash value
 * \p hash.
 *
 * @param hash Hash value of the position.
 * @return 0 if it is the first player (X)'s turn, or
 * @return 1 if it is the second player (O)'s turn.
 */
int TwoPieceHashGetTurn(Position hash);

/**
 * @brief Returns the canonical version of the given board inside the group of
 * symmetric boards defined by the symmetry matrix. The canonical board is
 * defined as the one with the smallest hash, which coincides with the one with
 * the smallest unsigned 64-bit integer representation.
 *
 * @param board Source board.
 * @return Canonical board.
 */
uint64_t TwoPieceHashGetCanonicalBoard(uint64_t board);

#endif  // GAMESMANONE_CORE_HASH_TWO_PIECE_H_
