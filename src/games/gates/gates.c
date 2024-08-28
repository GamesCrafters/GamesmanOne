#include "games/gates/gates.h"

#include <assert.h>  // assert
#include <stddef.h>  // NULL
#include <stdio.h>   // sprintf

#include "core/generic_hash/generic_hash.h"
#include "core/misc.h"
#include "core/solvers/tier_solver/tier_solver.h"
#include "core/types/gamesman_types.h"
#include "games/gates/gates_tier.h"

enum {
    kBoardAdjacency1SizeMax = 5,
    kBoardAdjacency2SizeMax = 6,
    kBoardAdjacency2BlockingPointsSize = 2,
};

static const int8_t kBoardAdjacency1[kBoardSize][kBoardAdjacency1SizeMax];
static const int8_t kBoardAdjacency1Size[kBoardSize];
static const int8_t kBoardAdjacency2[kBoardSize][kBoardAdjacency2SizeMax];
static const int8_t
    kBoardAdjacency2BlockingPoints[kBoardSize][kBoardAdjacency2SizeMax]
                                  [kBoardAdjacency2BlockingPointsSize];
static const int8_t kBoardAdjacency2Size[kBoardSize];

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
    for (GatesTierField i = kBoardSize - 1; i > index; ++i) {
        board[i] = board[i - 1];
    }
    board[index] = 'G';
}

static void UnhashTierAndPosition(TierPosition tier_position, GatesTier *t,
                                  char board[static kBoardSize]) {
    GatesTierUnhash(tier_position.tier, t);
    GenericHashUnhashLabel(tier_position.tier, tier_position.position, board);

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

static const char kPieces[kNumPieceTypes] = {'G', 'g', 'A', 'a', 'Z', 'z'};

typedef union GatesMove {
    struct {
        int8_t placement_type;
        int8_t placement_dest;
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
            .gate_dest = -1,
            .move_src = -1,
            .move_dest = -1,
            .teleport_dest = -1,
        },
};

static void GenerateMovesTriangle(const char board[static kBoardSize],
                                  int8_t src, GatesMove *move, MoveArray *ret) {
    assert(board[src] == 'A' || board[src] == 'a');
    const char gate = 'G' + board[src] - 'A';
    move->unpacked.move_src = src;

    // For all first level adjacencies, the destination can be reached if it is
    // blank or has a friendly gate on it.
    for (int i = 0; i < kBoardAdjacency1Size[src]; ++i) {
        int8_t dest = kBoardAdjacency1[src][i];
        if (board[dest] == '-' || board[dest] == gate) {
            move->unpacked.move_dest = dest;
            MoveArrayAppend(ret, move->hashed);
        }
    }

    // For all second level adjacencies, the destination can be reached if
    // itself is blank or has a friendly gate on it, and at least one of its
    // blocking points is blank.
    for (int i = 0; i < kBoardAdjacency2Size[src]; ++i) {
        int8_t dest = kBoardAdjacency2[src][i];
        bool unoccupied = board[dest] == '-' || board[dest] == gate;
        bool reachable = kBoardAdjacency2BlockingPoints[src][i][0] == '-' ||
                         kBoardAdjacency2BlockingPoints[src][i][1] == '-';
        if (unoccupied && reachable) {
            move->unpacked.move_dest = dest;
            MoveArrayAppend(ret, move->hashed);
        }
    }
}

static void GenerateMovesTrapezoid(const char board[static kBoardSize],
                                   int8_t src, GatesMove *move,
                                   MoveArray *ret) {
    assert(board[src] == 'Z' || board[src] == 'z');
    const char offset =
        (board[src] - 'Z');  // 0 if white's turn, 32 if black's turn.
    const char opponent_gate = 'g' - offset;
    const char friendly_triangle = 'A' + offset;
    const char friendly_trapezoid = 'Z' + offset;
    move->unpacked.move_src = src;

    // For all first level adjacencies, the destination can be reached if it is
    // blank, a friendly gate, or an opponent's spike. In other words, the
    // destination is blocked if it has an opponent's gate or a friendly spike.
    for (int8_t i = 0; i < kBoardAdjacency2Size[src]; ++i) {
        int8_t dest = kBoardAdjacency2[src][i];
        if (board[dest] != opponent_gate && board[dest] != friendly_triangle &&
            board[dest] != friendly_trapezoid) {
            move->unpacked.move_dest = dest;
            MoveArrayAppend(ret, move->hashed);
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
                                  GatesMove *move, MoveArray *ret) {
    char triangle = 'A' + ('a' - 'A') * (turn - 1);
    char trapezoid = 'Z' + ('z' - 'Z') * (turn - 1);
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
        m.unpacked.placement_type = p;
        for (int8_t i = 0; i < kBoardSize; ++i) {
            if (board[i] == '-') {
                board[i] = kPieces[p];  // Temporarily add piece to the board.
                m.unpacked.placement_dest = i;
                GenerateBlackMoves(board, &m, &ret);
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
    GenerateMovesOfPlayer(board, turn, &move, &ret);

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

static MoveArray GenerateMovesGateMoving(const GatesTier *pt,
                                         char board[static kBoardSize],
                                         int turn) {
    char gate = kPieces[!(turn - 1)];
    int8_t moving_gate_index = FindGate(board, gate, pt->phase == kGate2Moving);
    GatesMove move = kGatesMoveInit;
    MoveArray ret;
    MoveArrayInit(&ret);

    // Find all empty slots where the gate can go, INCLUDING THE ORIGINAL SLOT.
    for (int8_t i = 0; i < kBoardSize; ++i) {
        if (board[i] == '-' || i == moving_gate_index) {
            move.unpacked.gate_dest = i;
            GenerateMovesOfPlayer(board, turn, &move, &ret);
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

    static_assert(kGate1Moving < kGate2Moving,
                  "kGate1Moving and kGate2Moving (kGate1Moving + 1) must be "
                  "the largest two GatesPhase enum values");
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

static TierPosition GatesDoMove(TierPosition tier_position, Move move) {
    // TODO
    (void)tier_position;
    (void)move;

    TierPosition ret;
    ret.tier = -1;
    ret.position = -1;

    return ret;
}

static bool GatesIsLegalPosition(TierPosition tier_position) {
    (void)tier_position;  // There's no easy way to tell if a position is legal.
    return true;
}

static TierPositionArray GatesGetCanonicalChildPositions(
    TierPosition tier_position) {
    // TODO
    (void)tier_position;

    TierPositionArray ret;
    TierPositionArrayInit(&ret);

    return ret;
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
    .GetCanonicalChildPositions = GatesGetCanonicalChildPositions,

    .GetPositionInSymmetricTier = 1,
    .GetCanonicalTier = GatesGetCanonicalTier,
    .GetChildTiers = GatesGetChildTiers,
    .GetTierName = GatesGetTierName,
};

// ============================= kGatesGameplayApi =============================

static const GameplayApi kGatesGameplayApi = {
    .common = NULL,  // TODO
    .tier = NULL,    // TODO
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
    return InitGenericHash();
}

// =============================== GatesFinalize ===============================

static int GatesFinalize(void) { return kNoError; }

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
