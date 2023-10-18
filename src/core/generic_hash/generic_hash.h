/**
 * @file generic_hash.h
 * @author Dan Garcia: Designer of the original (3-variable only) version
 * @author Attila Gyulassy: Developer of the original (3-variable only) version
 * @author Michel D'Sa: Designer and developer of user-specified variables
 * version
 * @author Scott Lindeneau: Designer and developer of multiple contexts;
 * optimized for efficiency and readability.
 * @author Robert Shi (robertyishi@berkeley.edu): further optimized for
 * efficiency and readability, adapted to the new system and implemented
 * thread-safety
 *         GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Generic Hash system for finite board games with fixed sets of pieces.
 * @version 1.01
 * @date 2023-09-27
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

#ifndef GAMESMANEXPERIMENT_CORE_GENERIC_HASH_GENERIC_HASH_H_
#define GAMESMANEXPERIMENT_CORE_GENERIC_HASH_GENERIC_HASH_H_

#include <stdint.h>

#include "core/gamesman_types.h"

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
 * player only mode. E.g., this value should be set to 0 for the game of chess.
 * @param board_size Size of the board. E.g., this value should be set to 9 for
 * the game to Tic-Tac-Toe.
 * @param pieces_init_array An integer array of the following format:
 * {p_1, L_1, U_1, p_2, L_2, U_2, ... , p_n, L_n, U_n, -1} where
 * - The $p_i$'s are the characters associated with the pieces (including
 * blanks)
 * - The $L_i$'s are the minimum allowable number of occurrences of each piece
 *   type on the board.
 * - The $U_i$'s are the maximum allowable number of occurrences of each piece
 *   type on the board.
 * - The $-1$ is used to mark the end of the array.
 * E.g., set this to {'-', 0, 9, 'O', 0, 4, 'X', 0, 5, -1} for the game of
 * Tic-Tac-Toe. Explanation: there can be at least 0 or at most 9 blank slots,
 * there can be at least 0 or at most 4 O's on the board, and there can be at
 * least 0 or at most 5 X's on the board, assuming X always goes first.
 * @param IsValidConfiguration Pointer to a user-defined configuration
 * validation function which returns true if the given piece configuration is
 * valid based on game rules. The system will determine if configuration is
 * valid using this function while performing an additional check on the total
 * number of pieces, which should add up to board_size. If NULL is passed to
 * this value, a piece configuration will be considered valid as long as the
 * numbers of each type of piece add up to board_size.
 * @param label An unique integer label for the new Generic Hash Context. The
 * recommended way to manage multiple contexts in a tier game is to use the
 * Tier values as the labels on the corresponding contexts.
 * @return true on success,
 * @return false if PLAYER is not in the range [0, 2], a context was already
 * created under LABEL, PIECES_INIT_ARRAY is malformed, or any error such as
 * malloc failure occurred.
 */
bool GenericHashAddContext(
    int player, int board_size, const int *pieces_init_array,
    bool (*IsValidConfiguration)(const int *configuration), int64_t label);

// --------- Convenience functions for working with only one context. ---------

/**
 * @brief Returns the number of positions in the only Generic Hash Context
 * defined.
 *
 * @return The number of positions in the only context defined if exactly one
 * Generic Hash Context has been created since the last reinitialization. -1
 * if zero or more than one Generic Hash Context exists.
 */
Position GenericHashNumPositions(void);

/**
 * @brief Hashes the given BOARD with TURN using the only Generic Hash Context
 * defined.
 *
 * @param board Game board with pieces as a char array.
 * @param turn May take values 1 or 2, indicating the player who's making the
 * move at the current position. This value gets ignored by the function if the
 * only Generic Hash Context was initialized in single-player mode.
 * @return Hash value of the given board. -1 if zero or more than one Generic
 * Hash Context exists, the BOARD contains an invalid piece, TURN is not in the
 * range [1, 2], or if any other errors such as malloc failure occurred.
 */
Position GenericHashHash(ReadOnlyString board, int turn);

/**
 * @brief Unhashes the given HASH to fill the given BOARD with pieces using the
 * only Generic Hash Context defined.
 *
 * @note BOARD is assumed to have enough space to hold board_size bytes as
 * specified when the only Generic Hash Context was initialized. Calling this
 * function with an invalid BOARD or when BOARD does not have enough space
 * results in undefined behavior.
 *
 * @param hash Hash of the position.
 * @param board Game board as a char array with enough space to hold board_size
 * bytes as specified when the only Generic Hash Context was initialized.
 * @return true on success,
 * @return false if zero or more than one Generic Hash Context exists, or the
 * given hash is out of the Position range of the only Generic Hash Context.
 */
bool GenericHashUnhash(Position hash, char *board);

/**
 * @brief Returns whose turn it is at the Position represented by the given HASH
 * value using the only Generic Hash Context defined.
 *
 * @param hash Hash of the position.
 * @return 1 if the HASHed position is player 1's turn, 2 if player 2's turn, or
 * -1 if zero or more than one Generic Hash Context exists. Note that if the
 * only context was initialized with a single player, this function will always
 * return the predefined turn value passed into GenericHashAddContext().
 */
int GenericHashGetTurn(Position hash);

// ------------------------- Multi-context functions. -------------------------

/**
 * @brief Returns the number of positions in the Generic Hash Context with label
 * CONTEXT_LABEL.
 *
 * @param context_label Label of the Context.
 * @return Number of positions in the Context, or -1 if the given LABEL is
 * invalid.
 */
Position GenericHashNumPositionsLabel(int64_t context_label);

/**
 * @brief Hashes the given BOARD with TURN using the Generic Hash Context with
 * label CONTEXT_LABEL.
 *
 * @param context_label Label of the Generic Hash Context to use.
 * @param board Game board with pieces as a char array.
 * @param turn May take values 1 or 2, indicating the player who's making the
 * move at the current position. This value gets ignored by the function if the
 * Generic Hash Context selected was initialized in single-player mode.
 * @return Hash value of the given board. -1 if CONTEXT_LABEL is invalid, the
 * BOARD contains an invalid piece, TURN is not in the range [1, 2], or if any
 * other errors such as malloc failure occurred.
 */
Position GenericHashHashLabel(int64_t context_label, ReadOnlyString board,
                              int turn);

/**
 * @brief Unhashes the given HASH to fill the given BOARD with pieces using the
 * Generic Hash Context with label CONTEXT_LABEL.
 *
 * @param context_label Label of the Generic Hash Context to use.
 * @param hash Hash of the position.
 * @param board Game board as a char array with enough space to hold board_size
 * bytes as specified when the selected Generic Hash Context was initialized.
 * @return true on success,
 * @return false if CONTEXT_LABEL is invalid, the given hash is out of the
 * Position range of the Generic Hash Context selected.
 */
bool GenericHashUnhashLabel(int64_t context_label, Position hash, char *board);

/**
 * @brief Returns whose turn it is at the Position represented by the given HASH
 * value using the Generic Hash Context with label CONTEXT_LABEL.
 *
 * @param context_label Label of the Generic Hash Context to use.
 * @param hash Hash of the position.
 * @return 1 if the HASHed position is player 1's turn, 2 if player 2's turn, or
 * -1 if CONTEXT_LABEL is invalid. Note that if the Generic Hash Context was
 * initialized with a single player, this function will always return the
 * predefined turn value passed into GenericHashAddContext().
 */
int GenericHashGetTurnLabel(int64_t context_label, Position hash);

#endif  // GAMESMANEXPERIMENT_CORE_GENERIC_HASH_GENERIC_HASH_H_
