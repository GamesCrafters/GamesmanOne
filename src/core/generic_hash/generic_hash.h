/**
 * @file generic_hash.h
 * @author Dan Garcia: Designer of the original (3-variable only) version.
 * @author Attila Gyulassy: Developer of the original (3-variable only) version.
 * @author Michel D'Sa: Designer and developer of user-specified variables
 * version.
 * @author Scott Lindeneau: Designer and developer of multiple contexts;
 * optimized for efficiency and readability.
 * @author Robert Shi (robertyishi@berkeley.edu): further optimized for
 * efficiency and readability, adapted to the new system and implemented
 * thread-safety and unordered pieces.
 *         GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Generic Hash system for finite board games with fixed sets of pieces.
 * @version 1.1.0
 * @date 2024-07-27
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

#ifndef GAMESMANONE_CORE_GENERIC_HASH_GENERIC_HASH_H_
#define GAMESMANONE_CORE_GENERIC_HASH_GENERIC_HASH_H_

#include <stdint.h>  // int64_t

#include "core/types/gamesman_types.h"

/**
 * @brief (Re)initializes the Generic Hash system, clearing all previously
 * defined hash contexts and definitions. This function should be called before
 * the system is used for the first time, and before switching to a different
 * game.
 */
void GenericHashReinitialize(void);

/**
 * @brief Adds a new Generic Hash Context to the system.
 *
 * @param player May take values 0, 1, or 2. If set to 0, a two-player hash
 * context will be initialized and a turn bit will be added to the final hash
 * value to distinguish between 1st player's turn vs. 2nd player's turn; If set
 * to 1: initialize in 1st player only mode; If set to 2: initialize in 2nd
 * player only mode. E.g., set this to 0 for the game of Chess, and set this to
 * 1 for all tiers that correspond to player X's turn in Tic-Tac-Toe.
 * @param board_size Size of the board. E.g., this value should be set to 9 for
 * the game to Tic-Tac-Toe.
 * @param pieces_init_array An integer array of the following format:
 * [p_1, L_1, U_1, p_2, L_2, U_2, ..., p_n, L_n, U_n,
 * (-2, L_{n+1}, U_{n+1}, L_{n+2}, U_{n+2}, ..., L_m, U_m,) -1] where
 * - The \c p_i 's are the characters associated with the pieces (including
 * blanks.) Min: 0; Max: 127.
 * - The \c L_i 's for i in range(1, n+1) are the minimum allowable number of
 * occurrences of each piece type on the board. Min: 0; Max: \c U_i.
 * - The \c U_i 's for i in range(1, n+1) are the maximum allowable number of
 * occurrences of each piece type on the board. Min: \c L_i; Max: board_size.
 * - (Optional, v1.1.0+) The value -2 is used to separate the board pieces from
 * the unordered pieces, if exist.
 * - (Optional, v1.1.0+) The \c L_j 's for j in range(n+1, m) are the minimum
 * allowable number of occurrences of each piece type that may appear in the
 * unordered section of the game. Min: 0; Max: \c U_j.
 * - (Optional, v1.1.0+) The \c U_j 's for j in range(n+1, m) are the maximum
 * allowable number of occurrences of each piece type that may appear in the
 * unordered section of the game. Min: \c L_j; Max: 127.
 * - The value \c -1 is used to mark the end of the array.
 * \example 1: set this to {'-', 0, 9, 'O', 0, 4, 'X', 0, 5, -1} for the game of
 * Tic-Tac-Toe. Explanation: there can be at least 0 or at most 9 blank slots,
 * at least 0 or at most 4 O's on the board, and at least 0 or at most 5 X's on
 * the board, assuming X always goes first.
 * \example 2: one may set this to {'L', 0, 1, 'l', 0, 1, 'G', 0, 2, 'g', 0, 2,
 * 'E', 0, 2, 'e', 0, 2, 'H', 0, 2, 'h', 0, 2, 'C', 0, 2, 'c', 0, 2, '-', 4, 11,
 * -2, 0, 2, 0, 2, 0, 2, -1} for the game of Dōbutsu shōgi. The triplets before
 * the -2 denote the pieces that may appear on the board, whereas each pair
 * between the -2 and the -1 denotes a type of piece that the forest player may
 * have captured and not yet placed back into the board. Since the total number
 * of each type of piece is fixed, there is no need to store the number of
 * pieces held by the sky player since we can figure it out by looking at the
 * board.
 * @param IsValidConfig Pointer to a user-defined configuration validation
 * function which returns true if the given piece configuration is valid based
 * on game rules. The system will determine if configuration is valid using this
 * function while performing an additional check on the total number of pieces,
 * which should add up to board_size. If NULL is passed to this value, a piece
 * configuration will be considered valid as long as the numbers of each type of
 * piece add up to board_size. \note A piece configuration is an integer array
 * of size \c m (number of board pieces plus number of unordered pieces.) Each
 * value in this array denotes the number of that type of piece currently
 * appearing in the game. The first \c n (number of board pieces) values in a
 * piece configuration array denotes the number of board pieces of each type.
 * These values have a one-to-one correspondance with the first \c n pieces in
 * the \p pieces_init_array in the same order. The rest of the \c (m-n) pieces
 * correspond to the unordered pieces initialized by the last \c (m-n) pairs of
 * values in the \p pieces_init_array. As in example 2, one valid piece
 * configuration would be the array [1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 4, 0, 0, 0],
 * where the last three zeros correspond to the unordered pieces.
 * @param label An unique integer label for the new Generic Hash Context. The
 * recommended way to manage multiple contexts in a tier game is to use the
 * \c Tier hash values as the labels on the corresponding contexts.
 * @return \c true on success,
 * @return \c false if \p player is not in the range [0, 2], a context was
 * already created under \p label, \p pieces_init_array is malformed, or any
 * error such as malloc failure occurred.
 */
bool GenericHashAddContext(int player, int board_size,
                           const int *pieces_init_array,
                           bool (*IsValidConfig)(const int *config),
                           int64_t label);

// --------- Convenience functions for working with only one context. ---------

/**
 * @brief Returns the number of positions in the only Generic Hash Context
 * defined.
 *
 * @return The number of positions in the only context defined if exactly one
 * Generic Hash Context has been created since the last reinitialization, or
 * @return -1 if no Generic Hash Context or more than one contexts has been
 * initialized.
 */
Position GenericHashNumPositions(void);

/**
 * @brief Hashes the given \p board with \p turn being the current player's
 * index (1 or 2) using the only Generic Hash Context defined.
 *
 * @param board Game board with pieces as a char array. If the only context was
 * initialized with unordered pieces, the count of each type of unordered pieces
 * should be concatenated to the end of the board string and the length of
 * \p board should be (board_size + m - n). See the documentation on
 * \c GenericHashAddContext for definitions of m and n. The counts will be
 * interpreted as 8-bit integers (int8_t).
 * @param turn May take values 1 or 2, indicating the player who's making the
 * move at the current position. This value gets ignored by the function if the
 * only Generic Hash Context was initialized in single-player mode.
 * @return Hash value of the given board, or
 * @return -1 if zero or more than one Generic Hash Context exists, the \p board
 * contains an invalid piece, \p turn is not in the range [1, 2], or if any
 * other errors such as malloc failure occurred.
 */
Position GenericHashHash(const char *board, int turn);

/**
 * @brief Unhashes the given \p hash to fill the given \p board with pieces
 * using the only Generic Hash Context defined.
 *
 * @note The caller is responsible for making sure that \p board has enough
 * space to hold board_size bytes as specified when the only Generic Hash
 * Context was initialized, or (board_size + m - n) bytes if the only context
 * was initialized with unordered pieces. See the documentation on
 * \c GenericHashAddContext for definitions of m and n.
 *
 * @param hash Hash of the position.
 * @param board Game board as a char array with enough space to hold board_size
 * bytes as specified when the only Generic Hash Context was initialized, or
 * (board_size + m - n) bytes if the only context was initialized with unordered
 * pieces. See the documentation on \c GenericHashAddContext for definitions of
 * m and n.
 * @return \c true on success, or
 * @return \c false if no Generic Hash Context or more than one contexts has
 * been initialized, or the given hash is outside of the range of Position hash
 * of the only Generic Hash Context.
 */
bool GenericHashUnhash(Position hash, char *board);

/**
 * @brief Returns whose turn it is at the Position represented by the given
 * \p hash value using the only Generic Hash Context defined.
 *
 * @param hash Hash of the position.
 * @return 1 if the hashed position represents player 1's turn, 2 if player 2's
 * turn, or -1 if no Generic Hash Context or more than one context has been
 * initialized. Note that if the only context was initialized with a single
 * player, this function will always return the predefined turn value passed
 * into the \c GenericHashAddContext function to initialize the context.
 */
int GenericHashGetTurn(Position hash);

// ------------------------- Multi-context functions. -------------------------

/**
 * @brief Returns the number of positions in the Generic Hash Context with label
 * \p context_label.
 *
 * @param context_label Label of the context.
 * @return Number of positions in the Context, or
 * @return -1 if the given \p context_label is invalid.
 */
Position GenericHashNumPositionsLabel(int64_t context_label);

/**
 * @brief Hashes the given \p board with \p turn using the Generic Hash Context
 * with label \p context_label.
 *
 * @param context_label Label of the Generic Hash Context to use.
 * @param board Game board with pieces as a char array. If the context of label
 * \p context_label was initialized with unordered pieces, the count of each
 * type of unordered pieces should be concatenated to the end of the board
 * string and the length of \p board should be (board_size + m - n). See the
 * documentation on \c GenericHashAddContext for definitions of m and n. The
 * counts will be interpreted as 8-bit integers (int8_t).
 * @param turn May take values 1 or 2, indicating the player who's making the
 * move at the current position. This value gets ignored by the function if the
 * context selected was initialized in single-player mode.
 * @return Hash value of the given board, or
 * @return -1 if \p context_label is invalid, the \p board contains an invalid
 * piece, \p turn is not in the range [1, 2], or if any other errors such as
 * malloc failure occurred.
 */
Position GenericHashHashLabel(int64_t context_label, const char *board,
                              int turn);

/**
 * @brief Unhashes the given \p hash to fill the given \p board with pieces
 * using the Generic Hash Context with label \p context_label.
 *
 * @param context_label Label of the Generic Hash Context to use.
 * @param hash Hash of the position.
 * @param board Game board as a char array with enough space to hold board_size
 * bytes as specified when the selected Generic Hash Context was initialized, or
 * (board_size + m - n) bytes if the context was initialized with unordered
 * pieces. See the documentation on \c GenericHashAddContext for definitions of
 * m and n.
 * @return \c true on success, or
 * @return \c false if \p context_label is invalid or the given hash is outside
 * of the range of Position hash of the Generic Hash Context selected.
 */
bool GenericHashUnhashLabel(int64_t context_label, Position hash, char *board);

/**
 * @brief Returns whose turn it is at the Position represented by the given
 * \p hash value using the Generic Hash Context with label \p context_label.
 *
 * @param context_label Label of the Generic Hash Context to use.
 * @param hash Hash of the position.
 * @return 1 if the hashed position is player 1's turn, 2 if player 2's turn, or
 * -1 if \p context_label is invalid. Note that if the selected Generic Hash
 * Context was initialized with a single player, this function will always
 * return the predefined turn value that was passed into the
 * \c GenericHashAddContext function to initialize the context.
 */
int GenericHashGetTurnLabel(int64_t context_label, Position hash);

#endif  // GAMESMANONE_CORE_GENERIC_HASH_GENERIC_HASH_H_
