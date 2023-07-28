#ifndef GAMESMANEXPERIMENT_CORE_GENERIC_HASH_CONTEXT_H_
#define GAMESMANEXPERIMENT_CORE_GENERIC_HASH_CONTEXT_H_

#include <limits.h>

#include "core/data_structures/int64_hash_map.h"
#include "core/gamesman_types.h"

// Definitions --
//     Piece configuration: an array of integers representing the number
//     of each type of piece on board.
//
//     Valid piece configuration: a piece configuration is valid for a hash
//     context if and only if the total number of pieces equals to board_size
//     and context.IsValidConfiguration(configuration) returns true.

typedef struct GenericHashContext {
    int board_size;  // E.g. board_size == 9 for tic-tac-toe.
    int player;      // 0=Both Player boards (default), 1=1st Player only, 2=2nd
                     // only
    int num_pieces;  // Number of types of pieces.
    char *pieces;    // Array of all possible pieces, length == num_pieces.
    // Supports up to 128 different pieces. A piece can be of any value between
    // 0 to 127, which is then mapped to its index.
    char piece_index_mapping[128];
    int *mins;  // Min number of each type of piece, aligned with the pieces
                // array.
    int *maxs;  // Max number of each type of piece, aligned with the pieces
                // array.

    // Piece configuration validation function.
    bool (*IsValidConfiguration)(const int *configuration);

    // Equals max_hash_value + 1, similar to global_num_positions.
    Position num_positions;
    int64_t num_valid_configurations;
    int64_t *valid_configuration_indices;
    Position *configuration_hash_offsets;
} GenericHashContext;

bool GenericHashContextInit(
    GenericHashContext *context, int board_size, int player,
    const int *pieces_init_array,
    bool (*IsValidConfiguration)(const int *configuration));
void GenericHashContextDestroy(GenericHashContext *context);

Position GenericHashContextNumPositions(const GenericHashContext *context);
Position GenericHashContextHash(GenericHashContext *context, const char *board,
                                int turn);
bool GenericHashContextUnhash(GenericHashContext *context, Position hash,
                              char *board);
int GenericHashContextGetTurn(GenericHashContext *context, Position hash);

#endif  // GAMESMANEXPERIMENT_CORE_GENERIC_HASH_CONTEXT_H_