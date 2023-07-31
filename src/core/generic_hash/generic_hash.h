#ifndef GAMESMANEXPERIMENT_CORE_GENERIC_HASH_GENERIC_HASH_H_
#define GAMESMANEXPERIMENT_CORE_GENERIC_HASH_GENERIC_HASH_H_

#include <stdint.h>

#include "core/gamesman_types.h"

bool GenericHashAddContext(
    int player, int board_size, const int *pieces_init_array,
    bool (*IsValidConfiguration)(const int *configuration), int64_t label);
void GenericHashReinitialize(void);

// Convenience functions for working with only one context.

Position GenericHashNumPositions(void);
Position GenericHashHash(const char *board, int turn);
bool GenericHashUnhash(Position hash, char *board);

// Multi-context hashing and unhashing functions.

Position GenericHashNumPositionsLabel(int64_t context_label);
Position GenericHashHashLabel(int64_t context_label, const char *board,
                              int turn);
bool GenericHashUnhashLabel(int64_t context_label, Position hash, char *board);

#endif  // GAMESMANEXPERIMENT_CORE_GENERIC_HASH_GENERIC_HASH_H_
