#ifndef GAMESMANONE_GAMES_DSHOGI_DSHOGI_CONSTANTS_H_
#define GAMESMANONE_GAMES_DSHOGI_DSHOGI_CONSTANTS_H_

#include <stdbool.h>  // bool
#include <stdint.h>   // int8_t

// G -> 0, g -> 0, E -> 1, e -> 1, C -> 2, c -> 2, H -> 2, h -> 2, all others
// are mapped to 3.
extern int8_t kPieceTypeToIndex[128];

// 0 -> G, 1 -> E, 2 -> C.
extern const char kIndexToPieceType[3];

// L -> 0, l -> 1, G -> 2, g -> 3, E -> 4, e -> 5, H -> 6, h -> 7, C -> 8,
// c -> 9, all others are mapped to 0 and should not be accessed.
extern int8_t kPieceToIndex[128];

// kMoveMatrix[i][j][k] stores the k-th possible destination of piece of index i
// at the j-th slot of the board. The three dimensions correspond to piece
// index, board slot index, and available move index, respectively.
extern int8_t kMoveMatrix[10][12][8];

// kMoveMatrixNumMoves[i][j] is the size of kMoveMatrix[i][j].
// The two dimensions correspond to piece index and board slot index.
extern int8_t kMoveMatrixNumMoves[10][12];

// kPromoteMatrix[i][j] is true if and only if the piece of index i can be
// promoted once it moves to the j-th slot on the board. The two dimensions
// correspond to piece index and board slot index.
extern bool kPromoteMatrix[10][12];

extern const int kNumSymmetries;
extern const int8_t kSymmetryMatrix[2][12];

void DobutsuShogiInitGlobalVariables(void);

#endif  // GAMESMANONE_GAMES_DSHOGI_DSHOGI_CONSTANTS_H_
