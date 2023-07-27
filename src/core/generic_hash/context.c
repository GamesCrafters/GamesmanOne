#include "core/generic_hash/context.h"

#include <assert.h>
#include <malloc.h>  // malloc, free
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "core/gamesman_math.h"

static int64_t FindLargestSmallerEqual(int64_t *array, int64_t size,
                                       int64_t target);
static int64_t Rearrange(GenericHashContext *context);

static int PieceToIndex(GenericHashContext *context, char piece);
static int ContextGetNumPossiblePieceNumbers(const GenericHashContext *context,
                                             int piece_index);
static int64_t ConfigurationToIndex(GenericHashContext *context);
static void IndexToConfiguration(GenericHashContext *context, int64_t index);

// Helper functions for GenericHashContextInit().

static void InitStep0MemsetToDefaultValues(GenericHashContext *context);
static bool InitStep1SetupPiecesAndIndexMapping(GenericHashContext *context,
                                                const int *pieces_init_array);
static bool InitStep2SetValidConfigurations(GenericHashContext *context);
static int64_t InitStep2_0CountNumConfigurations(GenericHashContext *context);
static void InitStep2_1CountNumValidConfigurations(GenericHashContext *context,
                                                   int64_t num_configurations);
static bool InitStep2_2AllocateSpaces(GenericHashContext *context);
static bool InitStep2_3CalculateSizes(GenericHashContext *context,
                                      int64_t num_configurations);

// Helper functions for GenericHashContextHash().

static bool HashStep0Initialize(GenericHashContext *context, const char *board);
static int64_t HashStep1FindHashOffsetForConfiguration(
    GenericHashContext *context);
static Position HashStep2HashCruncher(GenericHashContext *context,
                                      const char *board);

// Helper functions for GenericHashContextUnhash().

static bool UnhashStep0HashUncruncher(GenericHashContext *context,
                                      Position hash, char *board);

//------------------------------------------------------------------------------
bool GenericHashContextInit(
    GenericHashContext *context, int board_size, int player,
    const int *pieces_init_array,
    bool (*IsValidConfiguration)(const int *configuration)) {
    // Throughout the whole initialization process, all pointers should remain
    // NULL if they are unset.
    InitStep0MemsetToDefaultValues(context);

    context->board_size = board_size;
    context->player = player;

    // This sets up num_pieces, pieces, piece_index_mapping, mins, maxs, and
    // this_configuration.
    if (!InitStep1SetupPiecesAndIndexMapping(context, pieces_init_array)) {
        GenericHashContextDestroy(context);
        return false;
    }
    context->IsValidConfiguration = IsValidConfiguration;

    // This sets up num_positions, num_valid_configurations,
    // valid_configuration_indices, and configuration_hash_offsets.
    if (!InitStep2SetValidConfigurations(context)) {
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
    free(context->valid_configuration_indices);
    free(context->configuration_hash_offsets);
    free(context->this_configuration);
    InitStep0MemsetToDefaultValues(context);
}

Position GenericHashContextHash(GenericHashContext *context, const char *board,
                                int turn) {
    if (!HashStep0Initialize(context, board)) return -1;
    int64_t index = HashStep1FindHashOffsetForConfiguration(context);
    if (index < 0) return -1;

    // hash_without_turn = offset_for_configuration + hash(board)
    Position hash = context->configuration_hash_offsets[index];
    hash += HashStep2HashCruncher(context, board);

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
    int64_t index_in_valid_configurations =
        FindLargestSmallerEqual(context->configuration_hash_offsets,
                                context->num_valid_configurations, hash);
    assert(index_in_valid_configurations >= 0);
    assert(index_in_valid_configurations < context->num_valid_configurations);
    int64_t configuration_index =
        context->valid_configuration_indices[index_in_valid_configurations];
    IndexToConfiguration(context, configuration_index);

    // hash(board) = hash_without_turn - offset_for_configuration
    hash -= context->configuration_hash_offsets[index_in_valid_configurations];
    UnhashStep0HashUncruncher(context, hash, board);
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
    return context->piece_index_mapping[(unsigned char)piece];
}

static int ContextGetNumPossiblePieceNumbers(const GenericHashContext *context,
                                             int piece_index) {
    return context->maxs[piece_index] - context->mins[piece_index] + 1;
}

static int64_t ConfigurationToIndex(GenericHashContext *context) {
    int64_t index = 0;
    for (int i = context->num_pieces - 1; i >= 0; --i) {
        index *= ContextGetNumPossiblePieceNumbers(context, i);
        index += context->this_configuration[i] - context->mins[i];
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
    context->this_configuration =
        (int *)malloc(context->num_pieces * sizeof(int));
    if (context->pieces == NULL || context->mins == NULL ||
        context->maxs == NULL || context->this_configuration == NULL) {
        return false;
    }
    // Setup values.
    for (int i = 0; pieces_init_array[i * 3] >= 0; ++i) {
        char piece = context->pieces[i] = (char)pieces_init_array[i * 3];
        context->mins[i] = pieces_init_array[i * 3 + 1];
        context->maxs[i] = pieces_init_array[i * 3 + 2];
        if (context->piece_index_mapping[piece] != -1) {
            fprintf(stderr,
                    "GenericHashContextInit: piece char '%c'(%x) appeared "
                    "twice in pieces_init_array. Aborting...\n",
                    piece, piece);
            return false;
        }
        context->piece_index_mapping[piece] = i;
    }
    return true;
}

static bool InitStep2SetValidConfigurations(GenericHashContext *context) {
    int64_t num_configurations = InitStep2_0CountNumConfigurations(context);
    if (num_configurations < 0) {
        fprintf(stderr,
                "GenericHashContextInit: too many possible piece "
                "configurations to be represented by the current integer type. "
                "Aborting...\n");
        return false;
    }
    InitStep2_1CountNumValidConfigurations(context, num_configurations);
    if (!InitStep2_2AllocateSpaces(context)) return false;
    return InitStep2_3CalculateSizes(context, num_configurations);
}

static int64_t InitStep2_0CountNumConfigurations(GenericHashContext *context) {
    int64_t num_configurations = 1;
    for (int i = 0; i < context->num_pieces; ++i) {
        int64_t num_possible_piece_numbers =
            ContextGetNumPossiblePieceNumbers(context, i);
        num_configurations = SafeMultiplyNonNegativeInt64(
            num_configurations, num_possible_piece_numbers);
    }
    return num_configurations;
}

static void InitStep2_1CountNumValidConfigurations(GenericHashContext *context,
                                                   int64_t num_configurations) {
    assert(context->this_configuration != NULL);
    assert(context->IsValidConfiguration != NULL);
    for (int64_t i = 0; i < num_configurations; ++i) {
        IndexToConfiguration(context, i);
        // Increment if valid, don't if not.
        context->num_valid_configurations +=
            context->IsValidConfiguration(context->this_configuration);
    }
}

static bool InitStep2_2AllocateSpaces(GenericHashContext *context) {
    context->valid_configuration_indices =
        (int64_t *)malloc(context->num_valid_configurations * sizeof(int64_t));
    context->configuration_hash_offsets = (Position *)malloc(
        context->num_valid_configurations * sizeof(Position));
    return (context->valid_configuration_indices != NULL &&
            context->configuration_hash_offsets != NULL);
}

// Calculates the size of each valid configuration and add them up to get
// num_positions.
static bool InitStep2_3CalculateSizes(GenericHashContext *context,
                                      int64_t num_configurations) {
    int64_t j = 0;
    for (int64_t i = 0; i < num_configurations; ++i) {
        IndexToConfiguration(context, i);
        if (context->IsValidConfiguration(context->this_configuration)) {
            context->valid_configuration_indices[j] = i;
            context->configuration_hash_offsets[j] = context->num_positions;
            context->num_positions = SafeAddNonNegativeInt64(
                context->num_positions, Rearrange(context));
            ++j;
        }
    }
    assert(j == context->valid_configuration_indices);
    if (context->num_positions < 0) {
        fprintf(stderr,
                "GenericHashContextInit: too many positions to be represented "
                "by the current Position type. Aborting...\n");
        return false;
    }
    return true;
}

static void IndexToConfiguration(GenericHashContext *context, int64_t index) {
    for (int i = 0; i < context->num_pieces; ++i) {
        int num_possible_piece_numbers =
            ContextGetNumPossiblePieceNumbers(context, i);
        context->this_configuration[i] =
            context->mins[i] + (index % num_possible_piece_numbers);
        index /= num_possible_piece_numbers;
    }
}

static bool HashStep0Initialize(GenericHashContext *context,
                                const char *board) {
    // Count the number of pieces of each type.
    memset(context->this_configuration, 0, sizeof(int) * context->num_pieces);
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
        ++context->this_configuration[piece_index];
    }
    return true;
}

static int64_t HashStep1FindHashOffsetForConfiguration(
    GenericHashContext *context) {
    // Assumes context->this_configuration has already been set.
    // Validate the given configuration and find its hash offset.
    int64_t configuration_index = ConfigurationToIndex(context);
    int64_t index_in_valid_configurations = FindLargestSmallerEqual(
        context->valid_configuration_indices, context->num_valid_configurations,
        configuration_index);
    if (configuration_index !=
        context->valid_configuration_indices[index_in_valid_configurations]) {
        fprintf(stderr,
                "GenericHashContextHash: invalid piece configuration\n");
        return -1;
    }
    return index_in_valid_configurations;
}

static int64_t Rearrange(GenericHashContext *context) {
    int64_t pieces_rearranged = 0, result = 1;
    for (int piece_index = 0; piece_index < context->num_pieces - 1;
         ++piece_index) {
        pieces_rearranged += context->this_configuration[piece_index];
        int64_t more_pieces = context->this_configuration[piece_index + 1];
        int64_t combinations =
            NChooseR(pieces_rearranged + more_pieces, pieces_rearranged);
        result = SafeMultiplyNonNegativeInt64(result, combinations);
    }
    return result;
}

static Position HashStep2HashCruncher(GenericHashContext *context,
                                      const char *board) {
    Position final_hash = 0;

    // The loop ends with i == 0 because there will be
    // only one way to place the last piece.
    for (int i = context->board_size - 1; i > 0; --i) {
        // Find the index corresponding to the type of piece at board[i].
        int piece_index = PieceToIndex(context, board[i]);

        // For each piece that has a rank smaller than the current piece...
        for (int j = 0; j < piece_index; ++j) {
            // If we still have any pieces of this smaller rank...
            if (context->this_configuration[j] > 0) {
                // Take this piece out and rearrange the rest of the pieces on
                // the remaining slots on the board. Add the number of
                // rearrangements to our final hash.
                --context->this_configuration[j];
                int64_t num_rearrangements = Rearrange(context);
                assert(num_rearrangements >= 0);
                final_hash += num_rearrangements;
                ++context->this_configuration[j];
            }
        }
        // Finished analyzing the current piece. "Recursively" hash the rest of
        // the pieces on board.
        --context->this_configuration[piece_index];
    }
    return final_hash;
}

static bool UnhashStep0HashUncruncher(GenericHashContext *context,
                                      Position hash, char *board) {
    for (int i = context->board_size - 1; i >= 0; --i) {
        int64_t prev_offset = 0, curr_offset = 0;
        int index_of_piece_to_place = 0;
        for (int j = 0; (curr_offset <= hash) && (j < context->num_pieces);
             ++j) {
            assert(context->this_configuration[j] >= 0);
            if (context->this_configuration[j] == 0) continue;
            prev_offset = curr_offset;
            --context->this_configuration[j];
            curr_offset += Rearrange(context);
            ++context->this_configuration[j];
            index_of_piece_to_place = j;
        }
        --context->this_configuration[index_of_piece_to_place];
        board[i] = context->pieces[index_of_piece_to_place];
        hash -= prev_offset;
    }
}
