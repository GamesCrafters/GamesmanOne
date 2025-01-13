#ifndef GAMESMANONE_CORE_HASH_TWO_PIECE_H_
#define GAMESMANONE_CORE_HASH_TWO_PIECE_H_

#include <stdbool.h>  // bool
#include <stdint.h>   // uint64_t

#include "core/types/gamesman_types.h"

intptr_t TwoPieceHashGetMemoryRequired(int rows, int cols, int num_symmetries);
int TwoPieceHashInit(int rows, int cols, const int *const *symmetry_matrix,
                     int num_symmetries);
void TwoPieceHashFinalize(void);

int64_t TwoPieceHashGetNumPositions(int num_x, int num_o);
Position TwoPieceHashHash(uint64_t board, int turn);
uint64_t TwoPieceHashUnhash(Position hash, int num_x, int num_o);
int TwoPieceHashGetTurn(Position hash);

uint64_t TwoPieceHashGetCanonicalBoard(uint64_t board);

#endif  // GAMESMANONE_CORE_HASH_TWO_PIECE_H_
