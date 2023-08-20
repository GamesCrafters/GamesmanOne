#include "core/generic_hash/context.h"

#include <assert.h>   // assert
#include <malloc.h>   // malloc, free
#include <stdbool.h>  // bool
#include <stdio.h>    // fprintf, stderr
#include <string.h>   // memset, memcpy

#include "core/gamesman_math.h"

#define STACK_CONFIG_SIZE 128  // At most 128 pieces.

static int64_t FindLargestSmallerEqual(int64_t *array, int64_t size,
                                       int64_t target);
static int64_t Rearrange(GenericHashContext *context, int *config);

static int PieceToIndex(GenericHashContext *context, char piece);
static int ContextGetNumPossiblePieceNumbers(const GenericHashContext *context,
                                             int piece_index);
static int64_t ConfigToIndex(GenericHashContext *context, int *config);
static void IndexToConfig(GenericHashContext *context, int64_t index,
                          int *config);

// Helper functions for GenericHashContextInit().

static void InitStep0MemsetToDefaultValues(GenericHashContext *context);
static bool InitStep1SetupPiecesAndIndexMapping(GenericHashContext *context,
                                                const int *pieces_init_array);
static bool IsValidConfig(GenericHashContext *context, int *config);
static bool InitStep2SetValidConfigs(GenericHashContext *context);
static int64_t InitStep2_0CountNumConfigs(GenericHashContext *context);
static void InitStep2_1CountNumValidConfigs(GenericHashContext *context,
                                            int64_t num_configs);
static bool InitStep2_2AllocateSpaces(GenericHashContext *context);
static bool InitStep2_3CalculateSizes(GenericHashContext *context,
                                      int64_t num_configs);

// Helper functions for GenericHashContextHash().

static bool HashStep0Initialize(GenericHashContext *context, const char *board,
                                int *config);
static int64_t HashStep1FindHashOffsetForConfig(GenericHashContext *context,
                                                int *config);
static Position HashStep2HashCruncher(GenericHashContext *context,
                                      const char *board, int *config);

// Helper functions for GenericHashContextUnhash().

static void UnhashStep0HashUncruncher(GenericHashContext *context,
                                      Position hash, int *config, char *board);

//------------------------------------------------------------------------------
bool GenericHashContextInit(GenericHashContext *context, int board_size,
                            int player, const int *pieces_init_array,
                            bool (*IsValidConfig)(const int *config)) {
    // Throughout the whole initialization process, all pointers should remain
    // NULL if they are unset.
    InitStep0MemsetToDefaultValues(context);

    context->board_size = board_size;
    context->player = player;

    // This sets up num_pieces, pieces, piece_index_mapping, mins, and maxs
    if (!InitStep1SetupPiecesAndIndexMapping(context, pieces_init_array)) {
        GenericHashContextDestroy(context);
        return false;
    }
    context->IsValidConfig = IsValidConfig;

    // This sets up num_positions, num_valid_configs,
    // valid_config_indices, and config_hash_offsets.
    if (!InitStep2SetValidConfigs(context)) {
        GenericHashContextDestroy(context);
        return false;
    }
    return true;
}

void GenericHashContextDestroy(GenericHashContext *context) {
    // Assumes all non-NULL pointers were malloc'ed.
    free(context->pieces);
    free(context->mins);
    free(context->maxs);
    free(context->valid_config_indices);
    free(context->config_hash_offsets);
    InitStep0MemsetToDefaultValues(context);
}

Position GenericHashContextNumPositions(const GenericHashContext *context) {
    return context->num_positions;
}

Position GenericHashContextHash(GenericHashContext *context, const char *board,
                                int turn) {
    int this_config[STACK_CONFIG_SIZE];
    if (!HashStep0Initialize(context, board, this_config)) return -1;
    int64_t index = HashStep1FindHashOffsetForConfig(context, this_config);
    if (index < 0) return -1;

    // hash_without_turn = offset_for_config + hash(board)
    Position hash = context->config_hash_offsets[index];
    hash += HashStep2HashCruncher(context, board, this_config);

    // The final hash contains no turn bit if there is only one player.
    if (context->player != 0) return hash;

    // Otherwise, validate the given turn value and append the turn bit.
    if (turn != 1 && turn != 2) return -1;
    return (hash << 1) | (turn == 2);
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
    int this_config[STACK_CONFIG_SIZE];
    IndexToConfig(context, config_index, this_config);

    // hash(board) = hash_without_turn - offset_for_config
    hash -= context->config_hash_offsets[index_in_valid_configs];
    UnhashStep0HashUncruncher(context, hash, this_config, board);
    return true;
}

int GenericHashContextGetTurn(GenericHashContext *context, Position hash) {
    if (context->player != 0) return context->player;
    return (hash & 1) + 1;
}
//------------------------------------------------------------------------------

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

static int PieceToIndex(GenericHashContext *context, char piece) {
    if (piece < 0) return -1;
    return context->piece_index_mapping[(int)piece];
}

static int ContextGetNumPossiblePieceNumbers(const GenericHashContext *context,
                                             int piece_index) {
    return context->maxs[piece_index] - context->mins[piece_index] + 1;
}

static int64_t ConfigToIndex(GenericHashContext *context, int *config) {
    int64_t index = 0;
    for (int i = context->num_pieces - 1; i >= 0; --i) {
        index *= ContextGetNumPossiblePieceNumbers(context, i);
        index += config[i] - context->mins[i];
    }
    return index;
}

static void InitStep0MemsetToDefaultValues(GenericHashContext *context) {
    memset(context, 0, sizeof(*context));
    memset(context->piece_index_mapping, -1,
           sizeof(context->piece_index_mapping));
}

static bool InitStep1SetupPiecesAndIndexMapping(GenericHashContext *context,
                                                const int *pieces_init_array) {
    // Count the number of pieces.
    for (int i = 0; pieces_init_array[i * 3] >= 0; ++i) {
        ++context->num_pieces;
    }
    // Allocate space.
    context->pieces = (char *)malloc(context->num_pieces * sizeof(char));
    context->mins = (int *)malloc(context->num_pieces * sizeof(int));
    context->maxs = (int *)malloc(context->num_pieces * sizeof(int));
    if (context->pieces == NULL || context->mins == NULL ||
        context->maxs == NULL) {
        return false;
    }
    // Setup values.
    for (int i = 0; pieces_init_array[i * 3] >= 0; ++i) {
        char piece = context->pieces[i] = (char)pieces_init_array[i * 3];
        if (piece < 0) {
            fprintf(stderr,
                    "GenericHashContextInit: unsupported negative piece char "
                    "[%x] detected in pieces_init_array. Aborting...\n",
                    piece);
            return false;
        }
        context->mins[i] = pieces_init_array[i * 3 + 1];
        context->maxs[i] = pieces_init_array[i * 3 + 2];
        if (context->piece_index_mapping[(int)piece] != -1) {
            fprintf(stderr,
                    "GenericHashContextInit: piece char '%c'(%x) appeared "
                    "twice in pieces_init_array. Aborting...\n",
                    piece, piece);
            return false;
        }
        context->piece_index_mapping[(int)piece] = i;
    }
    return true;
}

static bool IsValidConfig(GenericHashContext *context, int *config) {
    // Check if all pieces add up to context->board_size.
    int pieces = 0;
    for (int i = 0; i < context->num_pieces; ++i) {
        pieces += config[i];
    }

    if (pieces != context->board_size) return false;

    if (context->IsValidConfig != NULL) {
        return context->IsValidConfig(config);
    }
    return true;
}

static bool InitStep2SetValidConfigs(GenericHashContext *context) {
    int64_t num_configs = InitStep2_0CountNumConfigs(context);
    if (num_configs < 0) {
        fprintf(stderr,
                "GenericHashContextInit: too many possible piece "
                "configurations to be represented by the current integer type. "
                "Aborting...\n");
        return false;
    }
    InitStep2_1CountNumValidConfigs(context, num_configs);
    if (!InitStep2_2AllocateSpaces(context)) return false;
    return InitStep2_3CalculateSizes(context, num_configs);
}

static int64_t InitStep2_0CountNumConfigs(GenericHashContext *context) {
    int64_t num_configs = 1;
    for (int i = 0; i < context->num_pieces; ++i) {
        int64_t num_possible_piece_numbers =
            ContextGetNumPossiblePieceNumbers(context, i);
        num_configs = SafeMultiplyNonNegativeInt64(num_configs,
                                                   num_possible_piece_numbers);
    }
    return num_configs;
}

static void InitStep2_1CountNumValidConfigs(GenericHashContext *context,
                                            int64_t num_configs) {
    int this_config[STACK_CONFIG_SIZE];
    for (int64_t i = 0; i < num_configs; ++i) {
        IndexToConfig(context, i, this_config);

        // Increment if and only if this_config is valid.
        context->num_valid_configs += IsValidConfig(context, this_config);
    }
}

static bool InitStep2_2AllocateSpaces(GenericHashContext *context) {
    context->valid_config_indices =
        (int64_t *)malloc(context->num_valid_configs * sizeof(int64_t));
    if (context->valid_config_indices == NULL) return false;

    context->config_hash_offsets =
        (Position *)malloc(context->num_valid_configs * sizeof(Position));
    if (context->config_hash_offsets == NULL) return false;
    
    return true;
}

// Calculates the size of each valid configuration and add them up to get
// num_positions.
static bool InitStep2_3CalculateSizes(GenericHashContext *context,
                                      int64_t num_configs) {
    int64_t j = 0;
    int this_config[STACK_CONFIG_SIZE];
    for (int64_t i = 0; i < num_configs; ++i) {
        IndexToConfig(context, i, this_config);
        if (IsValidConfig(context, this_config)) {
            context->valid_config_indices[j] = i;
            context->config_hash_offsets[j] = context->num_positions;
            context->num_positions = SafeAddNonNegativeInt64(
                context->num_positions, Rearrange(context, this_config));
            ++j;
        }
    }
    assert(j == context->num_valid_configs);

    if (context->player == 0) {
        // Add the turn bit if there are two players.
        context->num_positions =
            SafeMultiplyNonNegativeInt64(context->num_positions, 2);
    }
    if (context->num_positions < 0) {
        fprintf(stderr,
                "GenericHashContextInit: too many positions to be represented "
                "by the current Position type. Aborting...\n");
        return false;
    }
    return true;
}

static void IndexToConfig(GenericHashContext *context, int64_t index,
                          int *config) {
    for (int i = 0; i < context->num_pieces; ++i) {
        int num_possible_piece_numbers =
            ContextGetNumPossiblePieceNumbers(context, i);
        config[i] = context->mins[i] + (index % num_possible_piece_numbers);
        index /= num_possible_piece_numbers;
    }
}

static bool HashStep0Initialize(GenericHashContext *context, const char *board,
                                int *config) {
    // Count the number of pieces of each type.
    memset(config, 0, sizeof(int) * context->num_pieces);
    for (int i = 0; i < context->board_size; ++i) {
        int piece_index = PieceToIndex(context, board[i]);
        if (piece_index < 0) {
            fprintf(stderr,
                    "GenericHashContextHash: invalid piece '%c'(%x) at "
                    "board[%d]\n",
                    board[i], board[i], i);
            return false;
        }
        assert(piece_index < context->num_pieces);
        ++config[piece_index];
    }
    return true;
}

static int64_t HashStep1FindHashOffsetForConfig(GenericHashContext *context,
                                                int *config) {
    // Validate the given config and find its hash offset.
    int64_t config_index = ConfigToIndex(context, config);
    int64_t index_in_valid_configs =
        FindLargestSmallerEqual(context->valid_config_indices,
                                context->num_valid_configs, config_index);
    if (config_index != context->valid_config_indices[index_in_valid_configs]) {
        fprintf(stderr,
                "GenericHashContextHash: invalid piece configuration\n");
        return -1;
    }
    return index_in_valid_configs;
}

static int64_t Rearrange(GenericHashContext *context, int *config) {
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

static Position HashStep2HashCruncher(GenericHashContext *context,
                                      const char *board, int *config) {
    Position final_hash = 0;

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
                int64_t num_rearrangements = Rearrange(context, config);
                assert(num_rearrangements >= 0);
                final_hash += num_rearrangements;
                ++config[j];
            }
        }
        // Finished analyzing the current piece. "Recursively" hash the rest of
        // the pieces on board.
        --config[piece_index];
    }
    return final_hash;
}

static void UnhashStep0HashUncruncher(GenericHashContext *context,
                                      Position hash, int *config, char *board) {
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
            curr_offset += Rearrange(context, config);
            ++config[j];
            index_of_piece_to_place = j;
        }
        --config[index_of_piece_to_place];
        board[i] = context->pieces[index_of_piece_to_place];
        hash -= prev_offset;
    }
}

#undef STACK_CONFIG_SIZE
