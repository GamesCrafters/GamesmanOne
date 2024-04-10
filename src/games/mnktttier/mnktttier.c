/**
 * @file mnktttier.c
 * @author Dan Garcia: designed and developed of the original version (mttt.c in
 * GamesmanClassic.)
 * @author Max Delgadillo: developed of the first tiered version using generic
 * hash.
 * @author Robert Shi (robertyishi@berkeley.edu): adapted to the new system.
 *         GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @author Robert Shi (robertyishi@berkeley.edu): implemented MNK version of
 *         Tic-Tac-Tier. 
 * @brief Implementation of MNK Tic-Tac-Tier.
 * @version 1.0.4
 * @date 2024-02-15
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

#include "games/mnktttier/mnktttier.h"

#include <assert.h>    // assert
#include <ctype.h>     // toupper
#include <inttypes.h>  // PRId64
#include <stdbool.h>   // bool, true, false
#include <stddef.h>    // NULL
#include <stdint.h>    // int64_t
#include <stdio.h>     // fprintf, stderr, sprintf
#include <stdlib.h>    // atoi

#include "core/generic_hash/generic_hash.h"
#include "core/solvers/tier_solver/tier_solver.h"
#include "core/types/gamesman_types.h"

// Game, Solver, Gameplay, and UWAPI Functions

static int MnktttierInit(void *aux);
static int MnktttierFinalize(void);

static const GameVariant *MnktttierGetCurrentVariant(void);
static int MnktttierSetVariantOption(int option, int selection);

static Tier MnktttierGetInitialTier(void);
static Position MnktttierGetInitialPosition(void);

static int64_t MnktttierGetTierSize(Tier tier);
static MoveArray MnktttierGenerateMoves(TierPosition tier_position);
static Value MnktttierPrimitive(TierPosition tier_position);
static TierPosition MnktttierDoMove(TierPosition tier_position, Move move);
static bool MnktttierIsLegalPosition(TierPosition tier_position);
static Position MnktttierGetCanonicalPosition(TierPosition tier_position);
static PositionArray MnktttierGetCanonicalParentPositions(
    TierPosition tier_position, Tier parent_tier);
static TierArray MnktttierGetChildTiers(Tier tier);

static int MnktttierGetTierName(char *dest, Tier tier);

static int MtttTierPositionToString(TierPosition tier_position, char *buffer);
static int MnktttierMoveToString(Move move, char *buffer);
static bool MnktttierIsValidMoveString(ReadOnlyString move_string);
static Move MnktttierStringToMove(ReadOnlyString move_string);

static bool MnktttierIsLegalFormalPosition(ReadOnlyString formal_position);
static TierPosition MnktttierFormalPositionToTierPosition(
    ReadOnlyString formal_position);
static CString MnktttierTierPositionToFormalPosition(TierPosition tier_position);
static CString MnktttierTierPositionToAutoGuiPosition(TierPosition tier_position);
static CString MnktttierMoveToFormalMove(TierPosition tier_position, Move move);
static CString MnktttierMoveToAutoGuiMove(TierPosition tier_position, Move move);

// Solver API Setup
static const TierSolverApi kSolverApi = {
    .GetInitialTier = &MnktttierGetInitialTier,
    .GetInitialPosition = &MnktttierGetInitialPosition,

    .GetTierSize = &MnktttierGetTierSize,
    .GenerateMoves = &MnktttierGenerateMoves,
    .Primitive = &MnktttierPrimitive,
    .DoMove = &MnktttierDoMove,
    .IsLegalPosition = &MnktttierIsLegalPosition,
    .GetCanonicalPosition = &MnktttierGetCanonicalPosition,
    .GetCanonicalChildPositions = NULL,
    .GetCanonicalParentPositions = &MnktttierGetCanonicalParentPositions,
    .GetPositionInSymmetricTier = NULL,
    .GetChildTiers = &MnktttierGetChildTiers,
    .GetParentTiers = NULL,
    .GetCanonicalTier = NULL,

    .GetTierName = &MnktttierGetTierName,
};

// Gameplay API Setup

static const GameplayApiCommon kMnktttierGameplayApiCommon = {
    .GetInitialPosition = &MnktttierGetInitialPosition,
    .position_string_length_max = 1024,

    .move_string_length_max = 2,
    .MoveToString = &MnktttierMoveToString,

    .IsValidMoveString = &MnktttierIsValidMoveString,
    .StringToMove = &MnktttierStringToMove,
};

static const GameplayApiTier kMnktttierGameplayApiTier = {
    .GetInitialTier = &MnktttierGetInitialTier,

    .TierPositionToString = &MtttTierPositionToString,

    .GenerateMoves = &MnktttierGenerateMoves,
    .DoMove = &MnktttierDoMove,
    .Primitive = &MnktttierPrimitive,
};

static const GameplayApi kMnktttierGameplayApi = {
    .common = &kMnktttierGameplayApiCommon,
    .tier = &kMnktttierGameplayApiTier,
};

// UWAPI Setup

static const UwapiTier kMnktttierUwapiTier = {
    .GenerateMoves = &MnktttierGenerateMoves,
    .DoMove = &MnktttierDoMove,
    .IsLegalFormalPosition = &MnktttierIsLegalFormalPosition,
    .FormalPositionToTierPosition = &MnktttierFormalPositionToTierPosition,
    .TierPositionToFormalPosition = &MnktttierTierPositionToFormalPosition,
    .TierPositionToAutoGuiPosition = &MnktttierTierPositionToAutoGuiPosition,
    .MoveToFormalMove = &MnktttierMoveToFormalMove,
    .MoveToAutoGuiMove = &MnktttierMoveToAutoGuiMove,
    .GetInitialTier = &MnktttierGetInitialTier,
    .GetInitialPosition = &MnktttierGetInitialPosition,
    .GetRandomLegalTierPosition = NULL,
};

// static const Uwapi kMnktttierUwapi = {.tier = &kMnktttierUwapiTier};

const Game kMnktttier = {
    .name = "mnktttier",
    .formal_name = "MNK Tic-Tac-Tier",
    .solver = &kTierSolver,
    .solver_api = (const void *)&kSolverApi,
    .gameplay_api = (const GameplayApi *)&kMnktttierGameplayApi,
    // .uwapi = &kMnktttierUwapi,

    .Init = &MnktttierInit,
    .Finalize = &MnktttierFinalize,

    .GetCurrentVariant = &MnktttierGetCurrentVariant,
    .SetVariantOption = &MnktttierSetVariantOption,
};

// Helper Types and Global Variables

static int M = 3;
static int N = 3;
static int K = 3;

static int kNumRowsToCheck;
static int **kRowsToCheck = NULL;
static int kNumSymmetries;
static int **kSymmetryMatrix = NULL;

// Helper Functions

static bool InitGenericHash(void);
static char KInARow(ReadOnlyString board, const int *indices);
static bool AllFilledIn(ReadOnlyString board);
static void CountPieces(ReadOnlyString board, int *xcount, int *ocount);
static char WhoseTurn(ReadOnlyString board);
static Position DoSymmetry(TierPosition tier_position, int symmetry);
static char ConvertBlankToken(char piece);

// Variant handling
static ConstantReadOnlyString kMnktttierMChoices[] = {"2", "3", "4", "5"};
static ConstantReadOnlyString kMnktttierNChoices[] = {"2", "3", "4", "5"};
static ConstantReadOnlyString kMnktttierKChoices[] = {"2", "3", "4", "5"};

static const GameVariantOption kMnktttierM = {
    .name = "M",
    .num_choices = 4,
    .choices = kMnktttierMChoices,
};

static const GameVariantOption kMnktttierN = {
    .name = "N",
    .num_choices = 4,
    .choices = kMnktttierNChoices,
};

static const GameVariantOption kMnktttierK = {
    .name = "K",
    .num_choices = 4,
    .choices = kMnktttierKChoices,
};

static bool InitArrays(void) {
    int offset = 0;
    kNumRowsToCheck = M * (N - K + 1) + N * (M - K + 1) + 2 * (M - K + 1) * (N - K + 1);
    kRowsToCheck = calloc(kNumRowsToCheck, sizeof(int*));
    if (kRowsToCheck == NULL) {
        return false;
    }
    // Add all vertical lines
    for (int i = 0; i < M; i++) {
        for (int j = 0; j < N - K + 1; j++) {
            kRowsToCheck[offset] = malloc(K * sizeof(int));
            if (kRowsToCheck[offset] ==  NULL) {
                return false;
            }
            for (int k = 0; k < K; k++){
                kRowsToCheck[offset][k] = i * N + j + k;
            }
            offset++;
        }
    }
    // Add all horizontal lines
    for (int i = 0; i < M - K + 1; i++) {
        for (int j = 0; j < N; j++) {
            kRowsToCheck[offset] = malloc(K * sizeof(int));
            if (kRowsToCheck[offset] ==  NULL) {
                return false;
            }
            for (int k = 0; k < K; k++){
                kRowsToCheck[offset][k] = (i + k) * N + j;
            }
            offset++;
        }
    }
    // Add all diagonal lines
    for (int i = 0; i < M - K + 1; i++) {
        for (int j = 0; j < N - K + 1; j++) {
            kRowsToCheck[offset] = malloc(K * sizeof(int));
            if (kRowsToCheck[offset] ==  NULL) {
                return false;
            }
            for (int k = 0; k < K; k++){
                kRowsToCheck[offset][k] = (i + k) * N + j + k;
            }
            offset++;
        }
    }
    for (int i = 0; i < M - K + 1; i++) {
        for (int j = K - 1; j < N; j++) {
            kRowsToCheck[offset] = malloc(K * sizeof(int));
            if (kRowsToCheck[offset] ==  NULL) {
                return false;
            }
            for (int k = 0; k < K; k++){
                kRowsToCheck[offset][k] = (i + k) * N + j - k;
            }
            offset++;
        }
    }
    // Initialize Symmetries
    kNumSymmetries = (M == K) ? 8 : 4;
    kSymmetryMatrix = calloc(kNumSymmetries, sizeof(int*));
    if (kSymmetryMatrix ==  NULL) {
        return false;
    }
    for (int i = 0; i < kNumSymmetries; i++) {
        kSymmetryMatrix[i] = malloc(M * N * sizeof(int));
        if (kSymmetryMatrix[i] ==  NULL) {
            return false;
        }
    }
    for (int i = 0; i < M; i++) { 
        for (int j = 0; j < N; j++) {
            kSymmetryMatrix[0][i * N + j] = i * N + j;
            kSymmetryMatrix[1][i * N + j] = (M - i - 1) * N + j;
            kSymmetryMatrix[2][i * N + j] = i * N + (N - j - 1);
            kSymmetryMatrix[3][i * N + j] = (M - i - 1) * N + (N - j - 1);
        }
    }
    if (M == N) {
        for (int i = 0; i < M; i++) { 
            for (int j = 0; j < N; j++) {
                kSymmetryMatrix[4][i * N + j] = j * N + i;
                kSymmetryMatrix[5][i * N + j] = (M - j - 1) * N + i;
                kSymmetryMatrix[6][i * N + j] = j * N + (N - i - 1);
                kSymmetryMatrix[7][i * N + j] = (M - j - 1) * N + (N - i - 1);
            }
        }
    }
    return true;
}

static void FreeArrays(void) {
    for (int i = 0; i < kNumRowsToCheck; i++) {
        if (kRowsToCheck[i] != NULL) free(kRowsToCheck[i]);
    }
    if (kRowsToCheck != NULL) free(kRowsToCheck);
    kRowsToCheck = NULL;
    for (int i = 0; i < kNumSymmetries; i++) {
        if (kSymmetryMatrix[i] != NULL) free(kSymmetryMatrix[i]);
    }
    if (kSymmetryMatrix != NULL) free(kSymmetryMatrix);
    kSymmetryMatrix = NULL;
}

#define NUM_OPTIONS 4  // 3 options and 1 zero-terminator.
static GameVariantOption options[NUM_OPTIONS];
static int selections[NUM_OPTIONS] = {4, 4, 4, 0};
#undef NUM_OPTIONS
static GameVariant current_variant;

static const GameVariant *MnktttierGetCurrentVariant(void) {
    return &current_variant;
}

static int MnktttierSetVariantOption(int option, int selection) {
    FreeArrays();
    if (option == 0 && selection >= 0 && selection < 4) {
        M = atoi(kMnktttierMChoices[selection]);
        printf("setting M to %d\n", M);
    } else if (option == 1 && selection >= 0 && selection < 4) {
        N = atoi(kMnktttierNChoices[selection]);
        printf("setting N to %d\n", N);
    } else if (option == 2 && selection >= 0 && selection < 4) {
        K = atoi(kMnktttierKChoices[selection]);
        printf("setting K to %d\n", K);
    } else {
        return kRuntimeError;
    }
    InitArrays();
    InitGenericHash();
    return kNoError;
}

static int MnktttierInit(void *aux) {
    (void)aux;  // Unused.
    // Initialize variant options.
    options[0] = kMnktttierM;
    options[1] = kMnktttierN;
    options[2] = kMnktttierK;
    current_variant.options = options;
    current_variant.selections = selections;
    return !InitArrays() || !InitGenericHash();
}

static int MnktttierFinalize(void) {
    FreeArrays();
    return kNoError; 
}

static Tier MnktttierGetInitialTier(void) { return 0; }

// Assumes Generic Hash has been initialized.
static Position MnktttierGetInitialPosition(void) {
    char board[M * N];
    for (int i = 0; i < M * N; i++) {
        board[i] = '-';
    }
    return GenericHashHashLabel(0, board, 1);
}

static int64_t MnktttierGetTierSize(Tier tier) {
    return GenericHashNumPositionsLabel(tier);
}

static MoveArray MnktttierGenerateMoves(TierPosition tier_position) {
    MoveArray moves;
    MoveArrayInit(&moves);

    if (MnktttierPrimitive(tier_position) != kUndecided) return moves;

    char board[M * N];
    GenericHashUnhashLabel(tier_position.tier, tier_position.position, board);
    for (Move i = 0; i < M * N; i++) {
        if (board[i] == '-') {
            MoveArrayAppend(&moves, i);
        }
    }
    return moves;
}

static Value MnktttierPrimitive(TierPosition tier_position) {
    char board[M * N];
    GenericHashUnhashLabel(tier_position.tier, tier_position.position, board);

    for (int i = 0; i < kNumRowsToCheck; ++i) {
        if (KInARow(board, kRowsToCheck[i]) > 0) return kLose;
    }
    if (AllFilledIn(board)) return kTie;
    return kUndecided;
}

static TierPosition MnktttierDoMove(TierPosition tier_position, Move move) {
    char board[M * N];
    GenericHashUnhashLabel(tier_position.tier, tier_position.position, board);
    char turn = WhoseTurn(board);
    board[move] = turn;
    TierPosition ret;
    ret.tier = tier_position.tier + 1;
    ret.position = GenericHashHashLabel(ret.tier, board, 1);
    return ret;
}

static bool MnktttierIsLegalPosition(TierPosition tier_position) {
    // A position is legal if and only if:
    // 1. xcount == ocount or xcount = ocount + 1 if no one is winning and
    // 2. xcount == ocount if O is winning and
    // 3. xcount == ocount + 1 if X is winning and
    // 4. only one player can be winning
    char board[M * N];
    GenericHashUnhashLabel(tier_position.tier, tier_position.position, board);

    int xcount, ocount;
    CountPieces(board, &xcount, &ocount);
    if (xcount != ocount && xcount != (ocount + 1)) return false;

    bool xwin = false, owin = false;
    for (int i = 0; i < kNumRowsToCheck; ++i) {
        char row_value = KInARow(board, kRowsToCheck[i]);
        xwin |= (row_value == 'X');
        owin |= (row_value == 'O');
    }
    if (xwin && owin) return false;
    if (xwin && xcount != ocount + 1) return false;
    if (owin && xcount != ocount) return false;
    return true;
}

static Position MnktttierGetCanonicalPosition(TierPosition tier_position) {
    Position canonical_position = tier_position.position;
    Position new_position;

    for (int i = 0; i < kNumSymmetries; ++i) {
        new_position = DoSymmetry(tier_position, i);
        if (new_position < canonical_position) {
            // By GAMESMAN convention, the canonical position is the one with
            // the smallest hash value.
            canonical_position = new_position;
        }
    }

    return canonical_position;
}

static PositionArray MnktttierGetCanonicalParentPositions(
    TierPosition tier_position, Tier parent_tier) {
    //
    Tier tier = tier_position.tier;
    Position position = tier_position.position;
    PositionArray parents;
    PositionArrayInit(&parents);
    if (parent_tier != tier - 1) return parents;

    PositionHashSet deduplication_set;
    PositionHashSetInit(&deduplication_set, 0.5);

    char board[M * N];
    GenericHashUnhashLabel(tier, position, board);

    char prev_turn = WhoseTurn(board) == 'X' ? 'O' : 'X';
    for (int i = 0; i < M * N; ++i) {
        if (board[i] == prev_turn) {
            // Take piece off the board.
            board[i] = '-';
            TierPosition parent = {
                .tier = tier - 1,
                .position = GenericHashHashLabel(tier - 1, board, 1),
            };
            // Add piece back to the board.
            board[i] = prev_turn;
            if (!MnktttierIsLegalPosition(parent)) {
                continue;  // Illegal.
            }
            parent.position = MnktttierGetCanonicalPosition(parent);
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

static TierArray MnktttierGetChildTiers(Tier tier) {
    TierArray children;
    TierArrayInit(&children);
    if (tier < M * N) TierArrayAppend(&children, tier + 1);
    return children;
}

static int MnktttierGetTierName(char *dest, Tier tier) {
    return sprintf(dest, "%" PRITier "p", tier);
}

static int MtttTierPositionToString(TierPosition tier_position, char *buffer) {
    char board[M * N];
    bool success = GenericHashUnhashLabel(tier_position.tier,
                                          tier_position.position, board);
    if (!success) return kRuntimeError;

    for (int i = 0; i < M * N; i++) {
        board[i] = ConvertBlankToken(board[i]);
    }
    const char prefix[] = "         ( ";
    const char mid_prefix[] = "LEGEND:  ( ";
    const char suffix[] = ")           :";
    const char mid_suffix[] = ")  TOTAL:   :";
    if (M * (sizeof(prefix) - 1 + 3 * N + sizeof(suffix) - 1 + 2 * N + 2) >=
        (unsigned long) kMnktttierGameplayApiCommon.position_string_length_max + 1) {
        fprintf(
            stderr,
            "MnktttierTierPositionToString: (BUG) not enough space was allocated "
            "to buffer. Please increase position_string_length_max.\n");
        return kMemoryOverflowError;
    }
    char* cur = buffer;
    for (int i = 0; i < M; i++) {
        if (i == M/2) {
            sprintf(cur, "%s", mid_prefix);
        } else {
            sprintf(cur, "%s", prefix);
        }
        
        cur += sizeof(prefix) - 1;
        for (int j = 0; j < N; j++) {
            sprintf(cur, "%02d ", i * N + j);
            cur += 3;
        }
        if (i == M/2) {
            sprintf(cur, "%s", mid_suffix);
        } else {
            sprintf(cur, "%s", suffix);
        }
        cur += sizeof(suffix) - 1;
        for (int j = 0; j < N; j++) {
            sprintf(cur, " %c", board[i * N + j]);
            cur += 2;
        }
        *cur = '\n';
        cur += 1;
    }
    *cur = '\0';

    return kNoError;
}

static int MnktttierMoveToString(Move move, char *buffer) {
    int actual_length =
        snprintf(buffer, kMnktttierGameplayApiCommon.move_string_length_max + 1,
                 "%" PRId64, move);
    snprintf(buffer, "%d", move);
    if (actual_length >= kMnktttierGameplayApiCommon.move_string_length_max + 1) {
        fprintf(stderr,
                "MnktttierMoveToString: (BUG) not enough space was allocated "
                "to buffer. Please increase move_string_length_max.\n");
        return kMemoryOverflowError;
    }

    return kNoError;
}

static bool MnktttierIsValidMoveString(ReadOnlyString move_string) {
    // Only "0" - "M*N-1" are valid move strings.
    int move = atoi(move_string);
    if (move < 0) return false;
    if (move >= M * N) return false;
    return true;
}

static Move MnktttierStringToMove(ReadOnlyString move_string) {
    assert(MnktttierIsValidMoveString(move_string));
    return (Move)atoi(move_string);
}

static bool MnktttierIsLegalFormalPosition(ReadOnlyString formal_position) {
    if (formal_position == NULL) return false;
    for (int i = 0; i < M * N; ++i) {
        char curr = formal_position[i];
        if (curr != '-' && curr != 'o' && curr != 'x') return false;
    }
    if (formal_position[M * N] != '\0') return false;

    return true;
}

static TierPosition MnktttierFormalPositionToTierPosition(
    ReadOnlyString formal_position) {
    // Formal position string format: M * N characters '-', 'o', or 'x'.
    char board[M * N];
    int piece_count = 0;
    for (int i = 0; i < M * N; ++i) {
        board[i] = toupper(formal_position[i]);
        piece_count += board[i] != '-';
    }

    TierPosition ret = {
        .tier = piece_count,
        .position = GenericHashHashLabel(piece_count, board, 1),
    };

    return ret;
}

static CString MnktttierTierPositionToFormalPosition(TierPosition tier_position) {
    char board[M * N];
    CString ret = {0};
    bool success = GenericHashUnhashLabel(tier_position.tier,
                                          tier_position.position, board);
    if (!success) return ret;

    for (int i = 0; i < M * N; ++i) {
        board[i] = tolower(board[i]);
    }
    CStringInit(&ret, board);

    return ret;
}

static CString MnktttierTierPositionToAutoGuiPosition(
    TierPosition tier_position) {
    //
    char board[M * N + 3];
    CString ret = {0};
    bool success = GenericHashUnhashLabel(tier_position.tier,
                                          tier_position.position, board + 2);
    if (!success) return ret;
    for (int i = 0; i < M * N; ++i) {
        board[i + 2] = tolower(board[i + 2]);
    }
    board[0] = WhoseTurn(board) == 'X' ? '1' : '2';
    board[1] = '_';
    board[M * N + 2] = '\0';
    CStringInit(&ret, board);
    return ret;
}

static CString MnktttierMoveToFormalMove(TierPosition tier_position, Move move) {
    (void)tier_position;  // Unused.
    CString ret;
    if (!CStringInit(&ret, "00")) return ret;

    assert(move >= 0 && move < M * N);
    sprintf(ret.str, "%02d", move);

    return ret;
}

static CString MnktttierMoveToAutoGuiMove(TierPosition tier_position, Move move) {
    CString ret = {0};
    char board[M * N];
    bool success = GenericHashUnhashLabel(tier_position.tier,
                                          tier_position.position, board);
    if (!success) return ret;
    if (!CStringInit(&ret, "A_x_00")) return ret;

    char turn = WhoseTurn(board);
    ret.str[2] = turn == 'X' ? 'x' : 'o';
    assert(move >= 0 && move < M * N);
    sprintf(ret.str + 4, "%02d", move);

    return ret;
}

// Helper functions implementation

static bool InitGenericHash(void) {
    GenericHashReinitialize();
    int player = 1;  // No turn bit needed as we can infer the turn from board.
    int board_size = M * N;
    int pieces_init_array[10] = {'-', M * N, M * N, 'O', 0, 0, 'X', 0, 0, -1};
    for (Tier tier = 0; tier <= board_size; tier++) {
        // Adjust piece_init_array
        pieces_init_array[1] = pieces_init_array[2] = board_size - tier;
        pieces_init_array[4] = pieces_init_array[5] = tier / 2;
        pieces_init_array[7] = pieces_init_array[8] = (tier + 1) / 2;
        bool success = GenericHashAddContext(player, board_size,
                                             pieces_init_array, NULL, tier);
        if (!success) {
            fprintf(stderr,
                    "MnktttierInit: failed to initialize generic hash context "
                    "for tier %" PRITier ". Aborting...\n",
                    tier);
            GenericHashReinitialize();
            return false;
        }
    }
    return true;
}

static char KInARow(ReadOnlyString board, const int *indices) {
    if (board[indices[0]] == '-') {
        return 0;
    }
    for (int i = 1; i < K; i++) {
        if (board[indices[i]] != board[indices[0]]) {
            return 0;
        }
    }
    return board[indices[0]];
}

static bool AllFilledIn(ReadOnlyString board) {
    for (int i = 0; i < M * N; ++i) {
        if (board[i] == '-') return false;
    }
    return true;
}

static void CountPieces(ReadOnlyString board, int *xcount, int *ocount) {
    *xcount = *ocount = 0;
    for (int i = 0; i < M * N; i++) {
        *xcount += (board[i] == 'X');
        *ocount += (board[i] == 'O');
    }
}

static char WhoseTurn(ReadOnlyString board) {
    int xcount, ocount;
    CountPieces(board, &xcount, &ocount);
    if (xcount == ocount) return 'X';  // In our TicTacToe, x always goes first.
    return 'O';
}

static Position DoSymmetry(TierPosition tier_position, int symmetry) {
    char board[M * N], symmetry_board[M * N];
    GenericHashUnhashLabel(tier_position.tier, tier_position.position, board);

    // Copy from the symmetry matrix.
    for (int i = 0; i < M * N; ++i) {
        symmetry_board[i] = board[kSymmetryMatrix[symmetry][i]];
    }

    return GenericHashHashLabel(tier_position.tier, symmetry_board, 1);
}

static char ConvertBlankToken(char piece) {
    if (piece == '-') return ' ';
    return piece;
}
