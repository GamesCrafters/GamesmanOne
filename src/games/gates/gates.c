#include "games/gates/gates.h"

#include <assert.h>  // assert
#include <ctype.h>   // islower
#include <stddef.h>  // NULL
#include <stdio.h>   // sprintf
#include <stdlib.h>  // atoi
#include <string.h>  // memcpy, strtok_r

#include "core/constants.h"
#include "core/generic_hash/generic_hash.h"
#include "core/misc.h"
#include "core/solvers/tier_solver/tier_solver.h"
#include "core/types/gamesman_types.h"
#include "games/gates/gates_tier.h"

// NOLINTBEGIN(cppcoreguidelines-narrowing-conversions)

enum {
    kBoardAdjacency1SizeMax = 5,
    kBoardAdjacency2SizeMax = 6,
    kBoardAdjacency2BlockingPointsSize = 2,
};

static const char kPieces[kNumPieceTypes] = {'G', 'g', 'A', 'a', 'Z', 'z'};
static int kPieceToTypeIndex[128];

typedef union GatesMove {
    struct {
        int8_t placement_type;
        int8_t placement_dest;
        int8_t gate_src;
        int8_t gate_dest;
        int8_t move_src;
        int8_t move_dest;
        int8_t teleport_dest;
    } unpacked;

    Move hashed;
} GatesMove;

// GatesMove default initializer.
static const GatesMove kGatesMoveInit = {
    .unpacked =
        {
            .placement_type = -1,
            .placement_dest = -1,
            .gate_src = -1,
            .gate_dest = -1,
            .move_src = -1,
            .move_dest = -1,
            .teleport_dest = -1,
        },
};

/**
 * @brief Adjacency matrix for immediately adjacent board spaces.
 * @details For i in range(kBoardSize) and j in range(kBoardAdjacency1Size[i]),
 * kBoardAdjacency1[i][j] equals the index of the j-th space immediately
 * adjacent to the i-th space.
 */
static const int8_t kBoardAdjacency1[kBoardSize][kBoardAdjacency1SizeMax] = {
    {1, 3, 4},           {0, 2, 4, 5},        {1, 5, 6},       {0, 4, 7, 8},
    {0, 1, 3, 5, 8},     {1, 2, 4, 6, 9},     {2, 5, 9, 10},   {3, 8, 11},
    {3, 4, 7, 11, 12},   {5, 6, 10, 13, 14},  {6, 9, 14},      {7, 8, 12, 15},
    {8, 11, 13, 15, 16}, {9, 12, 14, 16, 17}, {9, 10, 13, 17}, {11, 12, 16},
    {12, 13, 15, 17},    {13, 14, 16},
};

/**
 * @brief Number of spaces immediately adjacent to each board space.
 * @details For i in range(kBoardSize), kBoardAdjacency1Size[i] equals the
 * number of spaces immediately adjacent to the i-th space.
 */
static const int8_t kBoardAdjacency1Size[kBoardSize] = {
    3, 4, 3, 4, 5, 5, 4, 3, 5, 5, 3, 4, 5, 5, 4, 3, 4, 3,
};

/**
 * @brief Adjacency matrix for board spaces that are exactly 2 spaces away.
 * @details For i in range(kBoardSize) and j in range(kBoardAdjacency2Size[i]),
 * kBoardAdjacency2[i][j] equals the index of the j-th space that is exactly 2
 * spaces away from the i-th space.
 */
static const int8_t kBoardAdjacency2[kBoardSize][kBoardAdjacency2SizeMax] = {
    {2, 5, 7, 8},          {3, 6, 8, 9},          {0, 4, 9, 10},
    {1, 5, 11, 12},        {2, 6, 7, 9, 11, 12},  {0, 3, 8, 10, 13, 14},
    {1, 4, 13, 14},        {0, 4, 12, 15},        {0, 1, 5, 13, 15, 16},
    {1, 2, 4, 12, 16, 17}, {2, 5, 13, 17},        {3, 4, 13, 16},
    {3, 4, 7, 9, 14, 17},  {5, 6, 8, 10, 11, 15}, {5, 6, 12, 16},
    {7, 8, 13, 17},        {8, 9, 11, 14},        {9, 10, 12, 15},
};

/**
 * @brief Indices of board spaces that block off second level adjacent spaces.
 * @details For i in range(kBoardSize) and j in range(kBoardAdjacency2Size[i]).
 * kBoardAdjacency2BlockingPoints[i][j][0] and
 * kBoardAdjacency2BlockingPoints[i][j][1] are the indices of the spaces that
 * blocks off a path from the i-th space to the kBoardAdjacency2[i][j]-th
 * space. Note that there may be 1 or 2 paths from one space to a second level
 * adjacent space. If there is only one path,
 * kBoardAdjacency2BlockingPoints[i][j][0] and
 * kBoardAdjacency2BlockingPoints[i][j][1] are set equal to each other and they
 * both equal the index to the only blocking point. If there are two paths, the
 * two values are distinct and each one of them blocks off one of the two paths.
 * A triangle spike cannot reach its destination by moving two spaces only if
 * BOTH PATHS are blocked.
 */
static const int8_t
    kBoardAdjacency2BlockingPoints[kBoardSize][kBoardAdjacency2SizeMax][2] = {
        // 0
        {
            {1, 1},  // 0 -> 2
            {1, 4},  // 0 -> 5
            {3, 3},  // 0 -> 7
            {3, 4},  // 0 -> 8
        },
        // 1
        {
            {0, 4},  // 1 -> 3
            {2, 5},  // 1 -> 6
            {4, 4},  // 1 -> 8
            {5, 5},  // 1 -> 9
        },
        // 2
        {
            {1, 1},  // 2 -> 0
            {1, 5},  // 2 -> 4
            {5, 6},  // 2 -> 9
            {6, 6},  // 2 -> 10
        },
        // 3
        {
            {0, 4},  // 3 -> 1
            {4, 4},  // 3 -> 5
            {7, 8},  // 3 -> 11
            {8, 8},  // 3 -> 12
        },
        // 4
        {
            {1, 5},  // 4 -> 2
            {5, 5},  // 4 -> 6
            {3, 8},  // 4 -> 7
            {5, 5},  // 4 -> 9
            {8, 8},  // 4 -> 11
            {8, 8},  // 4 -> 12
        },
        // 5
        {
            {1, 4},  // 5 -> 0
            {4, 4},  // 5 -> 3
            {4, 4},  // 5 -> 8
            {6, 9},  // 5 -> 10
            {9, 9},  // 5 -> 13
            {9, 9},  // 5 -> 14
        },
        // 6
        {
            {2, 5},   // 6 -> 1
            {5, 5},   // 6 -> 4
            {9, 9},   // 6 -> 13
            {9, 10},  // 6 -> 14
        },
        // 7
        {
            {3, 3},    // 7 -> 0
            {3, 8},    // 7 -> 4
            {8, 11},   // 7 -> 12
            {11, 11},  // 7 -> 15
        },
        // 8
        {
            {3, 4},    // 8 -> 0
            {4, 4},    // 8 -> 1
            {4, 4},    // 8 -> 5
            {12, 12},  // 8 -> 13
            {11, 12},  // 8 -> 15
            {12, 12},  // 8 -> 16
        },
        // 9
        {
            {5, 5},    // 9 -> 1
            {5, 6},    // 9 -> 2
            {5, 5},    // 9 -> 4
            {13, 13},  // 9 -> 12
            {13, 13},  // 9 -> 16
            {13, 14},  // 9 -> 17
        },
        // 10
        {
            {6, 6},    // 10 -> 2
            {6, 9},    // 10 -> 5
            {9, 14},   // 10 -> 13
            {14, 14},  // 10 -> 17
        },
        // 11
        {
            {7, 8},    // 11 -> 3
            {8, 8},    // 11 -> 4
            {12, 12},  // 11 -> 13
            {12, 15},  // 11 -> 16
        },
        // 12
        {
            {8, 8},    // 12 -> 3
            {8, 8},    // 12 -> 4
            {8, 11},   // 12 -> 7
            {13, 13},  // 12 -> 9
            {13, 13},  // 12 -> 14
            {13, 16},  // 12 -> 17
        },
        // 13
        {
            {9, 9},    // 13 -> 5
            {9, 9},    // 13 -> 6
            {12, 12},  // 13 -> 8
            {9, 14},   // 13 -> 10
            {12, 12},  // 13 -> 11
            {12, 16},  // 13 -> 15
        },
        // 14
        {
            {9, 9},    // 14 -> 5
            {9, 10},   // 14 -> 6
            {13, 13},  // 14 -> 12
            {13, 17},  // 14 -> 16
        },
        // 15
        {
            {11, 11},  // 15 -> 7
            {11, 12},  // 15 -> 8
            {12, 16},  // 15 -> 13
            {16, 16},  // 15 -> 17
        },
        // 16
        {
            {12, 12},  // 16 -> 8
            {13, 13},  // 16 -> 9
            {12, 15},  // 16 -> 11
            {13, 17},  // 16 -> 14
        },
        // 17
        {
            {13, 14},  // 17 -> 9
            {14, 14},  // 17 -> 10
            {13, 16},  // 17 -> 12
            {16, 16},  // 17 -> 15
        },
};

/**
 * @brief Number of spaces that are exactly 2 spaces away from each board space.
 * @details For i in range(kBoardSize), kBoardAdjacency2Size[i] equals the
 * number of spaces that are exactly 2 spaces away from the i-th space.
 */
static const int8_t kBoardAdjacency2Size[kBoardSize] = {
    4, 4, 4, 4, 6, 6, 4, 4, 6, 6, 4, 4, 6, 6, 4, 4, 4, 4,
};

/**
 * @brief The set of tiers that contain positions from which different moves can
 * lead to the same child and therefore requires deduplication after generating
 * child positions using GatesGenerateMoves and GatesDoMove.
 */
static TierHashSet kChildDedupTiers;

// ========================== Common Helper Functions ==========================

static char TriangleOfPlayer(int turn) {
    // turn == 1 -> 'A'; turn == 2 -> 'a'
    return 'A' + ('a' - 'A') * (turn - 1);
}

static char TrapezoidOfPlayer(int turn) {
    // turn == 1 -> 'Z'; turn == 2 -> 'z'
    return 'Z' + ('z' - 'Z') * (turn - 1);
}

// ============================== kGatesSolverApi ==============================

static Position GatesGetInitialPosition(void) {
    static ConstantReadOnlyString kGatesInitialBoard = "------------------";

    return GenericHashHashLabel(GatesGetInitialTier(), kGatesInitialBoard, 1);
}

static int64_t GatesGetTierSize(Tier tier) {
    return GenericHashNumPositionsLabel(tier);
}

static void InsertWhiteGate(char board[static kBoardSize],
                            GatesTierField index) {
    for (GatesTierField i = kBoardSize - 1; i > index; --i) {
        board[i] = board[i - 1];
    }
    board[index] = 'G';
}

static void UnhashTierAndPosition(TierPosition tier_position, GatesTier *t,
                                  char board[static kBoardSize]) {
    GatesTierUnhash(tier_position.tier, t);
    bool success = GenericHashUnhashLabel(tier_position.tier,
                                          tier_position.position, board);
    assert(success);
    (void)success;

    // Insert the white gates.
    switch (t->n[G]) {
        case 1:
            InsertWhiteGate(board, t->G1);
            break;

        case 2:
            InsertWhiteGate(board, t->G1);
            InsertWhiteGate(board, t->G2);
            break;

        default:
            break;
    }
}

static void GenerateMovesTriangle(const char board[static kBoardSize],
                                  int8_t src, GatesMove move, MoveArray *ret) {
    assert(board[src] == 'A' || board[src] == 'a');
    const char gate = 'G' + board[src] - 'A';
    move.unpacked.move_src = src;

    // For all first level adjacencies, the destination can be reached if it is
    // blank or has a friendly gate on it.
    for (int8_t i = 0; i < kBoardAdjacency1Size[src]; ++i) {
        int8_t dest = kBoardAdjacency1[src][i];
        if (board[dest] == '-' || board[dest] == gate) {
            move.unpacked.move_dest = dest;
            MoveArrayAppend(ret, move.hashed);
        }
    }

    // For all second level adjacencies, the destination can be reached if
    // itself is blank or has a friendly gate on it, and at least one of its
    // blocking points is blank.
    for (int8_t i = 0; i < kBoardAdjacency2Size[src]; ++i) {
        int8_t dest = kBoardAdjacency2[src][i];
        bool unoccupied = board[dest] == '-' || board[dest] == gate;
        bool reachable =
            board[kBoardAdjacency2BlockingPoints[src][i][0]] == '-' ||
            board[kBoardAdjacency2BlockingPoints[src][i][1]] == '-';
        if (unoccupied && reachable) {
            move.unpacked.move_dest = dest;
            MoveArrayAppend(ret, move.hashed);
        }
    }
}

static void GenerateMovesTrapezoid(const char board[static kBoardSize],
                                   int8_t src, GatesMove move, MoveArray *ret) {
    assert(board[src] == 'Z' || board[src] == 'z');
    const char offset =
        (board[src] - 'Z');  // 0 if white's turn, 32 if black's turn.
    const char friendly_gate = 'G' + offset;
    const char opponent_triangle = 'a' - offset;
    const char opponent_trapezoid = 'z' - offset;
    move.unpacked.move_src = src;

    // For all first level adjacencies, the destination can be reached if it is
    // blank, a friendly gate, or an opponent's spike.
    for (int8_t i = 0; i < kBoardAdjacency1Size[src]; ++i) {
        int8_t dest = kBoardAdjacency1[src][i];
        move.unpacked.move_dest = dest;
        if (board[dest] == '-' || board[dest] == friendly_gate) {
            // Move only, no teleporting.
            move.unpacked.teleport_dest = -1;
            MoveArrayAppend(ret, move.hashed);
        } else if (board[dest] == opponent_triangle ||
                   board[dest] == opponent_trapezoid) {
            // Teleporting  to any existing blank spaces:
            for (int8_t j = 0; j < kBoardSize; ++j) {
                if (board[j] == '-') {
                    move.unpacked.teleport_dest = j;
                    MoveArrayAppend(ret, move.hashed);
                }
            }

            // Teleport to where the trapezoid came from:
            move.unpacked.teleport_dest = src;
            MoveArrayAppend(ret, move.hashed);
        }
    }
}

/**
 * Appends all possible moves by moving a piece belong to the player whose turn
 * it is after carrying out the existing operations as indicated by \p move. The
 * existing operations are assumed to be encoded with the following fields of \p
 * move: GatesMove::unpacked::placement_type,
 * GatesMove::unpacked::placement_dest, and GatesMove::unpacked::gate_dest.
 * These fields are not modified by this function.
 */
static void GenerateMovesOfPlayer(const char board[static kBoardSize], int turn,
                                  GatesMove move, MoveArray *ret) {
    char triangle = TriangleOfPlayer(turn);
    char trapezoid = TrapezoidOfPlayer(turn);
    for (int8_t i = 0; i < kBoardSize; ++i) {
        if (board[i] == triangle) {
            GenerateMovesTriangle(board, i, move, ret);
        } else if (board[i] == trapezoid) {
            GenerateMovesTrapezoid(board, i, move, ret);
        }
    }
}

static MoveArray GenerateMovesPlacement(const GatesTier *pt,
                                        char board[static kBoardSize]) {
    assert(pt->phase == kPlacement);
    MoveArray ret;
    MoveArrayInit(&ret);
    GatesTierField num_pieces = GatesTierGetNumPieces(pt);
    GatesMove m = kGatesMoveInit;

    if (num_pieces < 11) {  // Placement only.
        for (int8_t i = 0; i < kBoardSize; ++i) {
            if (board[i] == '-') {  // Find blank slots.
                m.unpacked.placement_dest = i;
                for (int8_t p = 0; p < kNumPieceTypes; ++p) {
                    if (pt->n[p] < 2) {  // Find remaining pieces.
                        m.unpacked.placement_type = p;
                        MoveArrayAppend(&ret, m.hashed);
                    }
                }
            }
        }
    } else {  // Place the last piece and then move a black piece.
        assert(num_pieces == 11);
        int8_t p;
        for (p = 0; p < kNumPieceTypes; ++p) {
            if (pt->n[p] < 2) break;  // Find the only remaining piece.
        }
        assert(p < kNumPieceTypes);
        m.unpacked.placement_type = p;
        for (int8_t i = 0; i < kBoardSize; ++i) {
            if (board[i] == '-') {
                board[i] = kPieces[p];  // Temporarily add piece to the board.
                m.unpacked.placement_dest = i;
                GenerateMovesOfPlayer(board, 2, m, &ret);
                board[i] = '-';  // Revert the change to the board.
            }
        }
    }

    return ret;
}

static MoveArray GenerateMovesMovement(char board[static kBoardSize],
                                       int turn) {
    MoveArray ret;
    MoveArrayInit(&ret);
    GatesMove move = kGatesMoveInit;
    GenerateMovesOfPlayer(board, turn, move, &ret);

    return ret;
}

static int8_t FindGate(const char board[static kBoardSize], char gate,
                       bool second) {
    int8_t start = second * (kBoardSize - 1);
    int8_t end = kBoardSize - (kBoardSize + 1) * second;
    int8_t step = 1 - 2 * second;

    for (int8_t i = start; i != end; i += step) {
        if (board[i] == gate) return i;
    }

    NotReached("FindGate: no gates are found");
    return -1;  // Never reached.
}

static void SwapPieces(char *a, char *b) {
    char tmp = *a;
    *a = *b;
    *b = tmp;
}

static MoveArray GenerateMovesGateMoving(const GatesTier *pt,
                                         char board[static kBoardSize],
                                         int turn) {
    char gate = kPieces[!(turn - 1)];
    GatesMove move = kGatesMoveInit;
    move.unpacked.gate_src = FindGate(board, gate, pt->phase == kGate2Moving);
    MoveArray ret;
    MoveArrayInit(&ret);

    // Find all empty slots where the gate can go, INCLUDING THE ORIGINAL SLOT.
    for (int8_t dest = 0; dest < kBoardSize; ++dest) {
        if (board[dest] == '-' || dest == move.unpacked.gate_src) {
            move.unpacked.gate_dest = dest;

            // Temporarily move the gate on board.
            SwapPieces(&board[move.unpacked.gate_src], &board[dest]);

            // Generate all subsequent moves on top of the gate move.
            GenerateMovesOfPlayer(board, turn, move, &ret);

            // Revert the gate move.
            SwapPieces(&board[move.unpacked.gate_src], &board[dest]);
        }
    }

    return ret;
}

static MoveArray GatesGenerateMoves(TierPosition tier_position) {
    GatesTier t;
    char board[kBoardSize];
    UnhashTierAndPosition(tier_position, &t, board);

    switch (t.phase) {
        case kPlacement:
            return GenerateMovesPlacement(&t, board);

        case kMovement: {
            int turn = GenericHashGetTurnLabel(tier_position.tier,
                                               tier_position.position);
            return GenerateMovesMovement(board, turn);
        }

        case kGate1Moving:
        case kGate2Moving: {
            int turn = GenericHashGetTurnLabel(tier_position.tier,
                                               tier_position.position);
            return GenerateMovesGateMoving(&t, board, turn);
        }

        default:
            NotReached("GatesGenerateMoves: unknown phase encountered");
    }

    // Never reached.
    MoveArray error;
    MoveArrayInit(&error);

    return error;
}

static Value GatesPrimitive(TierPosition tier_position) {
    GatesTier t;
    GatesTierUnhash(tier_position.tier, &t);

    static_assert(kGate1Moving < kGate2Moving, "");
    if (t.phase >= kGate1Moving) {
        // The game ends with the previous player winning by scoring their last
        // spike. Therefore, all primitive positions are losing.
        if (t.n[A] + t.n[Z] == 0) return kLose;
        if (t.n[a] + t.n[z] == 0) return kLose;
    }

    // The game can also end with the current player losing by having no moves.
    MoveArray moves = GatesGenerateMoves(tier_position);
    int64_t num_moves = moves.size;
    MoveArrayDestroy(&moves);
    if (num_moves == 0) return kLose;

    return kUndecided;
}

static void RemovePiece(char board[static kBoardSize], GatesTierField index) {
    for (GatesTierField i = index; i < kBoardSize - 1; ++i) {
        board[i] = board[i + 1];
    }
}

static Position HashWrapper(Tier tier, const GatesTier *t,
                            const char board[static kBoardSize], int turn) {
    char cleaned_board[kBoardSize];
    memcpy(cleaned_board, board, kBoardSize);
    switch (t->n[G]) {
        case 2:
            // Note that if we don't need to adjust the subsequent piece removal
            // indices only if we remove from back to front.
            RemovePiece(cleaned_board, t->G2);
            // Fall through

        case 1:
            RemovePiece(cleaned_board, t->G1);
            break;
    }

    return GenericHashHashLabel(tier, cleaned_board, turn);
}

static TierPosition HashTierAndPosition(const GatesTier *t,
                                        const char board[static kBoardSize],
                                        int turn) {
    TierPosition ret;
    ret.tier = GatesTierHash(t);
    ret.position = HashWrapper(ret.tier, t, board, turn);

    return ret;
}

/**
 * Returns 0 if board[loc] is the first gate (one with smaller board index) of
 * that color, or 1 otherwise.
 */
static int8_t GateIndex(const char board[static kBoardSize], int8_t loc) {
    for (int8_t i = 0; i < loc; ++i) {
        if (board[i] == board[loc]) return 1;
    }

    return 0;
}

/**
 * @brief Moves the piece on \p board from \p m.unpacked.move_src to
 * \p m.unpacked.move_dest and modify the GatesTier object \p t to reflect the
 * tier change.
 * @details For the number of each type of pieces as stored in \p t, one of them
 * may be decremented if the chosen move scores a spike. For the phase of the
 * tier, it may become kMovement if no scoring happens, or kGateMoving
 * otherwise. This function does not modify t->G1 and t->G2.
 */
static void PerformPieceMove(GatesTier *t, char board[static kBoardSize],
                             GatesMove m) {
    assert(board[m.unpacked.move_src] == 'A' ||
           board[m.unpacked.move_src] == 'a' ||
           board[m.unpacked.move_src] == 'Z' ||
           board[m.unpacked.move_src] == 'z');
    switch (board[m.unpacked.move_dest]) {
        // Scoring a piece
        case 'G':
        case 'g': {
            // Figure out which gate it is and change the tier phase to GM1 or
            // GM2 accordingly
            static_assert(kGate1Moving + 1 == kGate2Moving, "");
            t->phase = kGate1Moving + GateIndex(board, m.unpacked.move_dest);
            --t->n[kPieceToTypeIndex[(int8_t)board[m.unpacked.move_src]]];
            board[m.unpacked.move_src] = '-';
            assert(m.unpacked.teleport_dest < 0);
            break;
        }
        // Moving and perhaps teleporting.
        default: {
            t->phase = kMovement;
            char hold = board[m.unpacked.move_dest];
            board[m.unpacked.move_dest] = board[m.unpacked.move_src];
            board[m.unpacked.move_src] = '-';
            if (m.unpacked.teleport_dest >= 0) {
                board[m.unpacked.teleport_dest] = hold;
            }
        }
    }
}

static TierPosition DoMovePlacement(GatesTier *t, char board[static kBoardSize],
                                    int turn, Move move) {
    assert(t->phase == kPlacement);

    // Always perform the placement first.
    GatesMove m = {.hashed = move};
    assert(m.unpacked.placement_dest >= 0 && m.unpacked.placement_type >= 0);
    board[m.unpacked.placement_dest] = kPieces[m.unpacked.placement_type];
    ++t->n[m.unpacked.placement_type];

    // If placing a white gate, adjust t->G1 and t->G2 as necessary.
    if (m.unpacked.placement_type == G) {
        if (t->n[G] == 2) {
            t->G2 = m.unpacked.placement_dest;
            assert(t->G1 != t->G2);
            if (t->G1 > t->G2) SwapG(t);
        } else {
            assert(t->n[G] == 1);
            t->G1 = m.unpacked.placement_dest;
        }
    }

    // Check if also moving a black piece.
    if (m.unpacked.move_src >= 0) {
        assert(GatesTierGetNumPieces(t) == 12);
        assert(board[m.unpacked.move_src] == 'a' ||
               board[m.unpacked.move_src] == 'z');

        // In this case, The phase of the next tier is decided by the subsequent
        // piece move and set by this function call.
        PerformPieceMove(t, board, m);
    }  // Otherwise, t->phase should remain as kPlacement.

    return HashTierAndPosition(t, board, 3 - turn);
}

static TierPosition DoMoveMovement(GatesTier *t, char board[static kBoardSize],
                                   int turn, Move move) {
    assert(t->phase == kMovement);
    GatesMove m = {.hashed = move};
    // The phase of the next tier is decided by the subsequent piece move and
    // set by this function call.
    PerformPieceMove(t, board, m);

    return HashTierAndPosition(t, board, 3 - turn);
}

static TierPosition DoMoveGateMoving(GatesTier *t,
                                     char board[static kBoardSize], int turn,
                                     Move move) {
    assert(t->phase == kGate1Moving || t->phase == kGate2Moving);
    GatesMove m = {.hashed = move};
    assert((board[m.unpacked.gate_src] == 'G' && turn == 2) ||
           (board[m.unpacked.gate_src] == 'g' && turn == 1));
    assert(board[m.unpacked.gate_dest] == '-' ||
           m.unpacked.gate_dest == m.unpacked.gate_src);

    // First, move the gate.
    SwapPieces(&board[m.unpacked.gate_src], &board[m.unpacked.gate_dest]);

    // If moving a white gate, also adjust t->G1 and t->G2 as necessary.
    if (turn == 2) {
        if (t->phase == kGate1Moving) {
            assert(t->G1 == m.unpacked.gate_src);
            t->G1 = m.unpacked.gate_dest;
        } else {  // t->phase == kGate2Moving
            assert(t->G2 == m.unpacked.gate_src);
            t->G2 = m.unpacked.gate_dest;
        }
        assert(t->G1 != t->G2);
        if (t->G1 > t->G2) SwapG(t);
    }

    // Then, carry out the other piece move. The phase of the next tier is
    // decided by the subsequent piece move and set by this function call.
    PerformPieceMove(t, board, m);

    return HashTierAndPosition(t, board, 3 - turn);
}

static TierPosition DoMoveInternal(GatesTier *t, char board[static kBoardSize],
                                   int turn, Move move) {
    switch (t->phase) {
        case kPlacement:
            return DoMovePlacement(t, board, turn, move);

        case kMovement:
            return DoMoveMovement(t, board, turn, move);

        case kGate1Moving:
        case kGate2Moving:
            return DoMoveGateMoving(t, board, turn, move);

        default:
            NotReached("DoMoveInternal: unknown phase");
    }

    return kIllegalTierPosition;
}

static TierPosition GatesDoMove(TierPosition tier_position, Move move) {
    GatesTier t;
    char board[kBoardSize];
    UnhashTierAndPosition(tier_position, &t, board);
    int turn =
        GenericHashGetTurnLabel(tier_position.tier, tier_position.position);

    return DoMoveInternal(&t, board, turn, move);
}

static bool GatesIsLegalPosition(TierPosition tier_position) {
    (void)tier_position;  // There's no easy way to tell if a position is legal.
    return true;
}

static int GetChildPositionsInternalDedup(TierPosition tp,
                                          TierPositionArray *array) {
    MoveArray moves = GatesGenerateMoves(tp);
    GatesTier t, t_copy;
    char board[kBoardSize], board_copy[kBoardSize];
    UnhashTierAndPosition(tp, &t, board);
    int turn = GenericHashGetTurnLabel(tp.tier, tp.position);
    TierPositionHashSet dedup;
    TierPositionHashSetInit(&dedup, 0.5);
    for (int64_t i = 0; i < moves.size; ++i) {
        memcpy(board_copy, board, kBoardSize);
        t_copy = t;
        TierPosition child =
            DoMoveInternal(&t_copy, board_copy, turn, moves.array[i]);
        if (TierPositionHashSetContains(&dedup, child)) continue;
        TierPositionHashSetAdd(&dedup, child);
        if (array) TierPositionArrayAppend(array, child);
    }
    MoveArrayDestroy(&moves);
    int ret = (int)dedup.size;
    TierPositionHashSetDestroy(&dedup);

    return ret;
}

static int GetChildPositionsInternalNoDedup(TierPosition tp,
                                            TierPositionArray *array) {
    MoveArray moves = GatesGenerateMoves(tp);
    int ret = (int)moves.size;
    if (array != NULL) {
        GatesTier t, t_copy;
        char board[kBoardSize], board_copy[kBoardSize];
        UnhashTierAndPosition(tp, &t, board);
        int turn = GenericHashGetTurnLabel(tp.tier, tp.position);
        for (int64_t i = 0; i < moves.size; ++i) {
            memcpy(board_copy, board, kBoardSize);
            t_copy = t;
            TierPosition child =
                DoMoveInternal(&t_copy, board_copy, turn, moves.array[i]);
            TierPositionArrayAppend(array, child);
        }
    }
    MoveArrayDestroy(&moves);

    return ret;
}

/**
 * @brief Inserts all child positions of \p tp in \p array if \p array is
 * non-NULL, and returns the number of child positions found.
 */
static int GetChildPositionsInternal(TierPosition tp,
                                     TierPositionArray *array) {
    if (TierHashSetContains(&kChildDedupTiers, tp.tier)) {
        // Need child deduplication
        return GetChildPositionsInternalDedup(tp, array);
    }

    // No deduplication required.
    return GetChildPositionsInternalNoDedup(tp, array);
}

static int GatesGetNumberOfCanonicalChildPositions(TierPosition tp) {
    return GetChildPositionsInternal(tp, NULL);
}

static TierPositionArray GatesGetCanonicalChildPositions(TierPosition tp) {
    TierPositionArray ret;
    TierPositionArrayInit(&ret);
    GetChildPositionsInternal(tp, &ret);

    return ret;
}

static void GetCanonicalParentsMovementTriangle(Tier tier, const GatesTier *t,
                                                char board[static kBoardSize],
                                                int8_t dest, int opponent_turn,
                                                PositionArray *ret) {
    assert(board[dest] == 'A' || board[dest] == 'a');

    // For all first level adjacencies, the space could be a source only if it
    // is blank.
    for (int8_t i = 0; i < kBoardAdjacency1Size[dest]; ++i) {
        int8_t src = kBoardAdjacency1[dest][i];
        if (board[src] == '-') {
            SwapPieces(&board[src], &board[dest]);
            Position parent = HashWrapper(tier, t, board, opponent_turn);
            SwapPieces(&board[src], &board[dest]);
            PositionArrayAppend(ret, parent);
        }
    }

    // For all second level adjacencies, the space could be a source if
    // itself is blank and at least one of its blocking points is blank.
    for (int8_t i = 0; i < kBoardAdjacency2Size[dest]; ++i) {
        int8_t src = kBoardAdjacency2[dest][i];
        bool unoccupied = board[src] == '-';
        bool reachable =
            board[kBoardAdjacency2BlockingPoints[dest][i][0]] == '-' ||
            board[kBoardAdjacency2BlockingPoints[dest][i][1]] == '-';
        if (unoccupied && reachable) {
            SwapPieces(&board[src], &board[dest]);
            Position parent = HashWrapper(tier, t, board, opponent_turn);
            SwapPieces(&board[src], &board[dest]);
            PositionArrayAppend(ret, parent);
        }
    }
}

static void GetCanonicalParentsMovementTrapezoid(Tier tier, const GatesTier *t,
                                                 char board[static kBoardSize],
                                                 int8_t dest, int opponent_turn,
                                                 PositionArray *ret) {
    assert(board[dest] == 'Z' || board[dest] == 'z');
    // offset == 0 if black is opponent from previous turn, 32 if white.
    const char offset = ('z' - board[dest]);
    const char friendly_triangle = 'A' + offset;
    const char friendly_trapezoid = 'Z' + offset;

    // Collect the locations of all friendly spikes.
    int8_t friendly_spikes[kBoardSize];
    int8_t num_friendly_spikes = 0;
    for (int8_t i = 0; i < kBoardSize; ++i) {
        if (board[i] == friendly_triangle || board[i] == friendly_trapezoid) {
            friendly_spikes[num_friendly_spikes++] = i;
        }
    }

    // For all first level adjacencies, the space could be a source if it is
    // (A) blank or (B) a friendly spike which got teleported there in the
    // previous turn.
    for (int8_t i = 0; i < kBoardAdjacency1Size[dest]; ++i) {
        int8_t src = kBoardAdjacency1[dest][i];
        if (board[src] == '-') {
            // Case A.1: move only.
            SwapPieces(&board[src], &board[dest]);  // Move trapezoid to src.
            Position parent = HashWrapper(tier, t, board, opponent_turn);
            PositionArrayAppend(ret, parent);

            // Case A.2: move and teleport.
            for (int8_t j = 0; j < num_friendly_spikes; ++j) {
                // "Un-teleport" friendly spike.
                SwapPieces(&board[dest], &board[friendly_spikes[j]]);
                parent = HashWrapper(tier, t, board, opponent_turn);
                // Revert un-teleportation.
                SwapPieces(&board[dest], &board[friendly_spikes[j]]);
                PositionArrayAppend(ret, parent);
            }
            SwapPieces(&board[src], &board[dest]);  // Move trapezoid back.
        } else if (board[src] == friendly_triangle ||
                   board[src] == friendly_trapezoid) {
            // Case B: move and teleport to the source space.
            SwapPieces(&board[src], &board[dest]);
            Position parent = HashWrapper(tier, t, board, opponent_turn);
            SwapPieces(&board[src], &board[dest]);
            PositionArrayAppend(ret, parent);
        }
    }
}

static PositionArray GetCanonicalParentsOfMovement(
    Tier tier, const GatesTier *ct, char board[static kBoardSize], int turn) {
    //
    assert(ct->phase == kMovement);
    int opponent_turn = 3 - turn;
    char triangle = TriangleOfPlayer(opponent_turn);
    char trapezoid = TrapezoidOfPlayer(opponent_turn);
    PositionArray ret;
    PositionArrayInit(&ret);

    // In the previous turn, the opponent may have moved one of the triangles
    // or trapezoids that are on the board.
    for (int8_t i = 0; i < kBoardSize; ++i) {
        if (board[i] == triangle) {
            GetCanonicalParentsMovementTriangle(tier, ct, board, i,
                                                opponent_turn, &ret);
        } else if (board[i] == trapezoid) {
            GetCanonicalParentsMovementTrapezoid(tier, ct, board, i,
                                                 opponent_turn, &ret);
        }
    }

    return ret;
}

static void GetCanonicalParentsGateMovingHelper(Tier parent_tier,
                                                const GatesTier *pt,
                                                int8_t dest, char piece,
                                                char board[static kBoardSize],
                                                PositionArray *ret) {
    //
    assert(board[dest] == 'G' || board[dest] == 'g');
    assert((islower(piece) && islower(board[dest])) ||
           (!islower(piece) && !islower(board[dest])));
    int opponent_turn = ((bool)islower(piece)) + 1;

    // For both triangle and trapezoid moves, we need to consider all first
    // level adjacencies and the space could be a source only if it is blank.
    for (int8_t i = 0; i < kBoardAdjacency1Size[dest]; ++i) {
        int8_t src = kBoardAdjacency1[dest][i];
        if (board[src] == '-') {
            board[src] = piece;
            Position parent =
                HashWrapper(parent_tier, pt, board, opponent_turn);
            board[src] = '-';
            PositionArrayAppend(ret, parent);
        }
    }

    // For triangle moves only, we also need to consider second level
    // adjacencies. A space could be a source only if it is blank.
    if (piece == 'A' || piece == 'a') {
        for (int8_t i = 0; i < kBoardAdjacency2Size[dest]; ++i) {
            int8_t src = kBoardAdjacency2[dest][i];
            bool unoccupied = board[src] == '-';
            bool reachable =
                board[kBoardAdjacency2BlockingPoints[dest][i][0]] == '-' ||
                board[kBoardAdjacency2BlockingPoints[dest][i][1]] == '-';
            if (unoccupied && reachable) {
                board[src] = piece;
                Position parent =
                    HashWrapper(parent_tier, pt, board, opponent_turn);
                board[src] = '-';
                PositionArrayAppend(ret, parent);
            }
        }
    }
}

static PositionArray GetCanonicalParentsOfGateMoving(
    Tier parent_tier, const GatesTier *ct, char board[static kBoardSize],
    int turn) {
    //
    GatesTier pt;
    GatesTierUnhash(parent_tier, &pt);
    assert(ct->phase == kGate1Moving || ct->phase == kGate2Moving);
    assert(pt.phase == kMovement);

    const int offset = (turn - 1) * ('g' - 'G');
    const char opponent_gate = 'g' - offset;
    const char opponent_triangle = 'a' - offset;
    const char opponent_trapezoid = 'z' - offset;
    const int opponent_triangle_piece_index = A + (turn == 1);
    const int opponent_trapezoid_piece_index = Z + (turn == 1);
    int8_t gate_index =
        FindGate(board, opponent_gate, ct->phase == kGate2Moving);

    PositionArray ret;
    PositionArrayInit(&ret);
    if (ct->n[opponent_triangle_piece_index] + 1 ==
        pt.n[opponent_triangle_piece_index]) {
        // opponent scored a triangle in the previous turn
        GetCanonicalParentsGateMovingHelper(parent_tier, &pt, gate_index,
                                            opponent_triangle, board, &ret);
    }
    if (ct->n[opponent_trapezoid_piece_index] + 1 ==
        pt.n[opponent_trapezoid_piece_index]) {
        // opponent scored a trapezoid in the previous turn
        GetCanonicalParentsGateMovingHelper(parent_tier, &pt, gate_index,
                                            opponent_trapezoid, board, &ret);
    }

    return ret;
}

// This function only works if the parent tier is in the Movement phase,
// which is the only type of tiers that uses the loopy solving algorithm.
static PositionArray GatesGetCanonicalParentPositions(TierPosition child,
                                                      Tier parent_tier) {
    GatesTier ct;
    char board[kBoardSize];
    UnhashTierAndPosition(child, &ct, board);
    int turn = GenericHashGetTurnLabel(child.tier, child.position);

    switch (ct.phase) {
        case kMovement:
            assert(parent_tier == child.tier);
            return GetCanonicalParentsOfMovement(child.tier, &ct, board, turn);

        case kGate1Moving:
        case kGate2Moving:
            return GetCanonicalParentsOfGateMoving(parent_tier, &ct, board,
                                                   turn);

        default:
            NotReached(
                "GatesGetCanonicalParentPositions: unsupported child tier "
                "phase");
    }

    // Never reached.
    PositionArray error;
    PositionArrayInit(&error);
    return error;
}

static const TierSolverApi kGatesSolverApi = {
    .GetInitialTier = GatesGetInitialTier,
    .GetInitialPosition = GatesGetInitialPosition,
    .GetTierSize = GatesGetTierSize,

    .GenerateMoves = GatesGenerateMoves,
    .Primitive = GatesPrimitive,
    .DoMove = GatesDoMove,
    .IsLegalPosition = GatesIsLegalPosition,
    .GetCanonicalPosition = NULL,  // No symmetries within each tier.
    .GetNumberOfCanonicalChildPositions =
        GatesGetNumberOfCanonicalChildPositions,
    .GetCanonicalChildPositions = GatesGetCanonicalChildPositions,
    .GetCanonicalParentPositions = GatesGetCanonicalParentPositions,

    .GetPositionInSymmetricTier = NULL,  // Symmetry removal disabled for now.
    .GetChildTiers = GatesGetChildTiers,
    .GetTierType = GatesGetTierType,
    .GetCanonicalTier = NULL,  // Symmetry removal disabled for now.
    .GetTierName = GatesGetTierName,
};

// ============================= kGatesGameplayApi =============================

static const char kGatesPositionStringFormat[] =
    "            LEGEND                            TOTAL\n"
    "\n"
    "|        1     2     3       |  :          %c     %c     %c\n"
    "|                            |  :\n"
    "|     4     5     6     7    |  :       %c     %c     %c     %c\n"
    "|                            |  :\n"
    "|  8     9          10    11 |  :    %c     %c           %c     %c\n"
    "|                            |  :\n"
    "|    12    13    14    15    |  :       %c     %c     %c     %c\n"
    "|                            |  :\n"
    "|       16    17    18       |  :          %c     %c     %c";

#ifndef NDEBUG
static void DebugPrintTierPosition(TierPosition tier_position) {
    GatesTier t;
    char board[kBoardSize];
    UnhashTierAndPosition(tier_position, &t, board);
    printf("Tier %" PRITier ", Position %" PRIPos "\n", tier_position.tier,
           tier_position.position);
    printf("Phase: ");
    switch (t.phase) {
        case kPlacement:
            printf("Placement\n");
            break;
        case kMovement:
            printf("Movement\n");
            break;
        case kGate1Moving:
            printf("Gate 1 Moving\n");
            break;
        case kGate2Moving:
            printf("Gate 2 Moving\n");
            break;
    }
    printf(
        "According to tier label, there are \n"
        "%d G, %d g,\n"
        "%d A, %d a,\n"
        "%d Z, %d z.\n",
        t.n[G], t.n[g], t.n[A], t.n[a], t.n[Z], t.n[z]);
    printf("The two white gates are at indices %d and %d\n", t.G1, t.G2);
}
#endif

static int GatesTierPositionToString(TierPosition tier_position, char *buffer) {
    GatesTier t;
    char board[kBoardSize];
    UnhashTierAndPosition(tier_position, &t, board);
#ifndef NDEBUG
    DebugPrintTierPosition(tier_position);
#endif
    sprintf(buffer, kGatesPositionStringFormat, board[0], board[1], board[2],
            board[3], board[4], board[5], board[6], board[7], board[8],
            board[9], board[10], board[11], board[12], board[13], board[14],
            board[15], board[16], board[17]);

    return kNoError;
}

static int GatesMoveToString(Move move, char *buffer) {
    GatesMove m = {.hashed = move};
    bool placement = m.unpacked.placement_type >= 0;
    bool movement = m.unpacked.move_src >= 0;
    bool gate_movement = m.unpacked.gate_src >= 0;
    bool teleport = m.unpacked.teleport_dest >= 0;
    bool first_chunk = true;
    int count = 0;
    if (placement) {
        count += sprintf(buffer + count, "p %c %" PRId8,
                         kPieces[m.unpacked.placement_type],
                         m.unpacked.placement_dest + 1);
        first_chunk = false;
    }
    if (gate_movement) {
        count += sprintf(buffer + count, "g %" PRId8 " %" PRId8,
                         m.unpacked.gate_src + 1, m.unpacked.gate_dest + 1);
        first_chunk = false;
    }
    if (movement) {
        count += sprintf(buffer + count, "%sm %" PRId8 " %" PRId8,
                         first_chunk ? "" : " ", m.unpacked.move_src + 1,
                         m.unpacked.move_dest + 1);
    }
    if (teleport) {
        sprintf(buffer + count, " t %" PRId8, m.unpacked.teleport_dest + 1);
    }

    return kNoError;
}

static bool GatesIsValidMoveString(ReadOnlyString move_string) {
    (void)move_string;

    return true;  // TODO
}

static Move GatesStringToMove(ReadOnlyString move_string) {
    static ConstantReadOnlyString delim = " ";
    char move_string_copy[20];
    char *saveptr;
    strcpy(move_string_copy, move_string);
    char *tokens[10] = {strtok_r(move_string_copy, delim, &saveptr)};
    for (int i = 1; i < 8; ++i) {
        tokens[i] = strtok_r(NULL, delim, &saveptr);
    }

    GatesMove m = kGatesMoveInit;
    int i = 0;
    while (i < 8 && tokens[i] != NULL) {
        if (strcmp(tokens[i], "p") == 0) {
            m.unpacked.placement_type =
                kPieceToTypeIndex[(int8_t)tokens[i + 1][0]];
            m.unpacked.placement_dest = atoi(tokens[i + 2]) - 1;
            i += 3;
        } else if (strcmp(tokens[i], "g") == 0) {
            m.unpacked.gate_src = atoi(tokens[i + 1]) - 1;
            m.unpacked.gate_dest = atoi(tokens[i + 2]) - 1;
            i += 3;
        } else if (strcmp(tokens[i], "m") == 0) {
            m.unpacked.move_src = atoi(tokens[i + 1]) - 1;
            m.unpacked.move_dest = atoi(tokens[i + 2]) - 1;
            i += 3;
        } else {  // "t"
            m.unpacked.teleport_dest = atoi(tokens[i + 1]) - 1;
            i += 2;
        }
    }

    return m.hashed;
}

static const GameplayApiCommon GatesGameplayApiCommon = {
    .GetInitialPosition = GatesGetInitialPosition,
    .position_string_length_max = sizeof(kGatesPositionStringFormat),

    .move_string_length_max = 19,
    .MoveToString = GatesMoveToString,

    .IsValidMoveString = GatesIsValidMoveString,
    .StringToMove = GatesStringToMove,
};

static const GameplayApiTier GatesGameplayApiTier = {
    .GetInitialTier = GatesGetInitialTier,

    .TierPositionToString = GatesTierPositionToString,

    .GenerateMoves = GatesGenerateMoves,
    .DoMove = GatesDoMove,
    .Primitive = GatesPrimitive,
};

static const GameplayApi kGatesGameplayApi = {
    .common = &GatesGameplayApiCommon,
    .tier = &GatesGameplayApiTier,
};

// ================================= GatesInit =================================

static void PlacementIndexToGatesTier(int i, GatesTier *dest) {
    for (int p = 0; p < kNumPieceTypes; ++p) {
        dest->n[p] = i % 3;
        i /= 3;
    }
}

static void FillPieceInitArray(const GatesTier *t, int piece_init[static 19]) {
    piece_init[1] = piece_init[2] = t->n[g];
    piece_init[4] = piece_init[5] = t->n[A];
    piece_init[7] = piece_init[8] = t->n[a];
    piece_init[10] = piece_init[11] = t->n[Z];
    piece_init[13] = piece_init[14] = t->n[z];
    piece_init[16] = piece_init[17] = kBoardSize - GatesTierGetNumPieces(t);
}

static bool InitChildDedupTiers(void) {
    TierHashSetInit(&kChildDedupTiers, 0.5);
    GatesTier t;
    t.phase = kPlacement;
    t.n[G] = t.n[g] = t.n[A] = t.n[a] = t.n[Z] = t.n[z] = 2;
    static_assert(A + 1 == a && a + 1 == Z && Z + 1 == z, "");
    for (int p = A; p <= z; ++p) {
        t.n[p] = 1;
        for (t.G1 = 0; t.G1 < kBoardSize; ++t.G1) {
            for (t.G2 = t.G1 + 1; t.G2 < kBoardSize; ++t.G2) {
                Tier adding = GatesTierHash(&t);
                bool success = TierHashSetAdd(&kChildDedupTiers, adding);
                if (!success) {
                    TierHashSetDestroy(&kChildDedupTiers);
                    return false;
                }
#ifndef NDEBUG
                char name[kDbFileNameLengthMax + 1];
                GatesGetTierName(adding, name);
                printf("Adding [%s] (#%" PRITier ") to kChildDedupTiers\n",
                       name, adding);
#endif  // NDEBUG
            }
        }
        t.n[p] = 2;
    }

    return true;
}

static int InitGenericHashPlacement(void) {
    GatesTier t = {0};
    int piece_init[] = {
        'g', 0, 2, 'A', 0, 2, 'a', 0, 2, 'Z', 0, 2, 'z', 0, 2, '-', 6, 18, -1,
    };  // This is a placeholder definition and will be filled inside the loop.
    t.phase = kPlacement;

    // For each possible combination of pieces on board: 3 ** 6 == 729.
    for (int i = 0; i < 729; ++i) {
        PlacementIndexToGatesTier(i, &t);
        GatesTierField num_pieces = GatesTierGetNumPieces(&t);
        int turn = (num_pieces % 2 == 0) ? 1 : 2;
        FillPieceInitArray(&t, piece_init);
        Tier hashed;
        bool success = false;
        switch (t.n[G]) {
            case 0:  // No white gates
                t.G1 = t.G2 = 0;
                hashed = GatesTierHash(&t);
                success = GenericHashAddContext(turn, kBoardSize, piece_init,
                                                NULL, hashed);
                if (!success) return kGenericHashError;
                break;

            case 1:  // Only one white gate has been placed
                t.G2 = 0;
                for (t.G1 = 0; t.G1 < kBoardSize; ++t.G1) {
                    hashed = GatesTierHash(&t);
                    success = GenericHashAddContext(turn, kBoardSize - 1,
                                                    piece_init, NULL, hashed);
                    if (!success) return kGenericHashError;
                }
                break;

            case 2:  // Both white gates have been placed
                for (t.G1 = 0; t.G1 < kBoardSize; ++t.G1) {
                    for (t.G2 = t.G1 + 1; t.G2 < kBoardSize; ++t.G2) {
                        hashed = GatesTierHash(&t);
                        success = GenericHashAddContext(
                            turn, kBoardSize - 2, piece_init, NULL, hashed);
                        if (!success) return kGenericHashError;
                    }
                }
                break;
        }
    }

    return kNoError;
}

static void MovementIndexToGatesTier(int i, GatesTier *dest) {
    dest->n[A] = i % 3;
    i /= 3;
    dest->n[a] = i % 3;
    i /= 3;
    dest->n[Z] = i % 3;
    i /= 3;
    dest->n[z] = i;
}

static int InitGenericHashMovement(void) {
    GatesTier t = {0};
    int piece_init[] = {
        'g', 0, 2, 'A', 0, 2, 'a', 0, 2, 'Z', 0, 2, 'z', 0, 2, '-', 6, 18, -1,
    };  // This is a placeholder definition and will be filled inside the loop.
    t.n[G] = t.n[g] = 2;  // All four gates must be present.
    // For each possible combination of non-gate pieces on board: 3 ** 4 == 81.
    // Note that some combinations are invalid. However, including them in the
    // Generic Hash system should not be a bug.
    for (int i = 0; i < 81; ++i) {
        MovementIndexToGatesTier(i, &t);
        FillPieceInitArray(&t, piece_init);
        for (t.G1 = 0; t.G1 < kBoardSize; ++t.G1) {
            for (t.G2 = t.G1 + 1; t.G2 < kBoardSize; ++t.G2) {
                // Movement phase, not moving a gate this turn.
                t.phase = kMovement;
                Tier hashed = GatesTierHash(&t);
                bool success = GenericHashAddContext(0, kBoardSize - 2,
                                                     piece_init, NULL, hashed);
                if (!success) return kGenericHashError;

                // Movement phase, also moving a gate this turn.
                for (t.phase = kGate1Moving; t.phase <= kGate2Moving;
                     ++t.phase) {
                    hashed = GatesTierHash(&t);
                    int turn = 0;
                    if (t.n[A] + t.n[Z] == 4) {
                        turn =
                            1;  // White haven't scored, so it's white's turn.
                    } else if (t.n[a] + t.n[z] == 4) {
                        turn =
                            2;  // Black haven't scored, so it's black's turn.
                    } else if (t.n[A] + t.n[Z] == 0) {
                        turn = 2;  // White just won the game, so it's black's
                                   // turn.
                    } else if (t.n[a] + t.n[z] == 0) {
                        turn = 1;  // Black just won the game, so it's white's
                                   // turn.
                    }
                    // "t.n[A] + t.n[Z] + t.n[a] + t.n[z] == 8" and
                    // "t.n[A] + t.n[Z] + t.n[a] + t.n[z] == 0" are both invalid
                    // tiers.
                    success = GenericHashAddContext(turn, kBoardSize - 2,
                                                    piece_init, NULL, hashed);
                }
                if (!success) return kGenericHashError;
            }
        }
    }

    return kNoError;
}

static int InitGenericHash(void) {
    GenericHashReinitialize();
    int error = InitGenericHashPlacement();
    if (error) return error;
    error = InitGenericHashMovement();
    if (error) return error;

    return kNoError;
}

static int GatesInit(void *aux) {
    (void)aux;  // Unused.

    // Initialize piece to piece type index map.
    kPieceToTypeIndex[(int8_t)'G'] = G;
    kPieceToTypeIndex[(int8_t)'g'] = g;
    kPieceToTypeIndex[(int8_t)'A'] = A;
    kPieceToTypeIndex[(int8_t)'a'] = a;
    kPieceToTypeIndex[(int8_t)'Z'] = Z;
    kPieceToTypeIndex[(int8_t)'z'] = z;

    // Initialize kChildDedupTiers, which contains tiers that contain positions
    // from which different moves can lead to the same child.
    if (!InitChildDedupTiers()) return kMallocFailureError;

    // InitGenericHash();
    // TierPosition child = {.tier = 8646730, .position = 111};
    // TierPosition not_found_parent = {.tier = 8638794, .position = 1830};
    // PositionArray parents = GatesGetCanonicalParentPositions(child, 8638794);
    // TierPositionArray children_of_not_found_parent =
    //     DefaultGetCanonicalChildPositions(not_found_parent);

    // char buffer[1000];
    // GatesTierPositionToString(child, buffer);
    // printf("child:\n%s\n\n", buffer);

    // GatesTierPositionToString(not_found_parent, buffer);
    // printf("not found parent:\n%s\n\n", buffer);

    // for (int64_t i = 0; i < children_of_not_found_parent.size; ++i) {
    //     GatesTierPositionToString(children_of_not_found_parent.array[i],
    //                               buffer);
    //     printf("children_of_not_found_parent %" PRId64 ":\n%s\n\n", i,
    //     buffer);
    // }

    // for (int64_t i = 0; i < parents.size; ++i) {
    //     TierPosition parent = {.tier = 8638794, .position =
    //     parents.array[i]}; GatesTierPositionToString(parent, buffer);
    //     printf("parent %" PRId64 ":\n%s\n\n", i, buffer);
    // }

    // return kNoError;

    return InitGenericHash();
}

// =============================== GatesFinalize ===============================

static int GatesFinalize(void) {
    TierHashSetDestroy(&kChildDedupTiers);
    return kNoError;
}

// ================================== kGates ==================================

const Game kGates = {
    .name = "gates",
    .formal_name = "Gates",
    .solver = &kTierSolver,
    .solver_api = &kGatesSolverApi,
    .gameplay_api = &kGatesGameplayApi,
    .uwapi = NULL,  // TODO

    .Init = GatesInit,
    .Finalize = GatesFinalize,

    .GetCurrentVariant = NULL,  // No other variants for now.
    .SetVariantOption = NULL,
};

// NOLINTEND(cppcoreguidelines-narrowing-conversions)
