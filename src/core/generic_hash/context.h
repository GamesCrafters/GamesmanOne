/**
 * @file context.h
 * @author Scott Lindeneau: designer of the original version.
 * @author Robert Shi (robertyishi@berkeley.edu): Adaptation to GamesmanOne,
 *         optimizations, and additional features.
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Generic Hash Context module used by the Generic Hash system.
 * @version 1.2.3
 * @date 2024-12-22
 *
 * @note This module is for Generic Hash system internal use only. The user of
 * the Generic Hash system should use the accessor functions provided in
 * generic_hash.h.
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

#ifndef GAMESMANONE_CORE_GENERIC_HASH_CONTEXT_H_
#define GAMESMANONE_CORE_GENERIC_HASH_CONTEXT_H_

#include "core/types/gamesman_types.h"

/**
 * @brief Describes the restrictions on pieces and board size of an hash
 * environment in which Generic Hashing is performed.
 *
 * @par Definitions:
 * The following definitions are used throughout the implementation
 * GenericHashContext.
 *
 * @par
 * Board pieces: pieces that appear on the board. Each slot on the game board is
 * considered unique, thus different orderings of the board pieces correspond to
 * different positions.
 *
 * @par
 * Unordered pieces: collection of pieces whose order does not matter. An
 * example would be pieces that are removed from the board and waiting to be put
 * back on in Dōbutsu shōgi.
 *
 * @par
 * Pieces array: a fixed-length char array representing each type of piece.
 * Predefined by the user of the Generic Hash system.
 *
 * @par
 * Piece configuration: an array of integers representing the number of each
 * type of piece on board. The length of the array is equal to the length of the
 * piece array (n) plus the number of types of pieces in the unordered section
 * (m-n). For 0 <= i < n, \c piece_config[i] represents the number of pieces[i].
 * For n <= i <= m, \c piece_config[i] corresponds to the number of the
 * (i-n)-th unordered piece.
 *
 * @par
 * Valid piece configuration: a piece configuration is valid for a hash context
 * if and only if
 * 1. the total number of board pieces is equal to \c board_size, and
 * 2. \c context.IsValidConfig(configuration) returns \c true. Note that the
 * user may choose not to enforce the first rule in their own \c IsValidConfig
 * function and assume that Generic Hash system performs the check for them.
 * Also note that the piece configurations are generated based on the
 * user-provided lower and upper bounds of each type of piece. Hence there is no
 * need to check the boundaries.
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
     * @brief May take values 0, 1, or 2. 0: Initialized in two-player mode and
     * a turn bit is added to the final hash to distinguish between first player
     * vs. second player's turn; 1: 1st Player only; 2: 2nd player only.
     */
    int player;

    /** @brief Number of types of pieces. */
    int num_pieces;

    /**
     * @brief Number of types of pieces that may appear in the unordered
     * section.
     */
    int num_unordered_pieces;

    /** @brief Array of all possible pieces, length == num_pieces. */
    char *pieces;

    /**
     * @brief Maps a piece character to an index into the PIECES array. Supports
     * up to 128 different pieces. A piece can be of any value from 0 to 127
     * which is then mapped to its index.
     */
    char piece_index_mapping[128];

    /**
     * @brief Minimum number of each type of piece, including board pieces and
     * unordered pieces. The first n values are aligned to the
     * \c GenericHashContext::pieces array. The last (m-n) values correspond to
     * the unordered pieces.
     */
    int *mins;

    /**
     * @brief Maximum number of each type of piece, including board pieces and
     * unordered pieces. The first n values are aligned to the
     * \c GenericHashContext::pieces array. The last (m-n) values correspond to
     * the unordered pieces.
     */
    int *maxs;

    /** @brief Returns \c true if and only if the given \p config is valid. */
    bool (*IsValidConfig)(const int *config);

    /**
     * @brief Number of positions in the current context. Equals
     * max_hash_value + 1.
     */
    Position num_positions;

    /** @brief Number of valid piece configurations in the current context. */
    int64_t num_valid_configs;

    /**
     * @brief Array of all valid configuration indices. Its length is equal to
     * num_valid_configs.
     */
    int64_t *valid_config_indices;

    /**
     * @brief The position hash offsets for each valid configuration aligned to
     * the valid_config_indices array. I.e., config_hash_offsets[i] represents
     * the total number of positions in all valid configurations with an index
     * smaller than i. Its length is equal to num_valid_configs.
     */
    Position *config_hash_offsets;

    /** @brief Number of piece configurations, including invalid ones. */
    int64_t num_configs;

    /**
     * @brief An array that maps indices of piece configurations to indices of
     * valid piece configurations.
     */
    int64_t *config_index_to_valid_index;

    /**
     * @brief An exclusive multiplicative scan of the
     * \c GenericHashContext::maxs array with each entry incremented by 1. That
     * is, the value at index 0 is 1, and the value at any other index i is the
     * product of the value at i-1 and (GenericHashContext::maxs[i-1] + 1).
     *
     * @details This is cached for generating rearrangement values in constant
     * time.
     */
    int64_t *max_piece_mult_scan;

    /**
     * @brief Rearrangement-indexed cache for the \c Rearrange function. See
     * definition of \c Rearrange in \link context.c for details.
     */
    int64_t *rearranger_cache;
} GenericHashContext;

/**
 * @brief Initializes the given Generic Hash \p context. Zeros out the contents
 * in \p context and returns \c false on failure.
 *
 * @note Initializing an already initialized \p context results in undefined
 * behavior and may cause memory leak.
 *
 * @param context Context to initialize, which is assumed to be uninitialized.
 * @param board_size Size of the game board. E.g. board_size == 9 in
 * Tic-Tac-Toe.
 * @param player May take values 0, 1, or 2. If set to 0, a two-player hash
 * context will be initialized, and \c GenericHashContextGetTurn will return the
 * turn based on the given hash value. If set to either 1 or 2, a single-player
 * hash context will be initialized, and \c GenericHashContextGetTurn will
 * always return that player's index regardless of the given hash value. E.g.
 * set this to 0 for the game of Chess, and set this to 1 for all tiers that
 * correspond to player X's turn in Tic-Tac-Toe.
 * @param pieces_init_array An integer array of the following format:
 * [p_1, L_1, U_1, p_2, L_2, U_2, ..., p_n, L_n, U_n,
 * (-2, L_{n+1}, U_{n+1}, L_{n+2}, U_{n+2}, ..., L_m, U_m,) -1] where
 * - The \c p_i 's are the characters associated with the pieces (including
 * blanks.) Min: 0; Max: 127.
 * - The \c L_i 's for i in range(1, n+1) are the minimum allowable number of
 * occurrences of each piece type on the board. Min: 0; Max: \c U_i.
 * - The \c U_i 's for i in range(1, n+1) are the maximum allowable number of
 * occurrences of each piece type on the board. Min: \c L_i; Max: board_size.
 * - (Optional, v1.2.0+) The value -2 is used to separate the board pieces from
 * the unordered pieces, if exist.
 * - (Optional, v1.2.0+) The \c L_j 's for j in range(n+1, m) are the minimum
 * allowable number of occurrences of each piece type that may appear in the
 * unordered section of the game. Min: 0; Max: \c U_j.
 * - (Optional, v1.2.0+) The \c U_j 's for j in range(n+1, m) are the maximum
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
 * @return \c true on success,
 * @return \c false otherwise.
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
Position GenericHashContextHash(GenericHashContext *context,
                                ReadOnlyString board, int turn);

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
 * predefined turn value if CONTEXT was initialized with a single player.
 */
int GenericHashContextGetTurn(GenericHashContext *context, Position hash);

#endif  // GAMESMANONE_CORE_GENERIC_HASH_CONTEXT_H_
