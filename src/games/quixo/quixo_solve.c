#include "games/quixo/quixo_solve.h"

#include "core/types/gamesman_types.h"
#include "games/quixo/quixo_variants.h"

enum QuixoPieces { kBlank = '-', kX = 'X', kO = 'O' };
// Cached turn-to-piece mapping.
static const char kPlayerPiece[3] = {kBlank, kX, kO};

Tier QuixoGetInitialTier(void) { return QuixoCurrentVariantGetInitialTier(); }

Position QuixoGetInitialPosition(void) {
    return QuixoCurrentVariantGetInitialPosition();
}

int64_t QuixoGetTierSize(Tier tier) {
    return GenericHashNumPositionsLabel(tier);
}

MoveArray QuixoGenerateMoves(TierPosition tier_position) {
    MoveArray moves;
    MoveArrayInit(&moves);
    Tier tier = tier_position.tier;
    Position position = tier_position.position;
    char board[kBoardSizeMax];
    bool success = GenericHashUnhashLabel(tier, position, board);
    if (!success) {
        moves.size = -1;
        return moves;
    }

    int turn = GenericHashGetTurnLabel(tier, position);
    char piece_to_move = kPlayerPiece[turn];
    // Find all blank or friendly pieces on 4 edges and 4 corners of the board.
    for (int i = 0; i < num_edge_slots; ++i) {
        int edge_index = edge_indices[i];
        char piece = board[edge_index];
        // Skip opponent pieces, which cannot be moved.
        if (piece != kBlank && piece != piece_to_move) continue;

        for (int j = 0; j < edge_move_count[i]; ++j) {
            Move move = ConstructMove(edge_index, edge_move_dests[i][j]);
            MoveArrayAppend(&moves, move);
        }
    }

    return moves;
}

Value QuixoPrimitive(TierPosition tier_position) {
    Tier tier = tier_position.tier;
    Position position = tier_position.position;
    char board[kBoardSizeMax];
    bool success = GenericHashUnhashLabel(tier, position, board);
    if (!success) return kErrorValue;

    int turn = GenericHashGetTurnLabel(tier, position);
    assert(turn == 1 || turn == 2);
    char my_piece = kPlayerPiece[turn];
    char opponent_piece = kPlayerPiece[OpponentsTurn(turn)];

    if (HasKInARow(board, my_piece)) {
        // The current player wins if there is a k in a row of the current
        // player's piece, regardless of whether there is a k in a row of the
        // opponent's piece.
        return kWin;
    } else if (HasKInARow(board, opponent_piece)) {
        // If the current player is not winning but there's a k in a row of the
        // opponent's piece, then the current player loses.
        return kLose;
    }

    // Neither side is winning.
    return kUndecided;
}

static int Abs(int n) { return n >= 0 ? n : -n; }

static void MoveAndShiftPieces(char *board, int src, int dest,
                               char piece_to_move) {
    int offset = GetShiftOffset(src, dest);
    for (int i = src; i != dest; i += offset) {
        board[i] = board[i + offset];
    }
    board[dest] = piece_to_move;
}

// TODO for Robert: clean up this function.
TierPosition QuixoDoMove(TierPosition tier_position, Move move) {
    Tier tier = tier_position.tier;
    Position position = tier_position.position;
    char board[kBoardSizeMax];
    bool success = GenericHashUnhashLabel(tier, position, board);
    if (!success) return kIllegalTierPosition;

    // Get turn and piece for current player.
    int turn = GenericHashGetTurnLabel(tier, position);
    assert(turn == 1 || turn == 2);
    char piece_to_move = kPlayerPiece[turn];

    // Unpack move.
    int src, dest;
    UnpackMove(move, &src, &dest);
    assert(src >= 0 && src < GetBoardSize());
    assert(dest >= 0 && dest < GetBoardSize());
    assert(src != dest);

    // Update tier.
    TierPosition ret = {.tier = tier};
    if (board[src] == kBlank) {
        int num_blanks, num_x, num_o;
        UnhashTier(tier, &num_blanks, &num_x, &num_o);
        --num_blanks;
        num_x += (kPlayerPiece[turn] == kX);
        num_o += (kPlayerPiece[turn] == kO);
        ret.tier = HashTier(num_blanks, num_x, num_o);
    }

    // Move and shift pieces.
    MoveAndShiftPieces(board, src, dest, piece_to_move);
    ret.position = GenericHashHashLabel(ret.tier, board, OpponentsTurn(turn));

    return ret;
}

// Helper function for shifting a row or column.
static int GetShiftOffset(int src, int dest) {
    // 1 for row move, 0 for column move
    int isRowMove = Abs(dest - src) < board_cols;
    // 1 if dest > src, -1 if dest < src
    int direction = (dest > src) - (dest < src);
    // 1 for right, -1 for left, 0 for column moves
    int rowOffset = direction * isRowMove;
    // board_cols for down, -board_cols for up, 0 for row moves
    int colOffset = direction * (board_cols) * (!isRowMove);
    int offset = rowOffset + colOffset;
    assert(Abs(src - dest) % Abs(offset) == 0);

    return offset;
}

// Returns if a pos is legal, but not strictly according to game definition.
// In X's turn, returns illegal if no border Os, and vice versa
// Will not misidentify legal as illegal, but might misidentify illegal as
// legal.
bool QuixoIsLegalPosition(TierPosition tier_position) {
    Tier tier = tier_position.tier;              // get tier
    Position position = tier_position.position;  // get pos
    if (tier == initial_tier && position == initial_position) {
        // The initial position is always legal but does not follow the rules
        // below.
        return true;
    }

    char board[kBoardSizeMax];
    bool success = GenericHashUnhashLabel(tier, position, board);
    if (!success) {
        fprintf(stderr,
                "QuixoIsLegalPosition: GenericHashUnhashLabel error on tier "
                "%" PRITier " position %" PRIPos "\n",
                tier, position);
        return false;
    }

    int turn = GenericHashGetTurnLabel(tier, position);
    char opponent_piece = kPlayerPiece[OpponentsTurn(turn)];
    for (int i = 0; i < num_edge_slots; ++i) {
        int edge_index = edge_indices[i];
        char piece = board[edge_index];
        if (piece == opponent_piece) return true;
    }

    return false;
}

static int NumSymmetries(void) { return (board_rows == board_cols) ? 8 : 4; }

static Position QuixoDoSymmetry(Tier tier, const char *original_board, int turn,
                                int symmetry) {
    char symmetry_board[kBoardSizeMax];

    // Copy from the symmetry matrix.
    for (int i = 0; i < GetBoardSize(); ++i) {
        symmetry_board[i] = original_board[symmetry_matrix[symmetry][i]];
    }

    return GenericHashHashLabel(tier, symmetry_board, turn);
}

Position QuixoGetCanonicalPosition(TierPosition tier_position) {
    Tier tier = tier_position.tier;
    Position position = tier_position.position;
    char board[kBoardSizeMax];
    bool success = GenericHashUnhashLabel(tier, position, board);
    if (!success) return kIllegalPosition;

    int turn = GenericHashGetTurnLabel(tier, position);
    Position canonical_position = position;
    for (int i = 0; i < NumSymmetries(); ++i) {
        Position new_position = QuixoDoSymmetry(tier, board, turn, i);
        if (new_position < canonical_position) {
            canonical_position = new_position;
        }
    }

    return canonical_position;
}

PositionArray QuixoGetCanonicalParentPositions(TierPosition tier_position,
                                               Tier parent_tier) {
    //
    Tier tier = tier_position.tier;
    Position position = tier_position.position;
    PositionArray parents;
    PositionArrayInit(&parents);

    int turn = GenericHashGetTurnLabel(tier, position);
    assert(turn == 1 || turn == 2);
    int prior_turn = OpponentsTurn(turn);  // Player who made the last move.
    bool flipped = (tier != parent_tier);  // Tier must be different if flipped.
    char opponents_piece = kPlayerPiece[prior_turn];
    char piece_to_move = flipped ? kBlank : opponents_piece;

    char board[kBoardSizeMax];
    bool success = GenericHashUnhashLabel(tier, position, board);
    if (!success) {
        fprintf(
            stderr,
            "QuixoGetCanonicalParentPositions: GenericHashUnhashLabel error\n");
        return parents;
    }

    PositionHashSet deduplication_set;
    PositionHashSetInit(&deduplication_set, 0.5);
    for (int i = 0; i < num_edge_slots; ++i) {
        int dest = edge_indices[i];
        for (int j = 0; j < edge_move_count[i]; ++j) {
            int src = edge_move_dests[i][j];
            if (board[src] != opponents_piece) continue;

            // An opponent move from dest to src is possible. Hence we "undo"
            // that move here by moving the opponent's piece from src to dest.
            MoveAndShiftPieces(board, src, dest, piece_to_move);  // Undo move
            TierPosition parent = {.tier = parent_tier};
            parent.position =
                GenericHashHashLabel(parent_tier, board, prior_turn);
            MoveAndShiftPieces(board, dest, src, opponents_piece);  // Restore
            parent.position = QuixoGetCanonicalPosition(parent);
            if (PositionHashSetContains(&deduplication_set, parent.position)) {
                continue;  // Already included.
            }
            PositionHashSetAdd(&deduplication_set, parent.position);
            PositionArrayAppend(&parents, parent.position);
        }
    }
    PositionHashSetDestroy(&deduplication_set);

    return parents;
}

Position QuixoGetPositionInSymmetricTier(TierPosition tier_position,
                                         Tier symmetric) {
    Tier this_tier = tier_position.tier;
    Position this_position = tier_position.position;
    assert(QuixoGetCanonicalTier(symmetric) ==
           QuixoGetCanonicalTier(this_tier));

    char board[kBoardSizeMax];
    bool success = GenericHashUnhashLabel(this_tier, this_position, board);
    if (!success) return -1;

    // Swap all 'X' and 'O' pieces on the board.
    for (int i = 0; i < GetBoardSize(); ++i) {
        if (board[i] == kX) {
            board[i] = kO;
        } else if (board[i] == kO) {
            board[i] = kX;
        }
    }

    int turn = GenericHashGetTurnLabel(this_tier, this_position);
    int opponents_turn = OpponentsTurn(turn);

    return GenericHashHashLabel(symmetric, board, opponents_turn);
}

TierArray QuixoGetChildTiers(Tier tier) {
    int num_blanks, num_x, num_o;
    UnhashTier(tier, &num_blanks, &num_x, &num_o);
    int board_size = GetBoardSize();
    assert(num_blanks + num_x + num_o == board_size);
    assert(num_blanks >= 0 && num_x >= 0 && num_o >= 0);
    TierArray children;
    TierArrayInit(&children);
    if (num_blanks == board_size) {
        Tier next = HashTier(num_blanks - 1, 1, 0);
        TierArrayAppend(&children, next);
    } else if (num_blanks == board_size - 1) {
        Tier next = HashTier(num_blanks - 1, 1, 1);
        TierArrayAppend(&children, next);
    } else if (num_blanks > 0) {
        Tier x_flipped = HashTier(num_blanks - 1, num_x + 1, num_o);
        Tier o_flipped = HashTier(num_blanks - 1, num_x, num_o + 1);
        TierArrayAppend(&children, x_flipped);
        TierArrayAppend(&children, o_flipped);
    }  // no children for tiers with num_blanks == 0

    return children;
}
