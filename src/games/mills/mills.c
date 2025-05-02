/**
 * @file mills.c
 * @author Patricia Fong, Kevin Liu, Erwin A. Vedar, Wei Tu, Elmer Lee,
 * Cameron Cheung: developed the first version in GamesmanClassic (m369mm.c).
 * @author Cameron Cheung (cameroncheung@berkeley.edu): prototype version
 * @author Robert Shi (robertyishi@berkeley.edu): x86 SIMD hash version
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Implementation of the Mills Games (Morris Family of Games).
 * @version 1.0.0
 * @date 2025-04-27
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

#include "games/mills/mills.h"

#include <assert.h>     // assert
#include <stddef.h>     // NULL
#include <stdint.h>     // int64_t, int8_t, uint64_t
#include <stdio.h>      // sprintf
#include <stdlib.h>     // atoi
#include <string.h>     // strtok_r, strlen, strcpy
#include <x86intrin.h>  // __m128i

#include "core/constants.h"
#include "core/hash/x86_simd_two_piece.h"
#include "core/solvers/tier_solver/tier_solver.h"
#include "core/types/gamesman_types.h"

// =================================== Types ===================================

/**
 * @brief Mills tier definition:
 *     1. # remaining white pieces to be placed
 *     2. # remaining black pieces to be placed
 *     3. # white pieces on the board
 *     4. # black pieces on the board
 */
typedef union {
    struct {
        int8_t remaining[2];
        int8_t on_board[2];
    } unpacked;
    Tier hash;
} MillsTier;

typedef union {
    struct {
        /**
         * @brief Masked move source index in 8x8 grid, or kFromRemaining if
         * placing a piece.
         */
        int8_t src;

        /**
         * @brief Masked move dest index in 8x8 grid.
         */
        int8_t dest;

        /**
         * @brief Masked removal index in 8x8 grid, or kNoRemoval if no removal.
         */
        int8_t remove;
    } unpacked;
    Move hash;
} MillsMove;

enum {
    /**
     * @brief Special value for MillsMove source index indicating placement
     * moves.
     */
    kFromRemaining = 63,

    /**
     * @brief Special value for MillsMoves removal index indicating no removal.
     *
     */
    kNoRemoval = 63,
};

// ============================= Variant Settings =============================

/*
Option 1: board and pieces
    a) 5 pieces each on a 16-slot board (5mm)
    b) 6 pieces each on a 16-slot board (6mm)
    c) 7 pieces each on a 17-slot board (7mm)
    d) 9 pieces each on a 24-slot board (9mm)
    e) 10 pieces each on a 24-slot board (10mm)
    f) 11 pieces each on a 24-slot board with diagonals (11mm)
    g) 12 pieces each on a 24-slot board with diagonals (Morabaraba/12mm)
    h) 12 pieces each on a 25-slot Sesotho board (Sesotho Morabaraba)

Option 2: flying rule
    a) not allowed
    b) allowed when left with <= 3 pieces
    c) allowed at all times

Option 3: Lasker rule (merge placement and moving phases)
    a) false
    b) true

Option 4: removal rule
    a) standard: pieces in a mill can only be removed when all pieces are in a
       mill
    b) strict: if all pieces are in a mill, no piece is removed
    c) lenient: anypiece may be removed

Option 5: Misère (flip winning and losing conditions)
    a) false
    b) true

Standard combinations of options:
    (a) Five Men's Morris: [aaaaa]
    (b) Six Men's Morris: [baaaa]
    (c) Seven Men's Morris: [caaaa]
    (d) Nine Men's Morris: [dbaaa], [dbaba]
    (e) Lasker Morris: [ebbaa], [ebbba]
    (f) Eleven Men's Morris: [fbaaa]
    (g) Twelve Men's Morris/Morabaraba: [gbaaa]
    (h) Sesotho Morabaraba: [hbaaa]
*/

static ConstantReadOnlyString kMillsBoardAndPiecesChoices[] = {
    "5 pieces each on a 16-slot board (Five Men's Morris)",
    "6 pieces each on a 16-slot board (Six Men's Morris)",
    "7 pieces each on a 17-slot board (Seven Men's Morris)",
    "9 pieces each on a 24-slot board (Nine Men's Morris)",
    "10 pieces each on a 24-slot board (Ten Men's Morris)",
    "11 pieces each on a 24-slot board with diagonals (Eleven Men's Morris)",
    "12 pieces each on a 24-slot board with diagonals (Morabaraba/Twelve Men's "
    "Morris)",
    "12 pieces each on a 25-slot Sesotho board (Sesotho Morabaraba)",
};
#define NUM_BOARD_AND_PIECES_CHOICES       \
    (sizeof(kMillsBoardAndPiecesChoices) / \
     sizeof(kMillsBoardAndPiecesChoices[0]))

static ConstantReadOnlyString kMillsFlyingRuleChoices[] = {
    "Not allowed",
    "Allowed with fewer than 3 pieces on board",
    "Allowed always",
};

static ConstantReadOnlyString kBooleanChoices[] = {
    "False",
    "True",
};

static ConstantReadOnlyString kMillsRemovalRuleChoices[] = {
    "Standard",
    "Strict",
    "Lenient",
};

static const GameVariantOption mills_variant_options[6] = {
    {
        .name = "Board and Pieces",
        .num_choices = NUM_BOARD_AND_PIECES_CHOICES,
        .choices = kMillsBoardAndPiecesChoices,
    },
    {
        .name = "Flying Rule",
        .num_choices = sizeof(kMillsFlyingRuleChoices) /
                       sizeof(kMillsFlyingRuleChoices[0]),
        .choices = kMillsFlyingRuleChoices,
    },
    {
        .name = "Lasker Rule",
        .num_choices = 2,
        .choices = kBooleanChoices,
    },
    {
        .name = "Removal Rule",
        .num_choices = sizeof(kMillsRemovalRuleChoices) /
                       sizeof(kMillsRemovalRuleChoices[0]),
        .choices = kMillsRemovalRuleChoices,
    },
    {
        .name = "Misere",
        .num_choices = 2,
        .choices = kBooleanChoices,
    },
};

// Defaults to standard Nine Men's Morris rules.
union {
    int array[6];
    struct {
        int board_and_pieces;
        int flying_rule;
        int lasker_rule;
        int removal_rule;
        int misere;
        int zero_terminator;
    } unpacked;
} variant_option_selections = {
    .unpacked =
        {
            .board_and_pieces = 3,
            .flying_rule = 1,
            .lasker_rule = 0,
            .removal_rule = 0,
            .misere = 0,
            .zero_terminator = 0,
        },
};

static GameVariant current_variant = {
    .options = mills_variant_options,
    .selections = variant_option_selections.array,
};

// ================================= Constants =================================

static const int kPiecesPerPlayer[NUM_BOARD_AND_PIECES_CHOICES] = {
    5, 6, 7, 9, 10, 11, 12, 12,
};

static const MillsMove kMillsMoveInit = {.hash = 0};

static const uint64_t kBoardMasks[NUM_BOARD_AND_PIECES_CHOICES] = {
    0b0000000000000000000000000001010100001110000110110000111000010101,
    0b0000000000000000000000000001010100001110000110110000111000010101,
    0b0000000000000000000000000001010100001110000111110000111000010101,
    0b0000000001001001001010100001110001110111000111000010101001001001,
    0b0000000001001001001010100001110001110111000111000010101001001001,
    0b0000000001001001001010100001110001110111000111000010101001001001,
    0b0000000001001001001010100001110001110111000111000010101001001001,
    0b0000000001001001001010100001110001111111000111000010101001001001,
};

static const uint64_t kDestMasks[NUM_BOARD_AND_PIECES_CHOICES][56];

static const int kNumLines[NUM_BOARD_AND_PIECES_CHOICES] = {
    8, 8, 14, 16, 16, 20, 20, 22,
};

static const uint64_t kLines[NUM_BOARD_AND_PIECES_CHOICES][22];

static const int kNumParticipatingLines[NUM_BOARD_AND_PIECES_CHOICES][56];

static const uint64_t kParticipatingLines[NUM_BOARD_AND_PIECES_CHOICES][56][6];

static const uint64_t kInnerRingMasks[NUM_BOARD_AND_PIECES_CHOICES] = {
    0xe0a0e00,    0xe0a0e00,    0x0,          0x1c141c0000,
    0x1c141c0000, 0x1c141c0000, 0x1c141c0000, 0x0,
};

static const uint64_t kOuterRingMasks[NUM_BOARD_AND_PIECES_CHOICES] = {
    0x1500110015,     0x1500110015,     0x0, 0x49000041000049, 0x49000041000049,
    0x49000041000049, 0x49000041000049, 0x0,
};

static inline uint64_t SwapBits(uint64_t x, uint64_t mask1, uint64_t mask2) {
    uint64_t toggles = _pext_u64(x, mask1) ^ _pext_u64(x, mask2);

    return x ^ _pdep_u64(toggles, mask1) ^ _pdep_u64(toggles, mask2);
}

static inline __m128i SwapInnerOuterRings(__m128i board) {
    __attribute__((aligned(16))) uint64_t patterns[2];
    _mm_store_si128((__m128i *)patterns, board);
    int board_variant = variant_option_selections.unpacked.board_and_pieces;
    patterns[0] = SwapBits(patterns[0], kInnerRingMasks[board_variant],
                           kOuterRingMasks[board_variant]);
    patterns[1] = SwapBits(patterns[1], kInnerRingMasks[board_variant],
                           kOuterRingMasks[board_variant]);

    return _mm_load_si128((const __m128i *)patterns);
}

// ============================== kMillsSolverApi ==============================

static Tier MillsGetInitialTier(void) {
    int pieces_per_player =
        kPiecesPerPlayer[variant_option_selections.unpacked.board_and_pieces];
    MillsTier t = {
        .unpacked =
            {
                .remaining = {pieces_per_player, pieces_per_player},
                .on_board = {0, 0},
            },
    };

    return t.hash;
}

static Position MillsGetInitialPosition(void) {
    // The initial board is always empty, which by definition is the bit board
    // that is filled with all zeros.
    __m128i board = _mm_set1_epi64x(0);

    return X86SimdTwoPieceHashHashFixedTurn(board);
}

static inline int BoardId(void) {
    return variant_option_selections.unpacked.board_and_pieces;
}

static inline int Lasker(void) {
    return variant_option_selections.unpacked.lasker_rule;
}

static inline int FlyThreshold(void) {
    static const int ret[3] = {0, 3, 64};

    return ret[variant_option_selections.unpacked.flying_rule];
}

static inline bool IsTierFixedTurn(MillsTier t) {
    if (!Lasker()) {  // Not using Lasker rule
        // The turn of all positions in all placement phase tiers can be deduced
        // from the number of each player's remaining pieces.
        return t.unpacked.remaining[0] + t.unpacked.remaining[1] > 0;
    }

    // Reach here if using Lasker rule.
    // If only one of the players has no pieces on the board, then it must
    // be that player's turn. As long as one of the players has pieces on
    // the board, it is possible for either player to move a piece without
    // capturing and pass the turn to the opponent.
    return t.unpacked.on_board[0] == 0 || t.unpacked.on_board[1] == 0;
}

static int64_t MillsGetTierSize(Tier tier) {
    MillsTier t = {.hash = tier};
    int num_x = t.unpacked.on_board[0];
    int num_o = t.unpacked.on_board[1];
    if (IsTierFixedTurn(t)) {
        return X86SimdTwoPieceHashGetNumPositionsFixedTurn(num_x, num_o);
    }

    return X86SimdTwoPieceHashGetNumPositions(num_x, num_o);
}

static bool IsPlacementTier(MillsTier t) { return t.unpacked.remaining[1]; }

static int GetTurnFromPlacementTier(MillsTier t) {
    // Assuming both players start with the same number of pieces.
    return t.unpacked.remaining[0] != t.unpacked.remaining[1];
}

// Returns -1 if it can be either player's turn.
static int GetTurnFromLaskerTier(MillsTier t) {
    if (t.unpacked.on_board[0] == 0) return 0;
    if (t.unpacked.on_board[1] == 0) return 1;

    return -1;
}

static int GetTurnFromTierGeneric(MillsTier t) {
    if (!Lasker()) {
        return GetTurnFromPlacementTier(t);
    }

    return GetTurnFromLaskerTier(t);
}

static MillsTier Unhash(TierPosition tp, uint64_t patterns[2], int *turn) {
    MillsTier t = {.hash = tp.tier};
    int num_x = t.unpacked.on_board[0], num_o = t.unpacked.on_board[1];
    if (IsTierFixedTurn(t)) {
        *turn = GetTurnFromTierGeneric(t);
        X86SimdTwoPieceHashUnhashFixedTurnMem(tp.position, num_x, num_o,
                                              patterns);
    } else {
        *turn = X86SimdTwoPieceHashGetTurn(tp.position);
        X86SimdTwoPieceHashUnhashMem(tp.position, num_x, num_o, patterns);
    }

    return t;
}

/**
 * @brief Returns true if \p m closes a mill for the given bit board \p pattern
 * , or false otherwise.
 */
static bool ClosesMill(uint64_t pattern, MillsMove m) {
    // The most significant bit might be set by a placing move but it doesn't
    // affect the result of this function.
    pattern ^= ((1ULL << m.unpacked.src) | (1ULL << m.unpacked.dest));
    int bid = BoardId();
    for (int i = 0; i < kNumParticipatingLines[bid][m.unpacked.dest]; ++i) {
        uint64_t line = kParticipatingLines[bid][m.unpacked.dest][i];
        if (line == line & pattern) return true;
    }

    return false;
}

static void GeneratePlacingMoves(uint64_t patterns[2], int turn,
                                 uint64_t legal_removes, uint64_t blanks,
                                 Move moves[static kTierSolverNumMovesMax],
                                 int *ret) {
    MillsMove m = kMillsMoveInit;
    m.unpacked.src = kFromRemaining;
    for (; blanks; blanks = _blsr_u64(blanks)) {
        m.unpacked.dest = (int8_t)_tzcnt_u64(blanks);
        if (legal_removes && ClosesMill(patterns[turn], m)) {
            for (uint64_t removes = legal_removes; removes;
                 removes = _blsr_u64(removes)) {
                m.unpacked.remove = (int8_t)_tzcnt_u64(removes);
                moves[(*ret)++] = m.hash;
            }
        } else {
            m.unpacked.remove = kNoRemoval;
            moves[(*ret)++] = m.hash;
        }
    }
}

/**
 * @brief Returns a 64-bit mask with all bits corresponding to valid
 * destinations set to 1. A valid destination must be blank and adjacent to src,
 * unless the current player is allowed to fly, in which case any blank space is
 * valid.
 */
static inline uint64_t BuildDestMask(MillsTier t, int turn, int8_t src,
                                     uint64_t blanks) {
    if (t.unpacked.on_board[turn] <= FlyThreshold()) {
        return kDestMasks[BoardId()][src] & blanks;
    }

    return blanks;
}

static void GenerateSlidingMoves(MillsTier t, uint64_t patterns[2], int turn,
                                 uint64_t legal_removes, uint64_t blanks,
                                 Move moves[static kTierSolverNumMovesMax],
                                 int *ret) {
    MillsMove m = kMillsMoveInit;
    for (uint64_t pattern = patterns[turn]; pattern;
         pattern = _blsr_u64(pattern)) {
        m.unpacked.src = (int8_t)_tzcnt_u64(pattern);
        for (uint64_t dest_mask =
                 BuildDestMask(t, turn, m.unpacked.src, blanks);
             dest_mask; dest_mask = _blsr_u64(dest_mask)) {
            m.unpacked.dest = (int8_t)_tzcnt_u64(dest_mask);
            if (legal_removes && ClosesMill(patterns[turn], m)) {
                for (uint64_t removes = legal_removes; removes;
                     removes = _blsr_u64(removes)) {
                    m.unpacked.remove = (int8_t)_tzcnt_u64(removes);
                    moves[(*ret)++] = m.hash;
                }
            } else {
                m.unpacked.remove = kNoRemoval;
                moves[(*ret)++] = m.hash;
            }
        }
    }
}

/**
 * @brief Returns a mask with all set bits corresponding to valid removal
 * locations in \p pattern.
 */
static uint64_t BuildLegalRemovesMask(uint64_t pattern) {
    if (variant_option_selections.unpacked.removal_rule == 2) {
        return pattern;
    }

    uint64_t formed_mills = 0ULL;
    for (int i = 0; i < kNumLines[BoardId()]; ++i) {
        uint64_t line = kLines[BoardId()][i];
        bool formed = (line & pattern) == line;
        formed_mills |= BooleanMask(formed) & line;
    }
    uint64_t ret = pattern ^ formed_mills;

    if (variant_option_selections.unpacked.removal_rule == 1) {
        return ret;
    }

    return ret | (BooleanMask(ret == 0ULL) & pattern);
}

static inline uint64_t BuildBlanksMask(const uint64_t patterns[2]) {
    return ~(patterns[0] & patterns[1]);
}

static int GenerateMovesInternal(MillsTier t, uint64_t patterns[2], int turn,
                                 Move moves[static kTierSolverNumMovesMax]) {
    // Legal removal indices of opponent pieces as set bits
    uint64_t legal_removes = BuildLegalRemovesMask(patterns[!turn]);
    uint64_t blanks = BuildBlanksMask(patterns);  // All blank slots as set bits
    int ret = 0;

    // Placing moves are available whenever there are pieces to place.
    if (t.unpacked.remaining[turn]) {
        GeneratePlacingMoves(patterns, turn, legal_removes, blanks, moves,
                             &ret);
    }

    // Sliding/flying moves are available when all pieces have been placed
    // or if using Lasker rule.
    if (Lasker() || t.unpacked.remaining[turn] == 0) {
        GenerateSlidingMoves(t, patterns, turn, legal_removes, blanks, moves,
                             &ret);
    }

    return ret;
}

static int MillsGenerateMoves(TierPosition tier_position,
                              Move moves[static kTierSolverNumMovesMax]) {
    // Unhash
    uint64_t patterns[2];
    int turn;
    MillsTier t = Unhash(tier_position, patterns, &turn);

    return GenerateMovesInternal(t, patterns, turn, moves);
}

static Value MillsPrimitive(TierPosition tier_position) {
    MillsTier t = {.hash = tier_position.tier};

    // The current player loses if their remaining pieces has been reduced to 2.
    // It doesn't matter whose turn it is since it's not possible for players
    // to capture their own pieces.
    if (t.unpacked.remaining[0] + t.unpacked.on_board[0] == 2) return kLose;
    if (t.unpacked.remaining[1] + t.unpacked.on_board[1] == 2) return kLose;

    // The current player also loses if they have no moves to make.
    Move moves[kTierSolverNumMovesMax];
    int num_moves = MillsGenerateMoves(tier_position, moves);
    if (num_moves == 0) return kLose;

    return kUndecided;
}

/**
 * @brief Returns 0 if \p b is true, or the value with all 64 bits set to 1 if
 * \p b is true.
 */
static inline uint64_t BooleanMask(bool b) { return -(uint64_t)b; }

static TierPosition MillsDoMoveInternal(MillsTier t, uint64_t patterns[2],
                                        MillsMove m, int turn) {
    // Create and apply toggle masks for each player's bit board.
    // First handle the current player's bit board by applying the move.
    bool placing = (m.unpacked.src == kFromRemaining);
    uint64_t toggle = (1ULL << m.unpacked.dest) |
                      (BooleanMask(!placing) & (1ULL << m.unpacked.src));
    patterns[turn] ^= toggle;
    t.unpacked.remaining[turn] -= placing;
    t.unpacked.on_board[turn] += placing;

    // Then handle the opponent's bit board by conditionally applying removal.
    bool removing = (m.unpacked.remove != kNoRemoval);
    toggle = (BooleanMask(removing) & (1ULL << m.unpacked.remove));
    patterns[!turn] ^= toggle;
    t.unpacked.on_board[!turn] -= removing;

    TierPosition ret = {.tier = t.hash};
    if (IsTierFixedTurn(t)) {
        ret.position = X86SimdTwoPieceHashHashFixedTurnMem(patterns);
    } else {
        ret.position = X86SimdTwoPieceHashHashMem(patterns, !turn);
    }

    return ret;
}

static TierPosition MillsDoMove(TierPosition tier_position, Move move) {
    // Unhash
    uint64_t patterns[2];
    int turn;
    MillsTier t = Unhash(tier_position, patterns, &turn);
    MillsMove m = {.hash = move};

    return MillsDoMoveInternal(t, patterns, m, turn);
}

static bool MillsIsLegalPosition(TierPosition tier_position) {
    // No simple way to test if a position is unreachable.
    (void)tier_position;  // Unused.
    return true;
}

static Position MillsGetCanonicalPosition(TierPosition tier_position) {
    // TODO
    return tier_position.position;
}

static int MillsGetNumberOfCanonicalChildPositions(TierPosition tier_position) {
    // TODO
}

static int MillsGetCanonicalChildPositions(
    TierPosition tier_position,
    TierPosition children[static kTierSolverNumChildPositionsMax]) {
    // TODO
}

static int MillsGetCanonicalParentPositions(
    TierPosition tier_position, Tier parent_tier,
    Position parents[static kTierSolverNumParentPositionsMax]) {
    // TODO
}

static Tier ConstructTier(int rem_x, int rem_o, int num_x, int num_o) {
    MillsTier t = {.hash = 0};
    t.unpacked.remaining[0] = rem_x;
    t.unpacked.remaining[1] = rem_o;
    t.unpacked.on_board[0] = num_x;
    t.unpacked.on_board[1] = num_o;

    return t.hash;
}

static int GetChildTiersInternal(
    MillsTier t, Tier children[static kTierSolverNumChildTiersMax]) {
    //
    int ret = 0;
    if (IsPlacementTier(t)) {  // Placement phase tier
        int turn = GetTurnFromPlacementTier(t);

        // It is always possible to place a piece without capturing.
        --t.unpacked.remaining[turn];
        ++t.unpacked.on_board[turn];
        children[ret++] = t.hash;

        // Capturing may happen if the current player has at least 3 pieces
        // on board after the placement.
        if (t.unpacked.on_board[turn] >= 3) {
            assert(t.unpacked.on_board[!turn] > 0);
            --t.unpacked.on_board[!turn];
            children[ret++] = t.hash;
        }

        return ret;
    }

    // Movement phase tier
    // Both players have at least 3 pieces on board and it might be
    // either player's turn. Capturing is possible for both players.
    for (int turn = 0; turn <= 1; ++turn) {
        --t.unpacked.on_board[turn];
        children[ret++] = t.hash;
        ++t.unpacked.on_board[turn];
    }

    return ret;
}

static int GetChildTiersLaskerInternal(
    MillsTier t, Tier children[static kTierSolverNumChildTiersMax]) {
    //
    int turn = GetTurnFromLaskerTier(t);
    if (turn >= 0) {  // Turn can be deduced
        // At least one of the players have no pieces on the board.
        // It must be that player's turn and they must make a placement.
        assert(t.unpacked.on_board[turn] == 0);
        --t.unpacked.remaining[turn];
        t.unpacked.on_board[turn] = 1;
        children[0] = t.hash;

        return 1;
    }

    // Reach here if it can be either player's turn.
    int ret = 0;
    for (int turn = 0; turn <= 1; ++turn) {
        // Move and capture is possible if the current player has at least 3
        // on-board pieces
        if (t.unpacked.on_board[turn] >= 3) {
            --t.unpacked.on_board[!turn];
            children[ret++] = t.hash;
            ++t.unpacked.on_board[!turn];  // Revert capture
        }

        // Placement is possible if the current player has at least 1 remaining
        // piece
        if (t.unpacked.remaining[turn]) {
            // Placement without capturing is always possible when placement is
            // possible
            --t.unpacked.remaining[turn];
            ++t.unpacked.on_board[turn];
            children[ret++] = t.hash;

            // Place-and-capture is possible only if the current player has 3 or
            // more pieces on the board after placing
            if (t.unpacked.on_board[turn] >= 3) {
                --t.unpacked.on_board[!turn];
                children[ret++] = t.hash;
                ++t.unpacked.on_board[!turn];  // Revert capture
            }

            // Revert placement
            ++t.unpacked.remaining[turn];
            --t.unpacked.on_board[turn];
        }
    }
}

static int MillsGetChildTiers(
    Tier tier, Tier children[static kTierSolverNumChildTiersMax]) {
    // Primitive tiers have no children.
    MillsTier t = {.hash = tier};
    int rem_x = t.unpacked.remaining[0], rem_o = t.unpacked.remaining[1];
    int num_x = t.unpacked.on_board[0], num_o = t.unpacked.on_board[1];
    if (rem_x + num_x == 2 || rem_o + num_o == 2) return 0;

    // Not using Lasker rule
    if (!Lasker()) return GetChildTiersInternal(t, children);

    // Using Lasker rule
    return GetChildTiersLaskerInternal(t, children);
}

TierType MillsGetTierType(Tier tier) {
    MillsTier t = {.hash = tier};
    if (IsTierFixedTurn(t)) return kTierTypeImmediateTransition;

    return kTierTypeLoopy;
}

static int MillsGetTierName(Tier tier,
                            char name[static kDbFileNameLengthMax + 1]) {
    MillsTier t = {.hash = tier};
    sprintf(name, "R%dX%dO_B%dX%dO", t.unpacked.remaining[0],
            t.unpacked.remaining[1], t.unpacked.on_board[0],
            t.unpacked.on_board[1]);

    return kNoError;
}

static const TierSolverApi kMillsSolverApi = {
    .GetInitialTier = MillsGetInitialTier,
    .GetInitialPosition = MillsGetInitialPosition,
    .GetTierSize = MillsGetTierSize,

    .GenerateMoves = MillsGenerateMoves,
    .Primitive = MillsPrimitive,
    .DoMove = MillsDoMove,
    .IsLegalPosition = MillsIsLegalPosition,
    .GetCanonicalPosition = MillsGetCanonicalPosition,
    .GetNumberOfCanonicalChildPositions =
        MillsGetNumberOfCanonicalChildPositions,
    .GetCanonicalChildPositions = MillsGetCanonicalChildPositions,
    .GetCanonicalParentPositions = MillsGetCanonicalParentPositions,

    .GetChildTiers = MillsGetChildTiers,
    .GetTierType = MillsGetTierType,
    .GetTierName = MillsGetTierName,
};

// ========================== MillsGetCurrentVariant ==========================

static const GameVariant *MillsGetCurrentVariant(void) {
    return &current_variant;
}

// =========================== MillsSetVariantOption ===========================

static int MillsSetVariantOption(int option, int selection) {
    if (selection < 0) return kIllegalArgumentError;
    switch (option) {
        case 0: {
            if (selection >= NUM_BOARD_AND_PIECES_CHOICES) {
                return kIllegalArgumentError;
            }
            int error =
                X86SimdTwoPieceHashInitIrregular(kBoardMasks[selection]);
            assert(error == kNoError);
            (void)error;
            break;
        }

        case 1:
        case 3:
            if (selection >= 3) return kIllegalArgumentError;
            break;

        case 2:
        case 4:
            if (selection >= 2) return kIllegalArgumentError;
            break;

        default:
            return kIllegalArgumentError;
    }
    variant_option_selections.array[option] = selection;

    return kNoError;
}

// ================================= MillsInit =================================

static int MillsInit(void *aux) {
    (void)aux;  // Unused.

    // Initialize the default variant.
    for (int i = 1; i < 6; ++i) {
        int ret = MillsSetVariantOption(i, 0);
        assert(ret == kNoError);
        (void)ret;
    }

    return MillsSetVariantOption(0, 0);
}

// =============================== MillsFinalize ===============================

static int MillsFinalize(void) {
    X86SimdTwoPieceHashFinalize();

    return kNoError;
}

// ================================== kMills ==================================

const Game kMills = {
    .name = "mills",
    .formal_name = "Mills",
    .solver = &kTierSolver,
    .solver_api = &kMillsSolverApi,
    .gameplay_api = NULL,  // TODO
    .uwapi = NULL,         // TODO

    .Init = MillsInit,
    .Finalize = MillsFinalize,

    .GetCurrentVariant = MillsGetCurrentVariant,
    .SetVariantOption = MillsSetVariantOption,
};
