/**
 * @file quixo.c
 * @author Robert Shi (robertyishi@berkeley.edu),
 * @author Maria Rufova,
 * @author Benji Xu,
 * @author Angela He,
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Quixo implementation.
 * @version 2.0.0
 * @date 2025-01-07
 *
 * @copyright This file is part of GAMESMAN, The Finite, Two-person
 * Perfect-Information Game Generator released under the GPL:
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "games/quixo/quixo.h"

#include <assert.h>  // assert
#include <stddef.h>  // NULL
#include <stdio.h>   // sprintf
#include <stdlib.h>  // atoi
#include <string.h>  // strtok_r, strlen, strcpy

#include "core/generic_hash/two_piece.h"
#include "core/solvers/tier_solver/tier_solver.h"
#include "core/types/gamesman_types.h"
#include "games/quixo/quixo_constants.h"

// =================================== Types ===================================

typedef union {
    int8_t unpacked[2];  // 0: num_x, 1: num_o
    Tier hash;
} QuixoTier;

enum {
    kLeft,
    kRight,
    kUp,
    kDown,
} QuixoMoveDirections;

typedef union {
    struct {
        int8_t dir;
        int8_t idx;
    } unpacked;
    Move hash;
} QuixoMove;

// ================================= Constants =================================

static const QuixoMove kQuixoMoveInit = {.hash = 0};

static const char kDirToChar[] = {'R', 'L', 'D', 'U'};

// ============================= Variant Settings =============================

static ConstantReadOnlyString kQuixoRuleChoices[] = {
    "5x5 5-in-a-row",
    "4x4 4-in-a-row",
    "3x3 3-in-a-row",
};

static const GameVariantOption kQuixoRules = {
    .name = "rules",
    .num_choices = sizeof(kQuixoRuleChoices) / sizeof(kQuixoRuleChoices[0]),
    .choices = kQuixoRuleChoices,
};

static GameVariantOption quixo_variant_options[2];
static int quixo_variant_option_selections[2] = {0, 0};  // 5x5
static int curr_variant_idx = 0;
static int side_length = 5;
static int board_size = 25;
static QuixoTier initial_tier = {.unpacked = {0, 0}};

static GameVariant current_variant = {
    .options = quixo_variant_options,
    .selections = quixo_variant_option_selections,
};

// ============================== kQuixoSolverApi ==============================

static Tier QuixoGetInitialTier(void) { return initial_tier.hash; }

static Position QuixoGetInitialPosition(void) { return TwoPieceHashHash(0, 0); }

static int64_t QuixoGetTierSize(Tier tier) {
    QuixoTier t = {.hash = tier};

    return TwoPieceHashGetNumPositions(t.unpacked[0], t.unpacked[1]);
}

static MoveArray GenerateMovesInternal(uint64_t board, int turn) {
    MoveArray ret;
    MoveArrayInit(&ret);
    QuixoMove m = kQuixoMoveInit;
    int opp_turn = !turn;
    for (int i = 0; i < kNumMovesPerDir[curr_variant_idx]; ++i) {
        m.unpacked.idx = i;
        uint64_t src_mask = kMoveLeft[curr_variant_idx][i][opp_turn];
        if (!(board & src_mask)) {
            m.unpacked.dir = kLeft;
            MoveArrayAppend(&ret, m.hash);
        }
        src_mask = kMoveRight[curr_variant_idx][i][opp_turn];
        if (!(board & src_mask)) {
            m.unpacked.dir = kRight;
            MoveArrayAppend(&ret, m.hash);
        }
        src_mask = kMoveUp[curr_variant_idx][i][opp_turn];
        if (!(board & src_mask)) {
            m.unpacked.dir = kUp;
            MoveArrayAppend(&ret, m.hash);
        }
        src_mask = kMoveDown[curr_variant_idx][i][opp_turn];
        if (!(board & src_mask)) {
            m.unpacked.dir = kDown;
            MoveArrayAppend(&ret, m.hash);
        }
    }

    return ret;
}

static MoveArray QuixoGenerateMoves(TierPosition tier_position) {
    // Unhash
    QuixoTier t = {.hash = tier_position.tier};
    uint64_t board = TwoPieceHashUnhash(tier_position.position, t.unpacked[0],
                                        t.unpacked[1]);
    int turn = TwoPieceHashGetTurn(tier_position.position);

    return GenerateMovesInternal(board, turn);
}

static Value QuixoPrimitive(TierPosition tier_position) {
    // Unhash
    QuixoTier t = {.hash = tier_position.tier};
    uint64_t board = TwoPieceHashUnhash(tier_position.position, t.unpacked[0],
                                        t.unpacked[1]);
    int turn = TwoPieceHashGetTurn(tier_position.position);

    for (int i = 0; i < kNumLines[curr_variant_idx]; ++i) {
        // The current player wins if there is a k in a row of the current
        // player's pieces, regardless of whether there is a k in a row of the
        // opponent's pieces.
        uint64_t line = kLines[curr_variant_idx][turn][i];
        if ((board & line) == line) return kWin;
    }

    for (int i = 0; i < kNumLines[curr_variant_idx]; ++i) {
        // If the current player is not winning but there's a k in a row of the
        // opponent's pieces, then the current player loses.
        uint64_t line = kLines[curr_variant_idx][!turn][i];
        if ((board & line) == line) return kLose;
    }

    // Neither side is winning.
    return kUndecided;
}

static TierPosition DoMoveInternal(QuixoTier t, uint64_t board, int turn,
                                   QuixoMove m) {
    bool flipped = false;
    uint64_t A, B, C;
    switch (m.unpacked.dir) {
        case kLeft:
            A = kMoveLeft[curr_variant_idx][m.unpacked.idx][turn];
            flipped = !(board & A);
            B = kMoveLeft[curr_variant_idx][m.unpacked.idx][3];
            C = kMoveLeft[curr_variant_idx][m.unpacked.idx][2] >> (turn * 32);
            board = (((board & B) << 1) & B) | (board & ~B) | C;
            break;

        case kRight:
            A = kMoveRight[curr_variant_idx][m.unpacked.idx][turn];
            flipped = !(board & A);
            B = kMoveRight[curr_variant_idx][m.unpacked.idx][3];
            C = kMoveRight[curr_variant_idx][m.unpacked.idx][2] >> (turn * 32);
            board = (((board & B) >> 1) & B) | (board & ~B) | C;
            break;

        case kUp:
            A = kMoveUp[curr_variant_idx][m.unpacked.idx][turn];
            flipped = !(board & A);
            B = kMoveUp[curr_variant_idx][m.unpacked.idx][3];
            C = kMoveUp[curr_variant_idx][m.unpacked.idx][2] >> (turn * 32);
            board = (((board & B) << side_length) & B) | (board & ~B) | C;
            break;

        case kDown:
            A = kMoveDown[curr_variant_idx][m.unpacked.idx][turn];
            flipped = !(board & A);
            B = kMoveDown[curr_variant_idx][m.unpacked.idx][3];
            C = kMoveDown[curr_variant_idx][m.unpacked.idx][2] >> (turn * 32);
            board = (((board & B) >> side_length) & B) | (board & ~B) | C;
            break;
    }

    // Adjust tier if source tile is flipped
    t.unpacked[turn] += flipped;

    return (TierPosition){.tier = t.hash,
                          .position = TwoPieceHashHash(board, !turn)};
}

static TierPosition QuixoDoMove(TierPosition tier_position, Move move) {
    QuixoTier t = {.hash = tier_position.tier};
    uint64_t board = TwoPieceHashUnhash(tier_position.position, t.unpacked[0],
                                        t.unpacked[1]);
    int turn = TwoPieceHashGetTurn(tier_position.position);
    QuixoMove m = {.hash = move};

    return DoMoveInternal(t, board, turn, m);
}

// Performs a weak test on the position's legality. Will not misidentify legal
// as illegal, but might misidentify illegal as legal.
// In X's turn, returns illegal if there are no border Os, and vice versa
static bool QuixoIsLegalPosition(TierPosition tier_position) {
    // The initial position is always legal but does not follow the rules
    // below.
    if (tier_position.tier == initial_tier.hash &&
        tier_position.position == 0) {
        return true;
    }

    // Unhash
    QuixoTier t = {.hash = tier_position.tier};
    uint64_t board = TwoPieceHashUnhash(tier_position.position, t.unpacked[0],
                                        t.unpacked[1]);
    int turn = TwoPieceHashGetTurn(tier_position.position);

    return board & kEdges[curr_variant_idx][!turn];
}

static TierPositionArray QuixoGetCanonicalChildPositions(
    TierPosition tier_position) {
    // Unhash
    QuixoTier t = {.hash = tier_position.tier};
    uint64_t board = TwoPieceHashUnhash(tier_position.position, t.unpacked[0],
                                        t.unpacked[1]);
    int turn = TwoPieceHashGetTurn(tier_position.position);

    // Generate all moves
    MoveArray moves = GenerateMovesInternal(board, turn);

    // Collect all unique child positions
    TierPositionArray ret;
    TierPositionArrayInit(&ret);
    TierPositionHashSet dedup;
    TierPositionHashSetInit(&dedup, 0.5);
    for (int64_t i = 0; i < moves.size; ++i) {
        QuixoMove m = {.hash = moves.array[i]};
        TierPosition child = DoMoveInternal(t, board, turn, m);
        if (TierPositionHashSetContains(&dedup, child)) continue;
        TierPositionHashSetAdd(&dedup, child);
        TierPositionArrayAppend(&ret, child);
    }

    MoveArrayDestroy(&moves);
    TierPositionHashSetDestroy(&dedup);

    return ret;
}

static bool IsCorrectFlipping(QuixoTier child, QuixoTier parent,
                              int child_turn) {
    return (child.hash == parent.hash) ||
           !((child.unpacked[0] == parent.unpacked[0] + 1) ^ child_turn);
}

static PositionArray QuixoGetCanonicalParentPositions(
    TierPosition tier_position, Tier parent_tier) {
    // Unhash
    QuixoTier child_t = {.hash = tier_position.tier};
    QuixoTier parent_t = {.hash = parent_tier};
    PositionArray parents;
    PositionArrayInit(&parents);
    int turn = TwoPieceHashGetTurn(tier_position.position);
    if (!IsCorrectFlipping(child_t, parent_t, turn)) return parents;
    uint64_t board = TwoPieceHashUnhash(
        tier_position.position, child_t.unpacked[0], child_t.unpacked[1]);
    int opp_turn = !turn;
    uint64_t B, C;
    bool same_tier = (child_t.hash == parent_t.hash);
    PositionHashSet dedup;
    PositionHashSetInitSize(&dedup, 0.5, 127);
    for (int i = 0; i < kNumMovesPerDir[curr_variant_idx]; ++i) {
        // Revert a left shifting move
        uint64_t dest_mask =
            kMoveLeft[curr_variant_idx][i][2] >> (opp_turn * 32);
        if (board & dest_mask) {
            B = kMoveLeft[curr_variant_idx][i][3];
            C = kMoveLeft[curr_variant_idx][i][opp_turn] * same_tier;
            uint64_t new_board = (((board & B) >> 1) & B) | (board & ~B) | C;
            Position new_pos = TwoPieceHashHash(new_board, opp_turn);
            if (!PositionHashSetContains(&dedup, new_pos)) {
                PositionHashSetAdd(&dedup, new_pos);
                PositionArrayAppend(&parents, new_pos);
            }
        }

        // Revert a right shifting move
        dest_mask = kMoveRight[curr_variant_idx][i][2] >> (opp_turn * 32);
        if (board & dest_mask) {
            B = kMoveRight[curr_variant_idx][i][3];
            C = kMoveRight[curr_variant_idx][i][opp_turn] * same_tier;
            uint64_t new_board = (((board & B) << 1) & B) | (board & ~B) | C;
            Position new_pos = TwoPieceHashHash(new_board, opp_turn);
            if (!PositionHashSetContains(&dedup, new_pos)) {
                PositionHashSetAdd(&dedup, new_pos);
                PositionArrayAppend(&parents, new_pos);
            }
        }

        // Revert a upward shifting move
        dest_mask = kMoveUp[curr_variant_idx][i][2] >> (opp_turn * 32);
        if (board & dest_mask) {
            B = kMoveUp[curr_variant_idx][i][3];
            C = kMoveUp[curr_variant_idx][i][opp_turn] * same_tier;
            uint64_t new_board =
                (((board & B) >> side_length) & B) | (board & ~B) | C;
            Position new_pos = TwoPieceHashHash(new_board, opp_turn);
            if (!PositionHashSetContains(&dedup, new_pos)) {
                PositionHashSetAdd(&dedup, new_pos);
                PositionArrayAppend(&parents, new_pos);
            }
        }

        // Revert a downward shifting move
        dest_mask = kMoveDown[curr_variant_idx][i][2] >> (opp_turn * 32);
        if (board & dest_mask) {
            B = kMoveDown[curr_variant_idx][i][3];
            C = kMoveDown[curr_variant_idx][i][opp_turn] * same_tier;
            uint64_t new_board =
                (((board & B) << side_length) & B) | (board & ~B) | C;
            Position new_pos = TwoPieceHashHash(new_board, opp_turn);
            if (!PositionHashSetContains(&dedup, new_pos)) {
                PositionHashSetAdd(&dedup, new_pos);
                PositionArrayAppend(&parents, new_pos);
            }
        }
    }
    PositionHashSetDestroy(&dedup);

    return parents;
}

static int8_t GetNumBlanks(QuixoTier t) {
    return board_size - t.unpacked[0] - t.unpacked[1];
}

static TierArray QuixoGetChildTiers(Tier tier) {
    QuixoTier t = {.hash = tier};
    TierArray ret;
    TierArrayInit(&ret);
    if (GetNumBlanks(t) > 0) {
        ++t.unpacked[0];
        TierArrayAppend(&ret, t.hash);
        --t.unpacked[0];
        ++t.unpacked[1];
        TierArrayAppend(&ret, t.hash);
    }  // else: no children for tiers with no blanks

    return ret;
}

static int QuixoGetTierName(Tier tier,
                            char name[static kDbFileNameLengthMax + 1]) {
    QuixoTier t = {.hash = tier};
    sprintf(name, "%dBlank_%dX_%dO", GetNumBlanks(t), t.unpacked[0],
            t.unpacked[1]);

    return kNoError;
}

static const TierSolverApi kQuixoSolverApi = {
    .GetInitialTier = QuixoGetInitialTier,
    .GetInitialPosition = QuixoGetInitialPosition,
    .GetTierSize = QuixoGetTierSize,

    .GenerateMoves = QuixoGenerateMoves,
    .Primitive = QuixoPrimitive,
    .DoMove = QuixoDoMove,
    .IsLegalPosition = QuixoIsLegalPosition,
    .GetCanonicalPosition = NULL,
    .GetCanonicalChildPositions = QuixoGetCanonicalChildPositions,
    .GetCanonicalParentPositions = QuixoGetCanonicalParentPositions,

    .GetChildTiers = QuixoGetChildTiers,
    .GetTierName = QuixoGetTierName,
};

// ============================= kQuixoGameplayApi =============================

static void BoardToStr(uint64_t board, char *buffer) {
    for (int i = 0; i < board_size; ++i) {
        uint64_t x_mask = (1ULL << (32 + board_size - i - 1));
        uint64_t o_mask = (1ULL << (board_size - i - 1));
        bool is_x = board & x_mask;
        bool is_o = board & o_mask;
        buffer[i] = is_x ? 'X' : is_o ? 'O' : '-';
    }

    buffer[board_size] = '\0';
}

static int QuixoTierPositionToString(TierPosition tier_position, char *buffer) {
    // Unhash
    QuixoTier t = {.hash = tier_position.tier};
    uint64_t board = TwoPieceHashUnhash(tier_position.position, t.unpacked[0],
                                        t.unpacked[1]);
    char board_str[kBoardSizeMax + 1];
    BoardToStr(board, board_str);
    int offset = 0;
    for (int r = 0; r < side_length; ++r) {
        if (r == (side_length - 1) / 2) {
            offset += sprintf(buffer + offset, "LEGEND: ");
        } else {
            offset += sprintf(buffer + offset, "        ");
        }

        for (int c = 0; c < side_length; ++c) {
            int index = r * side_length + c + 1;
            if (c == 0) {
                offset += sprintf(buffer + offset, "(%2d", index);
            } else {
                offset += sprintf(buffer + offset, " %2d", index);
            }
        }
        offset += sprintf(buffer + offset, ")");

        if (r == (side_length - 1) / 2) {
            offset += sprintf(buffer + offset, "    BOARD: : ");
        } else {
            offset += sprintf(buffer + offset, "           : ");
        }

        for (int c = 0; c < side_length; ++c) {
            int index = r * side_length + c;
            offset += sprintf(buffer + offset, "%c ", board_str[index]);
        }
        offset += sprintf(buffer + offset, "\n");
    }

    return kNoError;
}

static int QuixoMoveToString(Move move, char *buffer) {
    QuixoMove m = {.hash = move};
    sprintf(
        buffer, "%d %c",
        kDirIndexToSrc[curr_variant_idx][m.unpacked.dir][m.unpacked.idx] + 1,
        kDirToChar[m.unpacked.dir]);

    return kNoError;
}

static bool QuixoIsValidMoveString(ReadOnlyString move_string) {
    // Strings of format "source direction". Ex: "6 R", "3 D".
    // Only "1" - "<board_size>" are valid sources.
    if (move_string == NULL) return false;
    if (strlen(move_string) < 3 || strlen(move_string) > 4) return false;

    // Make a copy of move_string bc strtok_r mutates the original move_string
    char copy_string[6];
    strcpy(copy_string, move_string);

    char *saveptr;
    char *token = strtok_r(copy_string, " ", &saveptr);
    if (token == NULL) return false;
    int src = atoi(token);

    token = strtok_r(NULL, " ", &saveptr);
    if (token == NULL) return false;

    if (src < 1) return false;
    if (src > board_size) return false;
    if (strlen(token) != 1) return false;
    if (token[0] != 'L' && token[0] != 'R' && token[0] != 'U' &&
        token[0] != 'D') {
        return false;
    }

    return true;
}

static Move QuixoStringToMove(ReadOnlyString move_string) {
    char copy_string[6];
    strcpy(copy_string, move_string);
    QuixoMove m = kQuixoMoveInit;
    char *saveptr;
    char *token = strtok_r(copy_string, " ", &saveptr);
    int src = atoi(token) - 1;

    token = strtok_r(NULL, " ", &saveptr);
    if (token[0] == 'R') {
        m.unpacked.dir = kLeft;
    } else if (token[0] == 'L') {
        m.unpacked.dir = kRight;
    } else if (token[0] == 'D') {
        m.unpacked.dir = kUp;
    } else {
        m.unpacked.dir = kDown;
    }
    m.unpacked.idx = kDirSrcToIndex[curr_variant_idx][m.unpacked.dir][src];

    return m.hash;
}

static const GameplayApiCommon QuixoGameplayApiCommon = {
    .GetInitialPosition = QuixoGetInitialPosition,
    .position_string_length_max = 512,

    .move_string_length_max = 4,
    .MoveToString = QuixoMoveToString,

    .IsValidMoveString = QuixoIsValidMoveString,
    .StringToMove = QuixoStringToMove,
};

static const GameplayApiTier QuixoGameplayApiTier = {
    .GetInitialTier = QuixoGetInitialTier,

    .TierPositionToString = QuixoTierPositionToString,

    .GenerateMoves = QuixoGenerateMoves,
    .DoMove = QuixoDoMove,
    .Primitive = QuixoPrimitive,
};

static const GameplayApi kQuixoGameplayApi = {
    .common = &QuixoGameplayApiCommon,
    .tier = &QuixoGameplayApiTier,
};

// ========================== QuixoGetCurrentVariant ==========================

static const GameVariant *QuixoGetCurrentVariant(void) {
    return &current_variant;
}

// =========================== QuixoSetVariantOption ===========================

static int QuixoInitVariant(int selection) {
    side_length = 5 - selection;
    board_size = side_length * side_length;
    int ret = TwoPieceHashInit(board_size);
    if (ret != kNoError) return ret;

    return kNoError;
}

static int QuixoSetVariantOption(int option, int selection) {
    // There is only one option in the game, and the selection must be between 0
    // to num_choices - 1 inclusive.
    if (option != 0 || selection < 0 || selection >= kQuixoRules.num_choices) {
        return kRuntimeError;
    }

    curr_variant_idx = quixo_variant_option_selections[0] = selection;

    return QuixoInitVariant(selection);
}

// ================================= QuixoInit =================================

static int QuixoInit(void *aux) {
    (void)aux;  // Unused.
    quixo_variant_options[0] = kQuixoRules;

    return QuixoSetVariantOption(0, 0);  // Initialize the default variant.
}

// =============================== QuixoFinalize ===============================

static int QuixoFinalize(void) {
    TwoPieceHashFinalize();

    return kNoError;
}

// ================================== kQuixo ==================================

const Game kQuixo = {
    .name = "quixo",
    .formal_name = "Quixo",
    .solver = &kTierSolver,
    .solver_api = &kQuixoSolverApi,
    .gameplay_api = &kQuixoGameplayApi,
    .uwapi = NULL,  // TODO

    .Init = QuixoInit,
    .Finalize = QuixoFinalize,

    .GetCurrentVariant = QuixoGetCurrentVariant,
    .SetVariantOption = QuixoSetVariantOption,
};
