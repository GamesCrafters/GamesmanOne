/**
 * @file context.h
 * @author Robert Shi (robertyishi@berkeley.edu)
 *         GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Generic Hash Context module used by the Generic Hash system.
 * @version 1.0
 * @date 2023-08-19
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

#ifndef GAMESMANEXPERIMENT_CORE_GENERIC_HASH_CONTEXT_H_
#define GAMESMANEXPERIMENT_CORE_GENERIC_HASH_CONTEXT_H_

#include "core/gamesman_types.h"

/**
 * @brief Describes the restrictions on pieces and board size of an hash
 * environment in which Generic Hashing is performed.
 *
 * @par Definitions:
 * The following definitions are used throughout the implementation
 * GenericHashContext.
 *
 * @par
 * Piece array: a fixed-length char array representing each type of piece.
 * Predefined by the user of the Generic Hash system.
 *
 * @par
 * Piece configuration: an array of integers representing the number of each
 * type of piece on board. The length of the array is equal to the length of the
 * piece array and piece_config[i] represents the number of pieces[i].
 *
 * @par
 * Valid piece configuration: a piece configuration is valid for a hash context
 * if and only if 1. the total number of pieces is equal to board_size and
 * 2. context.IsValidConfig(configuration) returns true. Note that the user may
 * choose not to enforce the first rule in their own IsValidConfig() function.
 *
 * @par
 * Piece configuration index: an unique integer assigned to a piece
 * configuration. This integer is calculated based on the minimum and maximum
 * number of each type of piece as specified by the user and is very similar
 * to a "hash" value. To avoid confusion with game position hashing, we choose
 * the name "index" instead.
 */
typedef struct GenericHashContext {
    /**
     * @brief Size of the board.
     * @example board_size == 9 for tic-tac-toe.
     */
    int board_size;

    /**
     * @brief May take values 0, 1, or 2. 0: Both Player boards (default),
     * 1: 1st Player only, 2: 2nd player only.
     */
    int player;

    /** @brief Number of types of pieces. */
    int num_pieces;

    /** @brief Array of all possible pieces, length == num_pieces. */
    char *pieces;

    /**
     * @brief Maps a piece character to an index into the PIECES array. Supports
     * up to 128 different pieces. A piece can be of any value between 0 to 127,
     * which is then mapped to its index.
     */
    char piece_index_mapping[128];

    /** @brief Min number of each type of piece, aligned to the pieces array. */
    int *mins;

    /** @brief Max number of each type of piece, aligned to the pieces array. */
    int *maxs;

    /** @brief Returns true iff the given CONFIG is valid. */
    bool (*IsValidConfig)(const int *config);

    /**
     * @brief Number of positions in the current context. Equals
     * max_hash_value + 1, similar to global_num_positions.
     */
    Position num_positions;

    /** @brief Number of valid piece configurations in the current context. */
    int64_t num_valid_configs;

    /**
     * @brief Array of all valid configuration indices. Its length equals
     * num_valid_configs.
     */
    int64_t *valid_config_indices;

    /**
     * @brief The position hash offsets for each valid configuration aligned to
     * the valid_config_indices array. I.e., config_hash_offsets[i] represents
     * the total number of positions in all valid configurations with an index
     * smaller than i. Its length equals num_valid_configs.
     */
    Position *config_hash_offsets;
} GenericHashContext;

/**
 * @brief Initializes the given Generic Hash CONTEXT and returns true. Zeros out
 * the contents in CONTEXT and returns false on failure.
 *
 * @note Initializing an already initialized CONTEXT results in undefined
 * behavior.
 *
 * @param context Context to initialize, which is assumed to be uninitialized.
 * @param board_size Size of the game board. E.g. board_size == 9 in
 * Tic-Tac-Toe.
 * @param player May take values 0, 1, or 2. If set to 0, a two-player hash
 * context will be initialized, and GenericHashContextGetTurn() will return the
 * turn based on the given hash value. If set to either 1 or 2, a single-player
 * hash context will be initialized, and GenericHashContextGetTurn() will always
 * return that player's index regardless of the given hash value. E.g. set this
 * to 0 for the game of Chess.
 * @param pieces_init_array An integer array of the following format:
 * {p_1, L_1, U_1, p_2, L_2, U_2, ... , p_n, L_n, U_n, -1} where
 * - The $p_i$'s are the characters associated with the pieces (including
 * blanks)
 * - The $L_i$'s are the minimum allowable number of occurrences of each piece
 *   type on the board.
 * - The $U_i$'s are the maximum allowable number of occurrences of each piece
 *   type on the board.
 * - The $-1$ is used to mark the end of the array.
 * @param IsValidConfig Pointer to a user-defined configuration validation
 * function which returns true if the given piece configuration is valid based
 * on game rules. The system will determine if configuration is valid using this
 * function while performing an additional check on the total number of pieces,
 * which should add up to board_size. If NULL is passed to this value, a piece
 * configuration will be considered valid as long as the numbers of each type of
 * piece add up to board_size.
 * @return true on success,
 * @return false otherwise.
 */
bool GenericHashContextInit(GenericHashContext *context, int board_size,
                            int player, const int *pieces_init_array,
                            bool (*IsValidConfig)(const int *config));

/**
 * @brief Deallocates the given Generic Hash CONTEXT.
 */
void GenericHashContextDestroy(GenericHashContext *context);

/**
 * @brief Returns the total number of positions in the given Generic Hash
 * CONTEXT. Assumes context is initialized.
 */
Position GenericHashContextNumPositions(const GenericHashContext *context);

/**
 * @brief Hashes BOARD along with current TURN using the given Generic Hash
 * CONTEXT. Assumes context is initialized.
 *
 * @param context Hash using this Generic Hash Context.
 * @param board Game board with pieces on it. The pieces are assumed to be
 * defined using the pieces_init_array when the given CONTEXT is initialized.
 * Otherwise, the function returns -1.
 * @param turn Current turn. This value is ignored if CONTEXT was initialized
 * with a single-player.
 * @return Hash of the given BOARD and TURN. -1 if an error occurred.
 */
Position GenericHashContextHash(GenericHashContext *context, const char *board,
                                int turn);

/**
 * @brief Unhashes the given HASH value using the given Generic Hash CONTEXT and
 * fills the given BOARD.
 *
 * @note CONTEXT is assumed to be initialized and BOARD is assumed to have
 * enough space to hold at least CONTEXT.board_size bytes. Otherwise, calling
 * this function results in undefined behavior.
 *
 * @param context Generic Hash Context to use.
 * @param hash Hash value.
 * @param board Unhash to this board.
 * @return true on success,
 * @return false otherwise.
 */
bool GenericHashContextUnhash(GenericHashContext *context, Position hash,
                              char *board);

/**
 * @brief Returns whose turn it is at the Position represented by the given HASH
 * value using the given Generic Hash CONTEXT.
 *
 * @return 1 if it's player 1's turn, 2 if it's player 2's turn. Returns the
 * predefined turn value if CONTEXT was initialized with a single-player.
 */
int GenericHashContextGetTurn(GenericHashContext *context, Position hash);

#endif  // GAMESMANEXPERIMENT_CORE_GENERIC_HASH_CONTEXT_H_