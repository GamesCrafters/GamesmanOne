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
#include "games/mills/board_formats.h"
#include "games/mills/masks.h"
#include "games/mills/variants.h"

// ============================ Types and Constants ============================

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

static const MillsMove kMillsMoveInit = {.hash = 0};

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
            .flying_rule = 0,
            .lasker_rule = 0,
            .removal_rule = 0,
            .misere = 0,
            .zero_terminator = 0,
        },
};

static int BoardId(void) {
    return variant_option_selections.unpacked.board_and_pieces;
}

static int Lasker(void) {
    return variant_option_selections.unpacked.lasker_rule;
}

static int FlyThreshold(void) {
    static const int ret[3] = {3, 0, 64};

    return ret[variant_option_selections.unpacked.flying_rule];
}

static bool StrictRemoval(void) {
    return variant_option_selections.unpacked.removal_rule == 1;
}

static bool LenientRemoval(void) {
    return variant_option_selections.unpacked.removal_rule == 2;
}

static int8_t PaddedSideLength(void) { return kPaddedSideLengths[BoardId()]; }

static bool Misere(void) { return variant_option_selections.unpacked.misere; }

static GameVariant current_variant = {
    .options = mills_variant_options,
    .selections = variant_option_selections.array,
};

// ============================= Variant Constants =============================

static MillsTier second_lasker_tier;

static int kGridIdxToBoardIdx[NUM_BOARD_AND_PIECES_CHOICES][64];

static void BuildGridIdxToBoardIdx(void) {
    for (size_t i = 0; i < NUM_BOARD_AND_PIECES_CHOICES; ++i) {
        for (int j = 0; j < kNumSlots[i]; ++j) {
            kGridIdxToBoardIdx[i][kBoardIdxToGridIdx[i][j]] = j;
        }
    }
}

// ============================== kMillsSolverApi ==============================

static Tier MillsGetInitialTier(void) {
    int8_t pieces_per_player =
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
    // filled with all zeros.
    return X86SimdTwoPieceHashHashFixedTurn(_mm_setzero_si128());
}

static bool IsPlacementTier(MillsTier t) { return t.unpacked.remaining[1]; }

static int GetTurnFromPlacementTier(MillsTier t) {
    // Assuming both players start with the same number of pieces.
    return t.unpacked.remaining[0] != t.unpacked.remaining[1];
}

static int GetTurnFromLaskerTier(MillsTier t) {
    if (t.unpacked.on_board[0] == 0) return 0;
    if (t.unpacked.on_board[1] == 0) return 1;
    if (t.unpacked.remaining[0] + t.unpacked.on_board[0] == 2) return 0;
    if (t.unpacked.remaining[1] + t.unpacked.on_board[1] == 2) return 1;

    return -1;
}

static int GetTurnFromNonLaskerTier(MillsTier t) {
    if (t.unpacked.remaining[0] + t.unpacked.remaining[1] > 0) {
        // Assuming both players start with the same number of pieces.
        return t.unpacked.remaining[0] != t.unpacked.remaining[1];
    } else if (t.unpacked.on_board[0] == 2) {
        return 0;
    } else if (t.unpacked.on_board[1] == 2) {
        return 1;
    }

    return -1;
}

// Returns -1 if it can be either player's turn.
static int GetTurnFromTier(MillsTier t) {
    if (!Lasker()) {
        return GetTurnFromNonLaskerTier(t);
    }

    return GetTurnFromLaskerTier(t);
}

static int64_t MillsGetTierSize(Tier tier) {
    MillsTier t = {.hash = tier};
    int num_x = t.unpacked.on_board[0];
    int num_o = t.unpacked.on_board[1];
    if (GetTurnFromTier(t) >= 0) {
        return X86SimdTwoPieceHashGetNumPositionsFixedTurn(num_x, num_o);
    }

    return X86SimdTwoPieceHashGetNumPositions(num_x, num_o);
}

static MillsTier Unhash(TierPosition tp, uint64_t patterns[2], int *turn) {
    MillsTier t = {.hash = tp.tier};
    int num_x = t.unpacked.on_board[0], num_o = t.unpacked.on_board[1];
    if ((*turn = GetTurnFromTier(t)) >= 0) {
        X86SimdTwoPieceHashUnhashFixedTurnMem(tp.position, num_x, num_o,
                                              patterns);
    } else {
        *turn = X86SimdTwoPieceHashGetTurn(tp.position);
        X86SimdTwoPieceHashUnhashMem(tp.position, num_x, num_o, patterns);
    }

    return t;
}

static __m128i UnhashSimd(TierPosition tp, MillsTier *t, int *turn,
                          bool *not_fixed_turn) {
    t->hash = tp.tier;
    int num_x = t->unpacked.on_board[0], num_o = t->unpacked.on_board[1];
    if ((*turn = GetTurnFromTier(*t)) >= 0) {
        *not_fixed_turn = false;
        return X86SimdTwoPieceHashUnhashFixedTurn(tp.position, num_x, num_o);
    }

    *turn = X86SimdTwoPieceHashGetTurn(tp.position);
    *not_fixed_turn = true;
    return X86SimdTwoPieceHashUnhash(tp.position, num_x, num_o);
}

/**
 * @brief Returns true if \p m closes a mill for the given bit board \p pattern
 * , or false otherwise.
 */
static bool ClosesMill(uint64_t pattern, MillsMove m) {
    // The most significant bit might be set by a placing move but it doesn't
    // affect the result of this function.
    pattern ^= 1ULL << m.unpacked.src;
    int bid = BoardId();
    for (int8_t i = 0; i < kNumParticipatingLines[bid][m.unpacked.dest]; ++i) {
        uint64_t line = kParticipatingLines[bid][m.unpacked.dest][i];
        if (line == (line & pattern)) return true;
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
 * @brief Returns a 64-bit mask with all set bits corresponding to valid
 * destinations. A valid destination must be blank and adjacent to src, unless
 * the current player is allowed to fly, in which case any blank space is valid.
 */
static uint64_t BuildDestMask(MillsTier t, int turn, int8_t src,
                              uint64_t blanks) {
    if (t.unpacked.on_board[turn] <= FlyThreshold()) {
        return blanks;
    }

    return kDestMasks[BoardId()][src] & blanks;
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
 * @brief Returns 0 if \p b is true, or the value with all 64 bits set to 1 if
 * \p b is true.
 */
static uint64_t BooleanMask(bool b) { return -(uint64_t)b; }

/**
 * @brief Returns a mask with all set bits corresponding to locations that are
 * currently in mills in \p pattern.
 */
static uint64_t BuildInMillMask(uint64_t pattern) {
    uint64_t formed_mills = 0ULL;
    for (int8_t i = 0; i < kNumLines[BoardId()]; ++i) {
        uint64_t line = kLineMasks[BoardId()][i];
        bool formed = (line & pattern) == line;
        formed_mills |= BooleanMask(formed) & line;
    }

    return formed_mills;
}

/**
 * @brief Returns a mask with all set bits corresponding to locations that are
 * not in mills in \p pattern.
 */
static uint64_t BuildNotInMillMask(uint64_t pattern) {
    uint64_t formed_mills = BuildInMillMask(pattern);

    return pattern ^ formed_mills;
}

/**
 * @brief Returns a mask with all set bits corresponding to valid removal
 * locations in \p pattern.
 */
static uint64_t BuildLegalRemovalsMask(uint64_t pattern) {
    if (LenientRemoval()) {
        return pattern;
    }

    uint64_t ret = BuildNotInMillMask(pattern);
    if (StrictRemoval()) {
        return ret;
    }

    return ret | (BooleanMask(ret == 0ULL) & pattern);
}

static uint64_t BuildBlanksMask(const uint64_t patterns[2]) {
    return (~(patterns[0] | patterns[1])) & kBoardMasks[BoardId()];
}

static int GenerateMovesInternal(MillsTier t, uint64_t patterns[2], int turn,
                                 Move moves[static kTierSolverNumMovesMax]) {
    // Legal removal indices of opponent pieces as set bits
    uint64_t legal_removes = BuildLegalRemovalsMask(patterns[!turn]);
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
    bool misere = Misere();

    // The current player loses if their remaining pieces has been reduced to 2.
    // It doesn't matter whose turn it is since it's not possible for players
    // to capture their own pieces.
    if (t.unpacked.remaining[0] + t.unpacked.on_board[0] == 2) {
        return misere ? kWin : kLose;
    } else if (t.unpacked.remaining[1] + t.unpacked.on_board[1] == 2) {
        return misere ? kWin : kLose;
    }

    // The current player also loses if they have no moves to make.
    Move moves[kTierSolverNumMovesMax];
    int num_moves = MillsGenerateMoves(tier_position, moves);
    if (num_moves == 0) {
        return misere ? kWin : kLose;
    }

    return kUndecided;
}

static TierPosition MillsDoMoveInternal(MillsTier t,
                                        const uint64_t _patterns[2],
                                        MillsMove m, int turn) {
    // Create a copy of the patterns
    uint64_t patterns[2] = {_patterns[0], _patterns[1]};

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
    if (GetTurnFromTier(t) >= 0) {
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

static uint64_t SwapBits(uint64_t x, uint64_t mask1, uint64_t mask2) {
    uint64_t toggles = _pext_u64(x, mask1) ^ _pext_u64(x, mask2);

    return x ^ _pdep_u64(toggles, mask1) ^ _pdep_u64(toggles, mask2);
}

static __m128i SwapInnerOuterRings(__m128i board) {
    __attribute__((aligned(16))) uint64_t patterns[2];
    _mm_store_si128((__m128i *)patterns, board);
    int board_id = BoardId();
    patterns[0] = SwapBits(patterns[0], kInnerRingMasks[board_id],
                           kOuterRingMasks[board_id]);
    patterns[1] = SwapBits(patterns[1], kInnerRingMasks[board_id],
                           kOuterRingMasks[board_id]);

    return _mm_load_si128((const __m128i *)patterns);
}

static __m128i GetCanonicalBoardRotation(__m128i board) {
    __m128i canonical = board;
    int8_t padded_side_length = PaddedSideLength();

    // 8 symmetries
    board = X86SimdTwoPieceHashFlipVertical(board, padded_side_length);
    if (X86SimdTwoPieceHashBoardLessThan(board, canonical)) canonical = board;
    board = X86SimdTwoPieceHashFlipDiag(board);
    if (X86SimdTwoPieceHashBoardLessThan(board, canonical)) canonical = board;
    board = X86SimdTwoPieceHashFlipVertical(board, padded_side_length);
    if (X86SimdTwoPieceHashBoardLessThan(board, canonical)) canonical = board;
    board = X86SimdTwoPieceHashFlipDiag(board);
    if (X86SimdTwoPieceHashBoardLessThan(board, canonical)) canonical = board;
    board = X86SimdTwoPieceHashFlipVertical(board, padded_side_length);
    if (X86SimdTwoPieceHashBoardLessThan(board, canonical)) canonical = board;
    board = X86SimdTwoPieceHashFlipDiag(board);
    if (X86SimdTwoPieceHashBoardLessThan(board, canonical)) canonical = board;
    board = X86SimdTwoPieceHashFlipVertical(board, padded_side_length);
    if (X86SimdTwoPieceHashBoardLessThan(board, canonical)) canonical = board;

    return canonical;
}

static __m128i GetCanonicalBoardRotationRingSwap(__m128i board) {
    // Rotational symmetries are always present
    __m128i canonical = GetCanonicalBoardRotation(board);

    // Ring swap symmetries are present in certain board variants
    if (kInnerRingMasks[BoardId()]) {
        __m128i swapped = SwapInnerOuterRings(board);
        __m128i ring_swapped_canonical = GetCanonicalBoardRotation(swapped);
        if (X86SimdTwoPieceHashBoardLessThan(ring_swapped_canonical,
                                             canonical)) {
            canonical = ring_swapped_canonical;
        }
    }

    return canonical;
}

static Position MillsGetCanonicalPosition(TierPosition tier_position) {
    // Unhash
    MillsTier t;
    int turn;
    bool not_fixed_turn;
    __m128i board = UnhashSimd(tier_position, &t, &turn, &not_fixed_turn);
    __m128i canonical = GetCanonicalBoardRotationRingSwap(board);

    // If swapping colors results in a position within the same tier, apply that
    // symmetry as well.
    // if (t.unpacked.on_board[0] == t.unpacked.on_board[1] &&
    //     t.unpacked.remaining[0] == t.unpacked.remaining[1] && not_fixed_turn)
    //     {
    //     __m128i swapped = X86SimdTwoPieceHashSwapPieces(board);
    //     __m128i pieces_swapped_canonical =
    //         GetCanonicalBoardRotationRingSwap(swapped);
    //     if (X86SimdTwoPieceHashBoardLessThan(pieces_swapped_canonical,
    //                                          canonical)) {
    //         canonical = pieces_swapped_canonical;
    //         turn = !turn;
    //     }
    // }

    // Hash
    if (not_fixed_turn) return X86SimdTwoPieceHashHash(canonical, turn);
    return X86SimdTwoPieceHashHashFixedTurn(canonical);
}

// static TierPosition GetCanonicalTierPosition(TierPosition tp) {
//     TierPosition canonical;

//     // Convert to the tier position inside the canonical tier.
//     canonical.tier = MillsGetCanonicalTier(tp.tier);
//     canonical.position = MillsGetPositionInSymmetricTier(tp, canonical.tier);

//     // Find the canonical position inside the canonical tier.
//     canonical.position = MillsGetCanonicalPosition(canonical);

//     return canonical;
// }

static int MillsGetNumberOfCanonicalChildPositions(TierPosition tier_position) {
    // Unhash
    uint64_t patterns[2];
    int turn;
    MillsTier t = Unhash(tier_position, patterns, &turn);

    // Generate moves
    Move moves[kTierSolverNumMovesMax];
    int num_moves = GenerateMovesInternal(t, patterns, turn, moves);

    // Collect children
    TierPositionHashSet dedup;
    TierPositionHashSetInit(&dedup, 0.5);
    TierPositionHashSetReserve(&dedup, num_moves);
    for (int i = 0; i < num_moves; ++i) {
        MillsMove m = {.hash = moves[i]};
        TierPosition child = MillsDoMoveInternal(t, patterns, m, turn);
        child.position = MillsGetCanonicalPosition(child);
        // child = GetCanonicalTierPosition(child);
        if (!TierPositionHashSetContains(&dedup, child)) {
            TierPositionHashSetAdd(&dedup, child);
        }
    }
    int ret = (int)dedup.size;
    TierPositionHashSetDestroy(&dedup);

    return ret;
}

static int MillsGetCanonicalChildPositions(
    TierPosition tier_position,
    TierPosition children[static kTierSolverNumChildPositionsMax]) {
    // Unhash
    uint64_t patterns[2];
    int turn;
    MillsTier t = Unhash(tier_position, patterns, &turn);

    // Generate moves
    Move moves[kTierSolverNumMovesMax];
    int num_moves = GenerateMovesInternal(t, patterns, turn, moves);

    // Collect children
    TierPositionHashSet dedup;
    TierPositionHashSetInit(&dedup, 0.5);
    TierPositionHashSetReserve(&dedup, num_moves);
    int ret = 0;
    for (int i = 0; i < num_moves; ++i) {
        MillsMove m = {.hash = moves[i]};
        TierPosition child = MillsDoMoveInternal(t, patterns, m, turn);
        child.position = MillsGetCanonicalPosition(child);
        // child = GetCanonicalTierPosition(child);
        if (!TierPositionHashSetContains(&dedup, child)) {
            TierPositionHashSetAdd(&dedup, child);
            children[ret++] = child;
        }
    }
    TierPositionHashSetDestroy(&dedup);

    return ret;
}

static void AddCanonicalParent(
    MillsTier pt, const uint64_t patterns[2], int opp_turn,
    PositionHashSet *dedup,
    Position parents[static kTierSolverNumParentPositionsMax], int *ret) {
    //
    TierPosition parent = {
        .tier = pt.hash,
        .position = X86SimdTwoPieceHashHashMem(patterns, opp_turn),
    };
    parent.position = MillsGetCanonicalPosition(parent);
    if (!PositionHashSetContains(dedup, parent.position)) {
        PositionHashSetAdd(dedup, parent.position);
        parents[(*ret)++] = parent.position;
    }
}

static uint64_t BuildOpponentNoCapturePossibleDestMask(
    const uint64_t patterns[2], int turn) {
    //
    uint64_t mask;
    int opp_turn = !turn;
    if (BuildLegalRemovalsMask(patterns[turn])) {  // We have vulnerable pieces.
        // The opponent could only have moved/placed those pieces that are
        // currently not in a mill. Otherwise, they would have removed one of
        // our pieces.
        mask = BuildNotInMillMask(patterns[opp_turn]);
    } else {
        // The opponent had no legal removals to make in the previous turn.
        // This means that the opponent could have moved/placed any piece on the
        // board, even those that are currently in a mill.
        mask = patterns[opp_turn];
    }

    return mask;
}

static int GetParentsSlidingNoCapture(
    MillsTier t, uint64_t patterns[2], int turn,
    Position parents[static kTierSolverNumParentPositionsMax]) {
    //
    int opp_turn = !turn;
    const uint64_t blanks = BuildBlanksMask(patterns);
    PositionHashSet dedup;
    PositionHashSetInit(&dedup, 0.5);
    PositionHashSetReserve(&dedup, 256);
    int ret = 0;
    for (uint64_t opp_possible_dests =
             BuildOpponentNoCapturePossibleDestMask(patterns, turn);
         opp_possible_dests;
         opp_possible_dests = _blsr_u64(opp_possible_dests)) {
        int8_t dest = (int8_t)_tzcnt_u64(opp_possible_dests);
        uint64_t dest_mask = 1ULL << dest;
        for (uint64_t sources = BuildDestMask(t, opp_turn, dest, blanks);
             sources; sources = _blsr_u64(sources)) {
            uint64_t move_mask = dest_mask | _blsi_u64(sources);
            patterns[opp_turn] ^= move_mask;  // Undo opponent's move
            AddCanonicalParent(t, patterns, opp_turn, &dedup, parents, &ret);
            patterns[opp_turn] ^= move_mask;  // Redo opponent's move
        }
    }
    PositionHashSetDestroy(&dedup);

    return ret;
}

static int GetParentsPlacingNoCapture(
    MillsTier pt, uint64_t patterns[2], int turn,
    Position parents[static kTierSolverNumParentPositionsMax]) {
    //
    int opp_turn = !turn;
    PositionHashSet dedup;
    PositionHashSetInit(&dedup, 0.5);
    PositionHashSetReserve(&dedup, 256);
    int ret = 0;
    for (uint64_t opp_possible_dests =
             BuildOpponentNoCapturePossibleDestMask(patterns, turn);
         opp_possible_dests;
         opp_possible_dests = _blsr_u64(opp_possible_dests)) {
        uint64_t move_mask = _blsi_u64(opp_possible_dests);
        patterns[opp_turn] ^= move_mask;  // Undo opponent's placement
        AddCanonicalParent(pt, patterns, opp_turn, &dedup, parents, &ret);
        patterns[opp_turn] ^= move_mask;  // Redo opponent's placement
    }
    PositionHashSetDestroy(&dedup);

    return ret;
}

/**
 * @brief Assuming that the opponent had just removed one of our pieces to
 * reach the current board, returns a mask with all set bits corresponding to
 * possible removal locations.
 */
static uint64_t BuildPriorLegalRemovalsMask(uint64_t patterns[2], int turn) {
    uint64_t ret = 0ULL;
    for (uint64_t blanks = BuildBlanksMask(patterns); blanks;
         blanks = _blsr_u64(blanks)) {
        uint64_t candidate = _blsi_u64(blanks);
        patterns[turn] ^= candidate;  // Place a piece at the candidate location
        uint64_t legal_removals = BuildLegalRemovalsMask(patterns[turn]);
        patterns[turn] ^= candidate;  // Revert placement
        if (legal_removals & candidate) {
            ret |= candidate;
        }
    }

    return ret;
}

static int GetParentsPlacingCapture(
    MillsTier pt, uint64_t patterns[2], int turn,
    Position parents[static kTierSolverNumParentPositionsMax]) {
    //
    uint64_t prior_legal_removals = BuildPriorLegalRemovalsMask(patterns, turn);
    int opp_turn = !turn;
    PositionHashSet dedup;
    PositionHashSetInit(&dedup, 0.5);
    PositionHashSetReserve(&dedup, 256);
    int ret = 0;
    for (uint64_t opp_possible_dests = BuildInMillMask(patterns[opp_turn]);
         opp_possible_dests;
         opp_possible_dests = _blsr_u64(opp_possible_dests)) {
        uint64_t dest_mask = _blsi_u64(opp_possible_dests);
        patterns[opp_turn] ^= dest_mask;  // Undo opponent's placement

        for (uint64_t plr = prior_legal_removals; plr; plr = _blsr_u64(plr)) {
            uint64_t capture_mask = _blsi_u64(plr);
            patterns[turn] ^= capture_mask;  // Undo opponent's capture
            AddCanonicalParent(pt, patterns, opp_turn, &dedup, parents, &ret);
            patterns[turn] ^= capture_mask;  // Redo opponent's capture
        }

        patterns[opp_turn] ^= dest_mask;  // Redo opponent's placement
    }
    PositionHashSetDestroy(&dedup);

    return ret;
}

static int GetParentsSlidingCapture(
    MillsTier pt, uint64_t patterns[2], int turn,
    Position parents[static kTierSolverNumParentPositionsMax]) {
    //
    uint64_t prior_legal_removals = BuildPriorLegalRemovalsMask(patterns, turn);
    int opp_turn = !turn;
    const uint64_t blanks = BuildBlanksMask(patterns);
    PositionHashSet dedup;
    PositionHashSetInit(&dedup, 0.5);
    PositionHashSetReserve(&dedup, 256);
    int ret = 0;
    for (uint64_t opp_possible_dests = BuildInMillMask(patterns[opp_turn]);
         opp_possible_dests;
         opp_possible_dests = _blsr_u64(opp_possible_dests)) {
        int8_t dest = (int8_t)_tzcnt_u64(opp_possible_dests);
        uint64_t dest_mask = 1ULL << dest;
        for (uint64_t sources = BuildDestMask(pt, opp_turn, dest, blanks);
             sources; sources = _blsr_u64(sources)) {
            uint64_t src_mask = _blsi_u64(sources);
            uint64_t move_mask = dest_mask | src_mask;
            patterns[opp_turn] ^= move_mask;  // Undo opponent's move

            for (uint64_t plr = prior_legal_removals & (~src_mask); plr;
                 plr = _blsr_u64(plr)) {
                uint64_t capture_mask = _blsi_u64(plr);
                patterns[turn] ^= capture_mask;  // Undo opponent's capture
                AddCanonicalParent(pt, patterns, opp_turn, &dedup, parents,
                                   &ret);
                patterns[turn] ^= capture_mask;  // Redo opponent's capture
            }

            patterns[opp_turn] ^= move_mask;  // Redo opponent's move
        }
    }
    PositionHashSetDestroy(&dedup);

    return ret;
}

static int MillsGetCanonicalParentPositions(
    TierPosition tier_position, Tier parent_tier,
    Position parents[static kTierSolverNumParentPositionsMax]) {
    // Unhash
    uint64_t patterns[2];
    int turn;
    MillsTier t = Unhash(tier_position, patterns, &turn);
    MillsTier pt = {.hash = parent_tier};
    int opp_turn = !turn;
    if (pt.hash == t.hash) {  // No tier transition
        return GetParentsSlidingNoCapture(t, patterns, turn, parents);
    } else if (pt.unpacked.on_board[opp_turn] ==
               t.unpacked.on_board[opp_turn] - 1) {  // Opponent placed
        if (pt.unpacked.on_board[turn] == t.unpacked.on_board[turn]) {
            return GetParentsPlacingNoCapture(pt, patterns, turn, parents);
        } else {
            return GetParentsPlacingCapture(pt, patterns, turn, parents);
        }
    } else if (pt.unpacked.on_board[turn] - 1 == t.unpacked.on_board[turn]) {
        // Opponent captured one of our pieces but didn't place a new piece
        return GetParentsSlidingCapture(pt, patterns, turn, parents);
    }

    // It is also possible that the current turn does not match the tier
    // transition that happened. In this case, no parent exists in the given
    // parent_tier.
    return 0;
}

// static Position MillsGetPositionInSymmetricTier(TierPosition tier_position,
//                                                 Tier symmetric) {
//     if (tier_position.tier == symmetric) {
//         return tier_position.position;
//     }

//     // Unhash
//     MillsTier t;
//     int turn;
//     bool not_fixed_turn;
//     __m128i board = UnhashSimd(tier_position, &t, &turn, &not_fixed_turn);

//     // Swap colors
//     board = X86SimdTwoPieceHashSwapPieces(board);

//     // Hash
//     if (not_fixed_turn) return X86SimdTwoPieceHashHash(board, !turn);
//     return X86SimdTwoPieceHashHashFixedTurn(board);
// }

// static Tier MillsGetCanonicalTier(Tier tier) {
//     MillsTier t = {.hash = tier};
//     if (!Lasker()) {
//         if (IsPlacementTier(t)) return tier;
//     } else if (tier == second_lasker_tier.hash) {
//         return tier;
//     }

//     // Swap colors
//     MillsTier symmetric = t;
//     int8_t tmp = symmetric.unpacked.remaining[0];
//     symmetric.unpacked.remaining[0] = symmetric.unpacked.remaining[1];
//     symmetric.unpacked.remaining[1] = tmp;
//     tmp = symmetric.unpacked.on_board[0];
//     symmetric.unpacked.on_board[0] = symmetric.unpacked.on_board[1];
//     symmetric.unpacked.on_board[1] = tmp;

//     return symmetric.hash < t.hash ? symmetric.hash : t.hash;
// }

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

        // Placement is possible if the current player has at least 1
        // remaining piece
        if (t.unpacked.remaining[turn]) {
            // Placement without capturing is always possible when placement
            // is possible
            --t.unpacked.remaining[turn];
            ++t.unpacked.on_board[turn];
            children[ret++] = t.hash;

            // Place-and-capture is possible only if the current player has
            // 3 or more pieces on the board after placing
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

    return ret;
}

static int MillsGetChildTiers(
    Tier tier, Tier children[static kTierSolverNumChildTiersMax]) {
    // Primitive tiers have no children.
    MillsTier t = {.hash = tier};
    int8_t rem_x = t.unpacked.remaining[0], rem_o = t.unpacked.remaining[1];
    int8_t num_x = t.unpacked.on_board[0], num_o = t.unpacked.on_board[1];
    if (rem_x + num_x == 2 || rem_o + num_o == 2) return 0;

    // Not using Lasker rule
    if (!Lasker()) return GetChildTiersInternal(t, children);

    // Using Lasker rule
    return GetChildTiersLaskerInternal(t, children);
}

TierType MillsGetTierType(Tier tier) {
    MillsTier t = {.hash = tier};
    if (GetTurnFromTier(t) >= 0) return kTierTypeImmediateTransition;

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

static void FillChars(const char *fmt, int count, const char *chars,
                      char *out_buf) {
    const char *p = fmt;
    char *out = out_buf;
    int chars_used = 0;

    while (*p) {
        if (*p == '%' && *(p + 1) == 'c' && chars_used < count) {
            *out++ = chars[chars_used++];
            p += 2;
        } else {
            *out++ = *p++;
        }
    }

    *out = '\0';
}

static void PatternsToStr(uint64_t patterns[2], char *buffer) {
    int board_id = BoardId();
    int num_slots = kNumSlots[board_id];
    for (int i = 0; i < num_slots; ++i) {
        if ((patterns[0] >> kBoardIdxToGridIdx[board_id][i]) & 1ULL) {
            buffer[i] = 'X';
        } else if ((patterns[1] >> kBoardIdxToGridIdx[board_id][i]) & 1ULL) {
            buffer[i] = 'O';
        } else {
            buffer[i] = '-';
        }
    }
}

static int MillsTierPositionToString(TierPosition tier_position, char *buffer) {
    // Unhash
    uint64_t patterns[2];
    int turn;
    MillsTier t = Unhash(tier_position, patterns, &turn);
    char board[25];
    PatternsToStr(patterns, board);

    char tmp[2048];
    FillChars(kFormats[BoardId()], kNumSlots[BoardId()], board, tmp);
    sprintf(buffer, tmp, t.unpacked.remaining[0], t.unpacked.on_board[0],
            t.unpacked.remaining[1], t.unpacked.on_board[1]);
    printf("it is %d's turn\n", turn);

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

    // .GetPositionInSymmetricTier = MillsGetPositionInSymmetricTier,
    // .GetCanonicalTier = MillsGetCanonicalTier,

    .GetChildTiers = MillsGetChildTiers,
    .GetTierType = MillsGetTierType,
    .GetTierName = MillsGetTierName,

    .position_string_length_max = 2047,
    .TierPositionToString = MillsTierPositionToString,
};

// ============================= kMillsGameplayApi =============================

MoveArray MillsGenerateMovesGameplay(TierPosition tier_position) {
    Move moves[kTierSolverNumMovesMax];
    int num_moves = MillsGenerateMoves(tier_position, moves);
    MoveArray ret;
    MoveArrayInit(&ret);
    for (int i = 0; i < num_moves; ++i) {
        MoveArrayAppend(&ret, moves[i]);
    }

    return ret;
}

static int GetBoardIndex(int8_t grid_index) {
    return kGridIdxToBoardIdx[BoardId()][grid_index];
}

static int MillsMoveToString(Move move, char *buffer) {
    MillsMove m = {.hash = move};
    if (m.unpacked.src == kFromRemaining) {  // placement
        if (m.unpacked.remove == kNoRemoval) {
            sprintf(buffer, "%d", GetBoardIndex(m.unpacked.dest));
        } else {
            sprintf(buffer, "%dr%d", GetBoardIndex(m.unpacked.dest),
                    GetBoardIndex(m.unpacked.remove));
        }
    } else {  // sliding/flying
        if (m.unpacked.remove == kNoRemoval) {
            sprintf(buffer, "%d-%d", GetBoardIndex(m.unpacked.src),
                    GetBoardIndex(m.unpacked.dest));
        } else {
            sprintf(buffer, "%d-%dr%d", GetBoardIndex(m.unpacked.src),
                    GetBoardIndex(m.unpacked.dest),
                    GetBoardIndex(m.unpacked.remove));
        }
    }

    return kNoError;
}

static bool MillsIsValidMoveString(ReadOnlyString move_string) {
    (void)move_string;
    return true;
}

static int8_t GetGridIndex(int board_index) {
    return kBoardIdxToGridIdx[BoardId()][board_index];
}

static Move MillsStringToMove(ReadOnlyString move_string) {
    MillsMove m = kMillsMoveInit;
    m.unpacked.src = kFromRemaining;
    m.unpacked.remove = kNoRemoval;

    int src, dest, remove;
    if (sscanf(move_string, "%d-%dr%d", &src, &dest, &remove) == 3) {
        m.unpacked.src = GetGridIndex(src);
        m.unpacked.dest = GetGridIndex(dest);
        m.unpacked.remove = GetGridIndex(remove);
    } else if (sscanf(move_string, "%d-%d", &src, &dest) == 2) {
        m.unpacked.src = GetGridIndex(src);
        m.unpacked.dest = GetGridIndex(dest);
    } else if (sscanf(move_string, "%dr%d", &dest, &remove) == 2) {
        m.unpacked.dest = GetGridIndex(dest);
        m.unpacked.remove = GetGridIndex(remove);
    } else if (sscanf(move_string, "%d", &dest) == 1) {
        m.unpacked.dest = GetGridIndex(dest);
    }

    return m.hash;
}

static const GameplayApiCommon MillsGameplayApiCommon = {
    .GetInitialPosition = MillsGetInitialPosition,
    .position_string_length_max = 2047,

    .move_string_length_max = 8,
    .MoveToString = MillsMoveToString,

    .IsValidMoveString = MillsIsValidMoveString,
    .StringToMove = MillsStringToMove,
};

static const GameplayApiTier MillsGameplayApiTier = {
    .GetInitialTier = MillsGetInitialTier,

    .TierPositionToString = MillsTierPositionToString,

    .GenerateMoves = MillsGenerateMovesGameplay,
    .DoMove = MillsDoMove,
    .Primitive = MillsPrimitive,
};

static const GameplayApi kMillsGameplayApi = {
    .common = &MillsGameplayApiCommon,
    .tier = &MillsGameplayApiTier,
};

// ========================== MillsGetCurrentVariant ==========================

static const GameVariant *MillsGetCurrentVariant(void) {
    return &current_variant;
}

// =========================== MillsSetVariantOption ===========================

static void UpdateSecondLaskerTier(void) {
    int8_t n = kPiecesPerPlayer[BoardId()];
    second_lasker_tier.unpacked.on_board[0] = 1;
    second_lasker_tier.unpacked.on_board[1] = 0;
    second_lasker_tier.unpacked.remaining[0] = n - 1;
    second_lasker_tier.unpacked.remaining[1] = n;
}

static int MillsSetVariantOption(int option, int selection) {
    if (selection < 0) return kIllegalArgumentError;
    switch (option) {
        case 0: {  // Board and pieces
            if (selection >= (int)NUM_BOARD_AND_PIECES_CHOICES) {
                return kIllegalArgumentError;
            }
            int error =
                X86SimdTwoPieceHashInitIrregular(kBoardMasks[selection]);
            assert(error == kNoError);
            (void)error;
            break;
        }

        case 1:  // Flying rule
        case 3:  // Lasker rule
            if (selection >= 3) return kIllegalArgumentError;
            break;

        case 2:  // Removal rule
        case 4:  // Misère
            if (selection >= 2) return kIllegalArgumentError;
            break;

        default:
            return kIllegalArgumentError;
    }
    variant_option_selections.array[option] = selection;
    if (option == 0) UpdateSecondLaskerTier();

    return kNoError;
}

// ================================= MillsInit =================================

static int MillsInit(void *aux) {
    (void)aux;  // Unused.

    // Initialize the default variant.
    for (int i = 1; i < 5; ++i) {
        int ret = MillsSetVariantOption(i, 0);
        assert(ret == kNoError);
        (void)ret;
    }
    BuildGridIdxToBoardIdx();

    return MillsSetVariantOption(0, 3);
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
    .gameplay_api = &kMillsGameplayApi,
    .uwapi = NULL,  // TODO

    .Init = MillsInit,
    .Finalize = MillsFinalize,

    .GetCurrentVariant = MillsGetCurrentVariant,
    .SetVariantOption = MillsSetVariantOption,
};
