#ifndef GAMESMANONE_GAMES_MILLS_MASKS_H_
#define GAMESMANONE_GAMES_MILLS_MASKS_H_

#include <stdint.h>

// #include "games/mills/variants.h"
#define NUM_BOARD_AND_PIECES_CHOICES 8

/**
 * @brief Number of slots on each board variant. kNumSlots[i] gives the number
 * of slots on board with index i.
 */
static const int kNumSlots[NUM_BOARD_AND_PIECES_CHOICES] = {
    16, 16, 17, 24, 24, 24, 24, 25,
};

/**
 * @brief Masks with set bits corresponding to slots on each board variant.
 * kBoardMasks[i] gives the mask for board with index i.
 */
static const uint64_t kBoardMasks[NUM_BOARD_AND_PIECES_CHOICES] = {
    0x150e1b0e15,     0x150e1b0e15,     0x150e1f0e15,     0x492a1c771c2a49,
    0x492a1c771c2a49, 0x492a1c771c2a49, 0x492a1c771c2a49, 0x492a1c7f1c2a49,
};

/**
 * @brief Masks with set bits corresponding to slots on the inner ring of each
 * board variant if it has a ring symmetry; otherwise the mask contains all
 * zeros for that board variant. kInnerRingMasks[i] gives the mask for board
 * with index i.
 */
static const uint64_t kInnerRingMasks[NUM_BOARD_AND_PIECES_CHOICES] = {
    0xe0a0e00,    0xe0a0e00,    0x0,          0x1c141c0000,
    0x1c141c0000, 0x1c141c0000, 0x1c141c0000, 0x0,
};

/**
 * @brief Masks with set bits corresponding to slots on the outer ring of each
 * board variant if it has a ring symmetry; otherwise the mask contains all
 * zeros for that board variant. kInnerRingMasks[i] gives the mask for board
 * with index i.
 */
static const uint64_t kOuterRingMasks[NUM_BOARD_AND_PIECES_CHOICES] = {
    0x1500110015,     0x1500110015,     0x0, 0x49000041000049, 0x49000041000049,
    0x49000041000049, 0x49000041000049, 0x0,
};

/**
 * @brief Masks with set bits corresponding to slots that are adjacent to the
 * current slot on the 16 board. kDestMasks16[i] gives the mask for the slot
 * that maps to the i-th least significant bit on the 8x8 bit board.
 */
static const uint64_t kDestMasks16[] = {
    0x10004,     0x0,       0x411,        0x0,       0x100004,     0x0,
    0x0,         0x0,       0x0,          0x20400,   0xa04,        0x80400,
    0x0,         0x0,       0x0,          0x0,       0x100020001,  0x2010200,
    0x0,         0x8100800, 0x1000080010, 0x0,       0x0,          0x0,
    0x0,         0x4020000, 0x40a000000,  0x4080000, 0x0,          0x0,
    0x0,         0x0,       0x400010000,  0x0,       0x1104000000, 0x0,
    0x400100000,
};

/**
 * @brief Masks with set bits corresponding to slots that are adjacent to the
 * current slot on the 17 board. kDestMasks17[i] gives the mask for the slot
 * that maps to the i-th least significant bit on the 8x8 bit board.
 */
static const uint64_t kDestMasks17[] = {
    0x10004,     0x0,       0x411,        0x0,       0x100004,     0x0,
    0x0,         0x0,       0x0,          0x20400,   0x40a04,      0x80400,
    0x0,         0x0,       0x0,          0x0,       0x100020001,  0x2050200,
    0x40a0400,   0x8140800, 0x1000080010, 0x0,       0x0,          0x0,
    0x0,         0x4020000, 0x40a040000,  0x4080000, 0x0,          0x0,
    0x0,         0x0,       0x400010000,  0x0,       0x1104000000, 0x0,
    0x400100000,
};

/**
 * @brief Masks with set bits corresponding to slots that are adjacent to the
 * current slot on the 24 board. kDestMasks24[i] gives the mask for the slot
 * that maps to the i-th least significant bit on the 8x8 bit board.
 */
static const uint64_t kDestMasks24[] = {
    0x1000008,
    0x0,
    0x0,
    0x841,
    0x0,
    0x0,
    0x40000008,
    0x0,
    0x0,
    0x2000800,
    0x0,
    0x82208,
    0x0,
    0x20000800,
    0x0,
    0x0,
    0x0,
    0x0,
    0x4080000,
    0x140800,
    0x10080000,
    0x0,
    0x0,
    0x0,
    0x1000002000001,
    0x20005000200,
    0x402040000,
    0x0,
    0x1020100000,
    0x200050002000,
    0x40000020000040,
    0x0,
    0x0,
    0x0,
    0x804000000,
    0x81400000000,
    0x810000000,
    0x0,
    0x0,
    0x0,
    0x0,
    0x80002000000,
    0x0,
    0x8220800000000,
    0x0,
    0x80020000000,
    0x0,
    0x0,
    0x8000001000000,
    0x0,
    0x0,
    0x41080000000000,
    0x0,
    0x0,
    0x8000040000000,
};

/**
 * @brief Masks with set bits corresponding to slots that are adjacent to the
 * current slot on the 24 board with diagonals. kDestMasks24Plus[i] gives the
 * mask for the slot that maps to the i-th least significant bit on the 8x8 bit
 * board.
 */
static const uint64_t kDestMasks24Plus[] = {
    0x1000208,
    0x0,
    0x0,
    0x841,
    0x0,
    0x0,
    0x40002008,
    0x0,
    0x0,
    0x2040801,
    0x0,
    0x82208,
    0x0,
    0x20100840,
    0x0,
    0x0,
    0x0,
    0x0,
    0x4080200,
    0x140800,
    0x10082000,
    0x0,
    0x0,
    0x0,
    0x1000002000001,
    0x20005000200,
    0x402040000,
    0x0,
    0x1020100000,
    0x200050002000,
    0x40000020000040,
    0x0,
    0x0,
    0x0,
    0x20804000000,
    0x81400000000,
    0x200810000000,
    0x0,
    0x0,
    0x0,
    0x0,
    0x1080402000000,
    0x0,
    0x8220800000000,
    0x0,
    0x40081020000000,
    0x0,
    0x0,
    0x8020001000000,
    0x0,
    0x0,
    0x41080000000000,
    0x0,
    0x0,
    0x8200040000000,
};

/**
 * @brief Masks with set bits corresponding to slots that are adjacent to the
 * current slot on the 25 board. kDestMasks25[i] gives the mask for the slot
 * that maps to the i-th least significant bit on the 8x8 bit board.
 */
static const uint64_t kDestMasks25[] = {
    0x1000208,
    0x0,
    0x0,
    0x841,
    0x0,
    0x0,
    0x40002008,
    0x0,
    0x0,
    0x2000801,
    0x0,
    0x82208,
    0x0,
    0x20000840,
    0x0,
    0x0,
    0x0,
    0x0,
    0x4080000,
    0x8140800,
    0x10080000,
    0x0,
    0x0,
    0x0,
    0x1000002000001,
    0x20005000200,
    0x40a040000,
    0x814080000,
    0x1028100000,
    0x200050002000,
    0x40000020000040,
    0x0,
    0x0,
    0x0,
    0x804000000,
    0x81408000000,
    0x810000000,
    0x0,
    0x0,
    0x0,
    0x0,
    0x1080002000000,
    0x0,
    0x8220800000000,
    0x0,
    0x40080020000000,
    0x0,
    0x0,
    0x8020001000000,
    0x0,
    0x0,
    0x41080000000000,
    0x0,
    0x0,
    0x8200040000000,
};

/**
 * @brief Masks with set bits corresponding to slots that are adjacent to the
 * each slot on each board variant. kDestMasks[board_id][i] gives the mask for
 * the slot that maps to the i-th least significant bit on the board with index
 * equal to board_id.
 */
static const uint64_t *kDestMasks[NUM_BOARD_AND_PIECES_CHOICES] = {
    kDestMasks16, kDestMasks16,     kDestMasks17,     kDestMasks24,
    kDestMasks24, kDestMasks24Plus, kDestMasks24Plus, kDestMasks25,
};

static const int kNumLines[NUM_BOARD_AND_PIECES_CHOICES] = {
    8, 8, 14, 16, 16, 20, 20, 22,
};

static const uint64_t kLineMasks16[] = {
    0x15,  0x1000100010, 0x1500000000, 0x100010001,
    0xe00, 0x8080800,    0xe000000,    0x2020200,
};

static const uint64_t kLineMasks17[] = {
    0x15,        0x1000100010, 0x1500000000, 0x100010001, 0xe00,
    0x8080800,   0xe000000,    0x2020200,    0x40404,     0x4040400,
    0x404040000, 0x70000,      0xe0000,      0x1c0000,
};

static const uint64_t kLineMasks24[] = {
    0x49,     0x40000040000040, 0x49000000000000, 0x1000001000001,
    0x2a00,   0x200020002000,   0x2a0000000000,   0x20002000200,
    0x1c0000, 0x1010100000,     0x1c00000000,     0x404040000,
    0x80808,  0x70000000,       0x8080800000000,  0x7000000,
};

static const uint64_t kLineMasks24Plus[] = {
    0x49,     0x40000040000040, 0x49000000000000, 0x1000001000001,
    0x2a00,   0x200020002000,   0x2a0000000000,   0x20002000200,
    0x1c0000, 0x1010100000,     0x1c00000000,     0x404040000,
    0x80808,  0x70000000,       0x8080800000000,  0x7000000,
    0x40201,  0x102040,         0x40201000000000, 0x1020400000000,
};

static const uint64_t kLineMasks25[] = {
    0x49,
    0x40000040000040,
    0x49000000000000,
    0x1000001000001,
    0x2a00,
    0x200020002000,
    0x2a0000000000,
    0x20002000200,
    0x1c0000,
    0x1010100000,
    0x1c00000000,
    0x404040000,
    0x80808,
    0x8080800,
    0x808080000,
    0x80808000000,
    0x8080800000000,
    0x7000000,
    0xe000000,
    0x1c000000,
    0x38000000,
    0x70000000,
};

static const uint64_t *kLineMasks[NUM_BOARD_AND_PIECES_CHOICES] = {
    kLineMasks16, kLineMasks16,     kLineMasks17,     kLineMasks24,
    kLineMasks24, kLineMasks24Plus, kLineMasks24Plus, kLineMasks25,
};

/**
 * @brief Maps row-major board slot indices to 8x8 bit board indices.
 * kBoardIdxToGridIdx[board_id][i] gives the 8x8 bit board index corresponding
 * to the i-th row-major board slot on the board with index board_id.
 */
static const int kBoardIdxToGridIdx[NUM_BOARD_AND_PIECES_CHOICES][25] = {
    {0, 2, 4, 9, 10, 11, 16, 17, 19, 20, 25, 26, 27, 32, 34, 36},
    {0, 2, 4, 9, 10, 11, 16, 17, 19, 20, 25, 26, 27, 32, 34, 36},
    {0, 2, 4, 9, 10, 11, 16, 17, 18, 19, 20, 25, 26, 27, 32, 34, 36},
    {0,  3,  6,  9,  11, 13, 18, 19, 20, 24, 25, 26,
     28, 29, 30, 34, 35, 36, 41, 43, 45, 48, 51, 54},
    {0,  3,  6,  9,  11, 13, 18, 19, 20, 24, 25, 26,
     28, 29, 30, 34, 35, 36, 41, 43, 45, 48, 51, 54},
    {0,  3,  6,  9,  11, 13, 18, 19, 20, 24, 25, 26,
     28, 29, 30, 34, 35, 36, 41, 43, 45, 48, 51, 54},
    {0,  3,  6,  9,  11, 13, 18, 19, 20, 24, 25, 26,
     28, 29, 30, 34, 35, 36, 41, 43, 45, 48, 51, 54},
    {0,  3,  6,  9,  11, 13, 18, 19, 20, 24, 25, 26, 27,
     28, 29, 30, 34, 35, 36, 41, 43, 45, 48, 51, 54},
};

#endif  // GAMESMANONE_GAMES_MILLS_MASKS_H_
