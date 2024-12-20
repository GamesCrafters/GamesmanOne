/**
 * @file context.c
 * @author Scott Lindeneau: designer of the original version.
 * @author Robert Shi (robertyishi@berkeley.edu): Adaptation to GamesmanOne,
 *         optimizations, and additional features.
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Implementation of the Generic Hash Context module.
 * @version 1.2.2
 * @date 2024-12-20
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

#include "core/generic_hash/context.h"

#include <assert.h>   // assert
#include <stdbool.h>  // bool
#include <stdint.h>   // INT8_MAX, int64_t
#include <stdio.h>    // fprintf, stderr
#include <stdlib.h>   // calloc, free
#include <string.h>   // memset, memcpy

#include "core/misc.h"

// At most 128 pieces, 128 additional slots for pieces in the unordered section.
#define STACK_CONFIG_SIZE 256

// ========================== Common Helper Functions ==========================

/**
 * @brief Returns the number of possible piece numbers of piece index
 * \p piece_index in \p context.
 */
static int GetNumPossiblePieceNumbers(const GenericHashContext *context,
                                      int piece_index) {
    return context->maxs[piece_index] - context->mins[piece_index] + 1;
}

/** @brief Converts the piece configuration array \p config into its index. */
static int64_t ConfigToIndex(GenericHashContext *context, int *config) {
    int64_t index = 0;
    int num_total_pieces = context->num_pieces + context->num_unordered_pieces;
    for (int i = num_total_pieces - 1; i >= 0; --i) {
        index *= GetNumPossiblePieceNumbers(context, i);
        index += config[i] - context->mins[i];
    }

    return index;
}

/**
 * @brief Expands the piece configuration array of configuration index \p index
 * into \p config.
 */
static void IndexToConfig(GenericHashContext *context, int64_t index,
                          int *config) {
    int num_total_pieces = context->num_pieces + context->num_unordered_pieces;
    for (int i = 0; i < num_total_pieces; ++i) {
        int num_possible_piece_numbers = GetNumPossiblePieceNumbers(context, i);
        config[i] = context->mins[i] + (index % num_possible_piece_numbers);
        index /= num_possible_piece_numbers;
    }
}

/**
 * @brief Converts the given piece configuration array \p config into its
 * rearrangement index, which is defined in the documentation for the
 * \c Rearrange function.
 */
static int64_t ConfigToRearrangement(GenericHashContext *context, int *config) {
    int64_t ret = 0;
    for (int i = 0; i < context->num_pieces; ++i) {
        ret += context->max_piece_mult_scan[i] * config[i];
    }

    return ret;
}

/**
 * @details 2024-02-18: this new version added in a new parameter,
 * \p rearrangement, which is an integer that corresponds to the index of the
 * piece configuration in an array of configurations that disregards the lower
 * bound for each type of piece. This new variable is defined to allow return
 * values from this function to be cached inside the \p context.
 *
 * The \p rearrangement variable is defined in the way described above because
 * during hashing/unhashing, pieces are removed/placed one by one from/onto the
 * board, which results in piece configurations with some types of pieces having
 * fewer than minimum allowed number of pieces on an effectively smaller board
 * (the unprocessed region of the original board). So to safely cache return
 * values from this function, we need to make sure that we allocate space for
 * these new "piece configurations". To avoid confusion, these new
 * "configurations" are hashed and stored as "rearrangement" values which are
 * 64-bit integers. Another justification is that it's more expensive to use the
 * piece configurations as keys to the cache map. It makes sense to use these
 * rearrangement values as keys to the mapping array.
 */
static int64_t Rearrange(GenericHashContext *context, const int *config,
                         int rearrangement) {
    if (context->rearranger_cache[rearrangement] < 0) {
        int64_t pieces_rearranged = 0, result = 1;
        for (int piece_index = 0; piece_index < context->num_pieces - 1;
             ++piece_index) {
            pieces_rearranged += config[piece_index];
            int64_t more_pieces = config[piece_index + 1];
            int64_t combinations =
                NChooseR(pieces_rearranged + more_pieces, pieces_rearranged);
            result *= combinations;
        }
        context->rearranger_cache[rearrangement] = result;
    }

    return context->rearranger_cache[rearrangement];
}

// ========================== GenericHashContextInit ==========================

static void InitStep0MemsetToDefaultValues(GenericHashContext *context) {
    memset(context, 0, sizeof(*context));
    memset(context->piece_index_mapping, -1,
           sizeof(context->piece_index_mapping));
}

static void InitStep1_0CountNumPieces(GenericHashContext *context,
                                      const int *pieces_init_array) {
    // format: [p_1, L_1, U_1, p_2, L_2, U_2, ..., p_n, L_n, U_n,
    // (-2, L_{n+1}, U_{n+1}, L_{n+2}, U_{n+2}, ..., L_m, U_m,) -1]
    int i;

    // Find the separator between board pieces and unordered pieces.
    for (i = 0; pieces_init_array[i] >= 0; i += 3) {
        ++context->num_pieces;
    }

    context->num_unordered_pieces = 0;
    if (pieces_init_array[i] == -1) {
        // No unordered pieces.
        return;
    }

    // Count the number of unordered pieces.
    for (i = i + 1; pieces_init_array[i] >= 0; i += 2) {
        ++context->num_unordered_pieces;
    }
}

static bool InitStep1_1AllocateSpace(GenericHashContext *context) {
    context->pieces = (char *)calloc(context->num_pieces, sizeof(char));
    int num_total = context->num_pieces + context->num_unordered_pieces;
    context->mins = (int *)calloc(num_total, sizeof(int));
    context->maxs = (int *)calloc(num_total, sizeof(int));
    // Successfully calloc'ed spaces will be freed by caller.
    if (context->pieces == NULL) return false;
    if (context->mins == NULL) return false;
    if (context->maxs == NULL) return false;

    return true;
}

static bool InitStep1_2SetupBoardPieces(GenericHashContext *context,
                                        const int *pieces_init_array) {
    static ConstantReadOnlyString kIllegalPieceErrorFormat =
        "GenericHashContextInit: out-of-bounds piece symbol [0x%x] detected in "
        "pieces initialization array. Aborting...\n";
    static ConstantReadOnlyString kNegPieceErrorFormat =
        "GenericHashContextInit: unsupported negative piece char "
        "[0x%x] detected in pieces initialization array. Aborting...\n";
    static ConstantReadOnlyString kDupPieceErrorFormat =
        "GenericHashContextInit: piece char '%c'(0x%x) appeared "
        "twice in pieces initialization array. Aborting...\n";
    static ConstantReadOnlyString kPieceBoundsErrorFormat =
        "GenericHashContextInit: piece char '%c'(0x%x) has malformed min/max "
        "bounds in the pieces initialization array. Aborting...\n";

    for (int i = 0; pieces_init_array[i * 3] >= 0; ++i) {
        int piece = pieces_init_array[i * 3];
        if (piece > INT8_MAX) {  // Max piece symbol value is INT8_MAX.
            fprintf(stderr, kIllegalPieceErrorFormat, piece);
            return false;
        } else if (piece < 0) {  // Negative piece symbols are not allowed.
            fprintf(stderr, kNegPieceErrorFormat, piece);
            return false;
        } else if (context->piece_index_mapping[piece] != -1) {
            // Repeated definition of the same piece symbol is not allowed.
            fprintf(stderr, kDupPieceErrorFormat, (char)piece, piece);
            return false;
        }

        context->pieces[i] = (char)piece;
        context->piece_index_mapping[piece] = i;
        context->mins[i] = pieces_init_array[i * 3 + 1];
        context->maxs[i] = pieces_init_array[i * 3 + 2];
        if (context->mins[i] < 0 || context->mins[i] > context->maxs[i] ||
            context->maxs[i] < 0) {  // Malformed min/max piece number(s).
            fprintf(stderr, kPieceBoundsErrorFormat, (char)piece, piece);
            return false;
        }
    }

    return true;
}

static bool InitStep1_3SetupUnorderedPieces(GenericHashContext *context,
                                            const int *pieces_init_array) {
    static ConstantReadOnlyString kMalformedBoundsErrorFormat =
        "GenericHashContextInit: malformed min/max bounds detected in the "
        "unordered pieces section of the pieces initialization array. The "
        "index "
        "of the unordered piece type is %d. Aborting...\n";
    static ConstantReadOnlyString kOutOfBoundsErrorFormat =
        "GenericHashContextInit: out-of-bounds min/max value(s) detected in "
        "the unordered pieces section of the pieces initialization array. The "
        "index of the unordered piece type is %d. Aborting...\n";

    // No unordered pieces to set up.
    if (context->num_unordered_pieces == 0) return true;

    int offset = context->num_pieces * 3 + 1;
    for (int i = 0; pieces_init_array[offset + i * 2] >= 0; ++i) {
        // Global index of the piece in the mins and the maxs array.
        int j = i + context->num_pieces;
        context->mins[j] = pieces_init_array[offset + i * 2];
        context->maxs[j] = pieces_init_array[offset + i * 2 + 1];
        if (context->mins[j] < 0 || context->mins[j] > context->maxs[j] ||
            context->maxs[j] < 0) {  // Malformed min/max piece number(s).
            fprintf(stderr, kMalformedBoundsErrorFormat, i);
            return false;
        } else if (context->mins[j] > INT8_MAX || context->maxs[j] > INT8_MAX) {
            fprintf(stderr, kOutOfBoundsErrorFormat, i);
            return false;
        }
    }

    return true;
}

static bool InitStep1SetupPiecesAndIndexMapping(GenericHashContext *context,
                                                const int *pieces_init_array) {
    // Count the number of pieces.
    InitStep1_0CountNumPieces(context, pieces_init_array);

    // Allocate space.
    bool success = InitStep1_1AllocateSpace(context);
    if (!success) return false;

    // Setup board pieces.
    success = InitStep1_2SetupBoardPieces(context, pieces_init_array);
    if (!success) return false;

    // Setup unordered pieces.
    return InitStep1_3SetupUnorderedPieces(context, pieces_init_array);
}

static int64_t InitStep2_0CountNumConfigs(GenericHashContext *context) {
    int64_t ret = 1;
    int num_total_pieces = context->num_pieces + context->num_unordered_pieces;
    for (int i = 0; i < num_total_pieces; ++i) {
        int64_t num_possible_piece_numbers =
            GetNumPossiblePieceNumbers(context, i);
        ret = SafeMultiplyNonNegativeInt64(ret, num_possible_piece_numbers);
    }

    return ret;
}

static int64_t InitStep2_1CountNumRearrangements(GenericHashContext *context) {
    int64_t ret = 1;
    for (int i = 0; i < context->num_pieces; ++i) {
        ret = SafeMultiplyNonNegativeInt64(ret, context->maxs[i] + 1);
    }

    return ret;
}

static bool IsValidConfigWrapper(GenericHashContext *context, int *config) {
    // Check if all board pieces add up to context->board_size.
    int pieces = 0;
    for (int i = 0; i < context->num_pieces; ++i) {
        pieces += config[i];
    }
    if (pieces != context->board_size) return false;
    if (context->IsValidConfig != NULL) return context->IsValidConfig(config);

    return true;
}

static void InitStep2_2CountNumValidConfigs(GenericHashContext *context,
                                            int64_t num_configs) {
    int this_config[STACK_CONFIG_SIZE];
    for (int64_t i = 0; i < num_configs; ++i) {
        IndexToConfig(context, i, this_config);

        // Increment if and only if this_config is valid.
        context->num_valid_configs +=
            IsValidConfigWrapper(context, this_config);
    }
}

static bool InitStep2_3InitSpaces(GenericHashContext *context,
                                  int64_t num_rearrangements) {
    context->valid_config_indices =
        (int64_t *)calloc(context->num_valid_configs, sizeof(int64_t));
    if (context->valid_config_indices == NULL) return false;

    context->config_index_to_valid_index =
        (int64_t *)calloc(context->num_configs, sizeof(int64_t));
    if (context->config_index_to_valid_index == NULL) return false;

    context->config_hash_offsets =
        (Position *)calloc(context->num_valid_configs, sizeof(Position));
    if (context->config_hash_offsets == NULL) return false;

    context->max_piece_mult_scan =
        (int64_t *)calloc(context->num_pieces, sizeof(int64_t));
    if (context->max_piece_mult_scan == NULL) return false;
    context->max_piece_mult_scan[0] = 1;
    for (int i = 1; i < context->num_pieces; ++i) {
        context->max_piece_mult_scan[i] =
            context->max_piece_mult_scan[i - 1] * (context->maxs[i - 1] + 1);
    }

    context->rearranger_cache =
        (int64_t *)calloc(num_rearrangements, sizeof(int64_t));
    if (context->rearranger_cache == NULL) return false;
    for (int64_t i = 0; i < num_rearrangements; ++i) {
        context->rearranger_cache[i] = -1;
    }

    return true;
}

/** @brief Rearrange with integer overflow checks. */
static int64_t SafeRearrange(GenericHashContext *context, const int *config) {
    int64_t pieces_rearranged = 0, result = 1;
    for (int piece_index = 0; piece_index < context->num_pieces - 1;
         ++piece_index) {
        pieces_rearranged += config[piece_index];
        int64_t more_pieces = config[piece_index + 1];
        int64_t combinations =
            NChooseR(pieces_rearranged + more_pieces, pieces_rearranged);
        result = SafeMultiplyNonNegativeInt64(result, combinations);
    }
    return result;
}

// Calculates the size of each valid configuration and add them up to get
// num_positions.
static bool InitStep2_4CalculateSizes(GenericHashContext *context,
                                      int64_t num_configs) {
    int64_t j = 0;
    int this_config[STACK_CONFIG_SIZE];
    for (int64_t i = 0; i < num_configs; ++i) {
        IndexToConfig(context, i, this_config);
        if (IsValidConfigWrapper(context, this_config)) {
            context->valid_config_indices[j] = i;
            context->config_index_to_valid_index[i] = j;
            context->config_hash_offsets[j] = context->num_positions;
            context->num_positions = SafeAddNonNegativeInt64(
                context->num_positions, SafeRearrange(context, this_config));
            ++j;
        } else {
            context->config_index_to_valid_index[i] = -1;
        }
    }
    assert(j == context->num_valid_configs);

    // Add the turn bit if there are two players.
    if (context->player == 0) {
        context->num_positions =
            SafeMultiplyNonNegativeInt64(context->num_positions, 2);
    }

    if (context->num_positions < 0) {
        fprintf(stderr,
                "GenericHashContextInit: too many positions to be represented "
                "using the current Position type. Aborting...\n");
        return false;
    }

    return true;
}

static bool InitStep2SetValidConfigs(GenericHashContext *context) {
    int64_t num_configs = InitStep2_0CountNumConfigs(context);
    int64_t num_rearrangements = InitStep2_1CountNumRearrangements(context);
    if (num_configs < 0 || num_rearrangements < 0) {
        fprintf(stderr,
                "GenericHashContextInit: too many possible piece "
                "rearrangements for the 64-bit integer type. Aborting...\n");
        return false;
    }

    context->num_configs = num_configs;
    InitStep2_2CountNumValidConfigs(context, num_configs);
    if (!InitStep2_3InitSpaces(context, num_rearrangements)) return false;
    return InitStep2_4CalculateSizes(context, num_configs);
}

bool GenericHashContextInit(GenericHashContext *context, int board_size,
                            int player, const int *pieces_init_array,
                            bool (*IsValidConfig)(const int *config)) {
    // PLAYER must be 0, 1, or 2.
    if (player < 0 || player > 2) return false;

    // Throughout the whole initialization process, all pointers should remain
    // NULL if they are not allocated yet. Then, on failure, we can safely call
    // GenericHashContextDestroy to destroy to context.
    InitStep0MemsetToDefaultValues(context);

    context->board_size = board_size;
    context->player = player;

    // This sets up num_pieces, pieces, piece_index_mapping, mins, and maxs
    if (!InitStep1SetupPiecesAndIndexMapping(context, pieces_init_array)) {
        GenericHashContextDestroy(context);
        return false;
    }
    context->IsValidConfig = IsValidConfig;

    // This sets up num_positions, num_valid_configs, valid_config_indices,
    // config_hash_offsets, num_configs, config_index_to_valid_index, and
    // rearranger_cache.
    if (!InitStep2SetValidConfigs(context)) {
        GenericHashContextDestroy(context);
        return false;
    }

    return true;
}

// ========================= GenericHashContextDestroy =========================

void GenericHashContextDestroy(GenericHashContext *context) {
    // Assumes all non-NULL pointers were calloc'ed.
    free(context->pieces);
    free(context->mins);
    free(context->maxs);
    free(context->valid_config_indices);
    free(context->config_index_to_valid_index);
    free(context->config_hash_offsets);
    free(context->max_piece_mult_scan);
    free(context->rearranger_cache);
    InitStep0MemsetToDefaultValues(context);
}

// ====================== GenericHashContextNumPositions ======================

Position GenericHashContextNumPositions(const GenericHashContext *context) {
    return context->num_positions;
}

// ========================== GenericHashContextHash ==========================

/** @brief Returns the index of the given \p piece in \p context. */
static int PieceToIndex(GenericHashContext *context, char piece) {
    if (piece < 0) return -1;

    return context->piece_index_mapping[(int)piece];
}

static bool HashStep0Initialize(GenericHashContext *context,
                                ReadOnlyString board, int *config) {
    // Count the number of board pieces of each type.
    memset(config, 0, sizeof(int) * context->num_pieces);
    for (int i = 0; i < context->board_size; ++i) {
        int piece_index = PieceToIndex(context, board[i]);
        if (piece_index < 0) {
            fprintf(
                stderr,
                "HashStep0Initialize: invalid piece '%c'(0x%x) at board[%d]\n",
                board[i], board[i], i);
            return false;
        }
        assert(piece_index < context->num_pieces);
        ++config[piece_index];
    }

    // Convert the number of unordered pieces of each type.
    for (int i = 0; i < context->num_unordered_pieces; ++i) {
        config[context->num_pieces + i] = (int)board[context->board_size + i];
    }

    return true;
}

static int64_t HashStep1FindIndexInValidConfigs(GenericHashContext *context,
                                                int *config) {
    // Validate the given config and find its hash offset.
    int64_t config_index = ConfigToIndex(context, config);
    if (config_index < 0 || config_index >= context->num_configs) return -1;

    return context->config_index_to_valid_index[config_index];
}

static Position HashStep2HashCruncher(GenericHashContext *context,
                                      ReadOnlyString board, int *config) {
    Position final_hash = 0;
    int64_t rearrangement = ConfigToRearrangement(context, config);

    // The loop ends with i == 0 because there will be
    // only one way to place the last piece.
    for (int i = context->board_size - 1; i > 0; --i) {
        // Find the index corresponding to the type of piece at board[i].
        int piece_index = PieceToIndex(context, board[i]);

        // For each piece that has a rank smaller than the current piece...
        for (int j = 0; j < piece_index; ++j) {
            // If we still have any pieces of this smaller rank...
            if (config[j] > 0) {
                // Take this piece out and rearrange the rest of the pieces on
                // the remaining slots on the board. Add the number of
                // rearrangements to our final hash.
                --config[j];
                int64_t new_rearrangement =
                    rearrangement - context->max_piece_mult_scan[j];
                int64_t num_rearrangements =
                    Rearrange(context, config, new_rearrangement);
                assert(num_rearrangements >= 0);
                final_hash += num_rearrangements;
                ++config[j];
            }
        }
        // Finished analyzing the current piece. "Recursively" hash the rest of
        // the pieces on board.
        --config[piece_index];
        rearrangement -= context->max_piece_mult_scan[piece_index];
    }

    return final_hash;
}

Position GenericHashContextHash(GenericHashContext *context,
                                ReadOnlyString board, int turn) {
    int config[STACK_CONFIG_SIZE];
    if (!HashStep0Initialize(context, board, config)) return -1;
    int64_t index = HashStep1FindIndexInValidConfigs(context, config);
    if (index < 0) {
        fprintf(stderr,
                "GenericHashContextHash: invalid piece configuration\n");
        return -1;
    }

    // hash_without_turn = offset_for_config + hash(board)
    Position hash = context->config_hash_offsets[index];
    hash += HashStep2HashCruncher(context, board, config);

    // The final hash contains no turn bit if there is only one player.
    if (context->player != 0) return hash;

    // Otherwise, validate the given turn value and append the turn bit.
    if (turn != 1 && turn != 2) return -1;
    return (hash << 1) | (turn == 2);
}

// ========================= GenericHashContextUnhash =========================

/**
 * @brief Returns the index of the largest element that is smaller than or equal
 * to TARGET in ARRAY, which is assumed to be sorted in non-decreasing order.
 * Returns -1 if no such element exists.
 */
static int64_t FindLargestSmallerEqual(int64_t *array, int64_t size,
                                       int64_t target) {
    int64_t left = 0;
    int64_t right = size - 1;
    int64_t result = -1;
    while (left <= right) {
        int64_t mid = left + (right - left) / 2;
        if (array[mid] <= target) {
            result = mid;
            left = mid + 1;
        } else {
            right = mid - 1;
        }
    }
    return result;
}

static void UnhashStep0HashUncruncher(GenericHashContext *context,
                                      Position hash, int *config, char *board) {
    int64_t rearrangement = ConfigToRearrangement(context, config);

    // Assuming hash is within context->num_positions. Therefore,
    // integer overflow should not occur.
    for (int i = context->board_size - 1; i >= 0; --i) {
        int64_t prev_offset = 0, curr_offset = 0;
        int index_of_piece_to_place = 0;
        for (int j = 0; (curr_offset <= hash) && (j < context->num_pieces);
             ++j) {
            assert(config[j] >= 0);
            if (config[j] == 0) continue;
            prev_offset = curr_offset;
            --config[j];
            int new_rearrangement =
                rearrangement - context->max_piece_mult_scan[j];
            curr_offset += Rearrange(context, config, new_rearrangement);
            ++config[j];
            index_of_piece_to_place = j;
        }
        --config[index_of_piece_to_place];
        rearrangement -= context->max_piece_mult_scan[index_of_piece_to_place];
        board[i] = context->pieces[index_of_piece_to_place];
        hash -= prev_offset;
    }
}

static void UnhashStep1ConvertUnordered(GenericHashContext *context,
                                        int *config, char *board) {
    for (int i = 0; i < context->num_unordered_pieces; ++i) {
        board[context->board_size + i] = (char)config[context->num_pieces + i];
    }
}

bool GenericHashContextUnhash(GenericHashContext *context, Position hash,
                              char *board) {
    if (hash < 0 || hash > context->num_positions) return false;
    if (context->player == 0) hash >>= 1;  // Get rid of the turn bit.

    // Find the index of the largest offset that is smaller than or equal to the
    // given hash.
    int64_t index_in_valid_configs = FindLargestSmallerEqual(
        context->config_hash_offsets, context->num_valid_configs, hash);
    assert(index_in_valid_configs >= 0);
    assert(index_in_valid_configs < context->num_valid_configs);

    int64_t config_index =
        context->valid_config_indices[index_in_valid_configs];
    int config[STACK_CONFIG_SIZE] = {0};
    IndexToConfig(context, config_index, config);

    // hash(board) = hash_without_turn - offset_for_config
    hash -= context->config_hash_offsets[index_in_valid_configs];
    UnhashStep0HashUncruncher(context, hash, config, board);
    UnhashStep1ConvertUnordered(context, config, board);

    return true;
}

// ========================= GenericHashContextGetTurn =========================

int GenericHashContextGetTurn(GenericHashContext *context, Position hash) {
    if (context->player != 0) return context->player;
    return (hash & 1) + 1;
}

#undef STACK_CONFIG_SIZE
