#ifndef GAMESMANONE_CORE_GENERIC_HASH_TWO_PIECE_H_
#define GAMESMANONE_CORE_GENERIC_HASH_TWO_PIECE_H_

#include <stdint.h>  // uint64_t

#include "core/types/gamesman_types.h"

intptr_t TwoPieceHashGetMemoryRequired(int board_size);
int TwoPieceHashInit(int board_size);
void TwoPieceHashFinalize(void);

Position TwoPieceHashHash(uint64_t board, int turn);
uint64_t TwoPieceHashUnhash(Position hash, int num_x, int num_o);
int TwoPieceHashGetTurn(Position hash);

#endif  // GAMESMANONE_CORE_GENERIC_HASH_TWO_PIECE_H_
