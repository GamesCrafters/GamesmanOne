/**
 * @file mninemensmorris.c
 * @author Cameron Cheung (cameroncheung@berkeley.edu): Added to system.
 *         GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Implementation of Nine Men's Morris.
 *
 * @version 1.0
 * @date 2023-09-25
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

#include "games/mninemensmorris/mninemensmorris.h"

#include <assert.h>    // assert
#include <inttypes.h>  // PRId64
#include <stdbool.h>   // bool, true, false
#include <stddef.h>    // NULL
#include <stdint.h>    // int64_t
#include <stdio.h>     // fprintf, stderr
#include <stdlib.h>    // atoi

#include "core/gamesman_types.h"
#include "core/generic_hash/generic_hash.h"
#include "core/solvers/tier_solver/tier_solver.h"

// Game, Solver, and Gameplay API Functions

static int MninemensmorrisInit(void *aux);
static int MninemensmorrisFinalize(void);

static const GameVariant *MninemensmorrisGetCurrentVariant(void);
static int MninemensmorrisSetVariantOption(int option, int selection);

static Tier MninemensmorrisGetInitialTier(void);
static Position MninemensmorrisGetInitialPosition(void);

static int64_t MninemensmorrisGetTierSize(Tier tier);
static MoveArray MninemensmorrisGenerateMoves(TierPosition tier_position);
static Value MninemensmorrisPrimitive(TierPosition tier_position);
static TierPosition MninemensmorrisDoMove(
    TierPosition tier_position, Move move);
static bool MninemensmorrisIsLegalPosition(TierPosition tier_position);
static Position MninemensmorrisGetCanonicalPosition(
    TierPosition tier_position);
static TierPositionArray MninemensmorrisGetCanonicalChildPositions(
    TierPosition tier_position);
static PositionArray MninemensmorrisGetCanonicalParentPositions(
    TierPosition tier_position);
static TierArray MninemensmorrisGetChildTiers(Tier tier);
static TierArray MninemensmorrisGetParentTiers(Tier tier);
static Tier MninemensmorrisGetCanonicalTier(Tier tier);
static Position MninemensmorrisGetPositionInSymmetricTier(
    TierPosition tier_position, Tier symmetric);

static int MninemensmorrisTierPositionToString(
    TierPosition tier_position, char *buffer);
static int MninemensmorrisMoveToString(Move move, char *buffer);
static bool MninemensmorrisIsValidMoveString(ReadOnlyString move_string);
static Move MninemensmorrisStringToMove(ReadOnlyString move_string);

// Solver API Setup
static const TierSolverApi kSolverApi = {
    .GetInitialTier = &MninemensmorrisGetInitialTier,
    .GetInitialPosition = &MninemensmorrisGetInitialPosition,

    .GetTierSize = &MninemensmorrisGetTierSize,
    .GenerateMoves = &MninemensmorrisGenerateMoves,
    .Primitive = &MninemensmorrisPrimitive,
    .DoMove = &MninemensmorrisDoMove,

    .IsLegalPosition = &MninemensmorrisIsLegalPosition,
    .GetCanonicalPosition = &MninemensmorrisGetCanonicalPosition,
    .GetCanonicalChildPositions = &MninemensmorrisGetCanonicalChildPositions,
    .GetCanonicalParentPositions = &MninemensmorrisGetCanonicalParentPositions,
    .GetPositionInSymmetricTier = &MninemensmorrisGetPositionInSymmetricTier,
    .GetChildTiers = &MninemensmorrisGetChildTiers,
    .GetParentTiers = &MninemensmorrisGetParentTiers,
    .GetCanonicalTier = &MninemensmorrisGetCanonicalTier,
};

// Gameplay API Setup
static const GameplayApi kGameplayApi = {
    .GetInitialTier = &MninemensmorrisGetInitialTier,
    .GetInitialPosition = &MninemensmorrisGetInitialPosition,

    .position_string_length_max = 1200,
    .TierPositionToString = &MninemensmorrisTierPositionToString,

    .move_string_length_max = 8,
    .MoveToString = &MninemensmorrisMoveToString,

    .IsValidMoveString = &MninemensmorrisIsValidMoveString,
    .StringToMove = &MninemensmorrisStringToMove,

    .TierGenerateMoves = &MninemensmorrisGenerateMoves,
    .TierDoMove = &MninemensmorrisDoMove,
    .TierPrimitive = &MninemensmorrisPrimitive,

    .TierGetCanonicalPosition = &MninemensmorrisGetCanonicalPosition,

    .GetCanonicalTier = &MninemensmorrisGetCanonicalTier,
    .GetPositionInSymmetricTier = &MninemensmorrisGetPositionInSymmetricTier,
};

const Game kMninemensmorris = {
    .name = "mninemensmorris",
    .formal_name = "Nine Men's Morris",
    .solver = &kTierSolver,
    .solver_api = (const void *)&kSolverApi,
    .gameplay_api = (const GameplayApi *)&kGameplayApi,

    .Init = &MninemensmorrisInit,
    .Finalize = &MninemensmorrisFinalize,

    .GetCurrentVariant = &MninemensmorrisGetCurrentVariant,
    .SetVariantOption = &MninemensmorrisSetVariantOption,
};

// Helper Types and Global Variables

#define MOVE_ENCODE(from, to, remove) ((from << 10) | (to << 5) | remove)
#define THREE_IN_A_ROW(board, slot1, slot2, turn) (board[slot1] == turn && board[slot2] == turn)
#define NONE 31
#define X 'X'
#define O 'O'
#define BLANK '-'

// Variant-Related Global Variables
int gNumPiecesPerPlayer = 9; // Ranges from 3 to 12
bool gFlyRule = true; // If false, no flying phase
int removalRule = 0; // 0: Standard, 1: Lenient, 2: Strict
int boardType = 1; // 0: 16Board, 1: 24Board, 2: 24BoardExt
bool isMisere = false;
int boardSize = 24; // Depends on boardType

const struct GameVariantOption optionMisere = {
    .name = "Misere",
    .num_choices = 2,
    .choices = { "Regular", "Misere" }
};

const struct GameVariantOption optionFlyRule = {
    .name = "Fly Rule",
    .num_choices = 2,
    .choices = { "No Flying Phase", "Flying Phase" }
};

const struct GameVariantOption optionRemovalRule = {
    .name = "Removal Rule",
    .num_choices = 3,
    .choices = { "Standard", "Lenient", "Strict" }
};

const struct GameVariantOption optionBoardType = {
    .name = "Board Type",
    .num_choices = 3,
    .choices = { "16-Board", "24-Board", "24-Board with Added Diags" }
};

const struct GameVariantOption optionNumPiecesPerPlayer = {
    .name = "Number of Pieces Per Player",
    .num_choices = 10,
    .choices = { "3", "4", "5", "6", "7", "8", "9", "10", "11", "12" }
};

// const struct GameVariantOption options[5] = {
//     optionMisere,
//     optionFlyRule,
//     optionRemovalRule,
//     optionBoardType,
//     optionNumPiecesPerPlayer
// };

int adjacent16[16][5] = {
	{1,6,0,0,2},
	{0,2,4,0,3},
	{1,9,0,0,2},
	{4,7,0,0,2},
	{1,3,5,0,3},
	{4,8,0,0,2},
	{0,7,13,0,3},
	{3,6,10,0,3},
	{5,9,12,0,3},
	{2,8,15,0,3},
	{7,11,0,0,2},
	{10,12,14,0,3},
	{8,11,0,0,2},
	{6,14,0,0,2},
	{11,13,15,0,3},
	{9,14,0,0,2}
};

int adjacent24[24][5] = {
	{1,9,0,0,2},
	{0,2,4,0,3},
	{1,14,0,0,2},
	{4,10,0,0,2},
	{1,3,5,7,4},
	{4,13,0,0,2},
	{7,11,0,0,2},
	{4,6,8,0,3},
	{7,12,0,0,2},
	{0,10,21,0,3},
	{3,9,11,18,4},
	{6,10,15,0,3},
	{8,13,17,0,3},
	{5,12,14,20,4},
	{2,13,23,0,3},
	{11,16,0,0,2},
	{15,17,19,0,3},
	{12,16,0,0,2},
	{10,19,0,0,2},
	{16,18,20,22,4},
	{13,19,0,0,2},
	{9,22,0,0,2},
	{19,21,23,0,3},
	{14,22,0,0,2}
};

int adjacent24Ext[24][5] = {
	{1,9,0,0,2},
	{0,2,4,0,3},
	{1,14,0,0,2},
	{4,10,0,0,2},
	{1,3,5,7,4},
	{4,13,0,0,2},
	{7,11,0,0,2},
	{4,6,8,0,3},
	{7,12,0,0,2},
	{0,10,21,0,3},
	{3,9,11,18,4},
	{6,10,15,0,3},
	{8,13,17,0,3},
	{5,12,14,20,4},
	{2,13,23,0,3},
	{11,16,0,0,2},
	{15,17,19,0,3},
	{12,16,0,0,2},
	{10,19,0,0,2},
	{16,18,20,22,4},
	{13,19,0,0,2},
	{9,22,0,0,2},
	{19,21,23,0,3},
	{14,22,0,0,2}
};

int (*adjacent)[5];
adjacent = adjacent24;

static const int kNumGeometricSymmetriesNormal = 16;
static const int kNumGeometricSymmetries33 = 32;

// There are three types of boards, the 16Board, 24Board, and 24BoardExt

static const int gSymmetryMatrix16Board[16][24] = {
	{0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,-1,-1,-1,-1,-1,-1,-1,-1},
	{13,6,0,10,7,3,14,11,4,1,12,8,5,15,9,2,-1,-1,-1,-1,-1,-1,-1,-1},
	{15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0,-1,-1,-1,-1,-1,-1,-1,-1},
	{2,9,15,5,8,12,1,4,11,14,3,7,10,0,6,13,-1,-1,-1,-1,-1,-1,-1,-1},
	{3,4,5,0,1,2,7,6,9,8,13,14,15,10,11,12,-1,-1,-1,-1,-1,-1,-1,-1},
	{10,7,3,13,6,0,11,14,1,4,15,9,2,12,8,5,-1,-1,-1,-1,-1,-1,-1,-1},
	{12,11,10,15,14,13,8,9,6,7,2,1,0,5,4,3,-1,-1,-1,-1,-1,-1,-1,-1},
	{5,8,12,2,9,15,4,1,14,11,0,6,13,3,7,10,-1,-1,-1,-1,-1,-1,-1,-1},
	{2,1,0,5,4,3,9,8,7,6,12,11,10,15,14,13,-1,-1,-1,-1,-1,-1,-1,-1},
	{0,6,13,3,7,10,1,4,11,14,5,8,12,2,9,15,-1,-1,-1,-1,-1,-1,-1,-1},
	{13,14,15,10,11,12,6,7,8,9,3,4,5,0,1,2,-1,-1,-1,-1,-1,-1,-1,-1},
	{15,9,2,12,8,5,14,11,4,1,10,7,3,13,6,0,-1,-1,-1,-1,-1,-1,-1,-1},
	{5,4,3,2,1,0,8,9,6,7,15,14,13,12,11,10,-1,-1,-1,-1,-1,-1,-1,-1},
	{3,7,10,0,6,13,4,1,14,11,2,9,15,5,8,12,-1,-1,-1,-1,-1,-1,-1,-1},
	{10,11,12,13,14,15,7,6,9,8,0,1,2,3,4,5,-1,-1,-1,-1,-1,-1,-1,-1},
	{12,8,5,15,9,2,11,14,1,4,13,6,0,10,7,3,-1,-1,-1,-1,-1,-1,-1,-1}
};

static const int gSymmetryMatrix24Board[16][24] = { 
	{0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23},
	{6,7,8,3,4,5,0,1,2,11,10,9,14,13,12,21,22,23,18,19,20,15,16,17},
	{2,1,0,5,4,3,8,7,6,14,13,12,11,10,9,17,16,15,20,19,18,23,22,21},
	{8,7,6,5,4,3,2,1,0,12,13,14,9,10,11,23,22,21,20,19,18,17,16,15},
	{21,22,23,18,19,20,15,16,17,9,10,11,12,13,14,6,7,8,3,4,5,0,1,2},
	{15,16,17,18,19,20,21,22,23,11,10,9,14,13,12,0,1,2,3,4,5,6,7,8},
	{23,14,2,20,13,5,17,12,8,22,19,16,7,4,1,15,11,6,18,10,3,21,9,0},
	{17,12,8,20,13,5,23,14,2,16,19,22,1,4,7,21,9,0,18,10,3,15,11,6},
	{0,9,21,3,10,18,6,11,15,1,4,7,16,19,22,8,12,17,5,13,20,2,14,23},
	{6,11,15,3,10,18,0,9,21,7,4,1,22,19,16,2,14,23,5,13,20,8,12,17},
	{21,9,0,18,10,3,15,11,6,22,19,16,7,4,1,17,12,8,20,13,5,23,14,2},
	{15,11,6,18,10,3,21,9,0,16,19,22,1,4,7,23,14,2,20,13,5,17,12,8},
	{23,22,21,20,19,18,17,16,15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0},
	{17,16,15,20,19,18,23,22,21,12,13,14,9,10,11,2,1,0,5,4,3,8,7,6},
	{2,14,23,5,13,20,8,12,17,1,4,7,16,19,22,6,11,15,3,10,18,0,9,21},
	{8,12,17,5,13,20,2,14,23,7,4,1,22,19,16,0,9,21,3,10,18,6,11,15}
};

// Helper Functions

static Tier HashTier(int piecesLeft, int numX, int numO);
static void UnhashTier(Tier tier, int *piecesLeft, int *numX, int *numO);
static void UnhashMove(Move move, int *from, int *to, int *remove);

static bool InitGenericHash(void);
static char WhoseTurn(ReadOnlyString board);
static Position DoSymmetry(TierPosition tier_position, int symmetry);

static int MninemensmorrisInit(void *aux) {
    (void)aux;  // Unused.
    return !InitGenericHash();
}

static int MninemensmorrisFinalize(void) { return 0; }

static const GameVariant *MninemensmorrisGetCurrentVariant(void) {
    return NULL;
    //return (gNumPiecesPerPlayer << 6) | (boardType << 4) | (removalRule << 2) | (gFlyRule << 1) | isMisere;  // Not implemented.
}

static int MninemensmorrisSetVariantOption(int option, int selection) {
    switch (option) {
        case 0:
            isMisere = selection == 1;
            break;
        case 1:
            gFlyRule = selection == 1;
            break;
        case 2:
            removalRule = selection;
            break;
        case 3:
            boardType = selection;
            if (selection == 0) {
                boardSize = 16;
            } else if (selection == 1) {
                boardSize = 24;
            } else {
                boardSize = 24;
            }
            break;
        case 4:
            gNumPiecesPerPlayer = selection - 3;
            break;
        default:
            return 1;
    }
    return 0;
}

static Tier MninemensmorrisGetInitialTier(void) { return HashTier(gNumPiecesPerPlayer, 0, 0); }

// Assumes Generic Hash has been initialized.
static Position MninemensmorrisGetInitialPosition(void) {
    char board[boardSize];
    memset(board, '-', boardSize);
    return GenericHashHashLabel(HashTier(gNumPiecesPerPlayer, 0, 0), board, 1);
}

static int64_t MninemensmorrisGetTierSize(Tier tier) {
    return GenericHashNumPositionsLabel(tier);
}

static MoveArray MninemensmorrisGenerateMoves(TierPosition tier_position) {
    MoveArray moves;
    MoveArrayInit(&moves);

    char board[boardSize];
    GenericHashUnhashLabel(tier_position.tier, tier_position.position, board);
    int t = GenericHashGetTurnLabel(tier_position.tier, tier_position.position);
    char turn = (t == 1) ? X : O;
	int piecesLeft, numX, numO;
    UnhashTier(tier_position.tier, &piecesLeft, &numX, &numO);

	int legalRemoves[boardSize];
	int numLegalRemoves = findLegalRemoves(board, turn, legalRemoves);
	int *legalTos;
	int numLegalTos;
    int i, j, from;

	if ((numX > 2 && numO > 2) || piecesLeft > 0) {
        int allBlanks[boardSize];
        int numBlanks = 0;
        for (i = 0; i < boardSize; i++)
            if (board[i] == BLANK)
                allBlanks[numBlanks++] = i;

        if (piecesLeft > 0) {
            for (i = 0; i < numBlanks; i++)
                if (closesMill(board, turn, NONE, allBlanks[i]) && numLegalRemoves > 0)
                    for (j = 0; j < numLegalRemoves; j++)
                        MoveArrayAppend(&moves, MOVE_ENCODE(NONE, allBlanks[i], legalRemoves[j]));
                else
                    MoveArrayAppend(&moves, MOVE_ENCODE(NONE, allBlanks[i], NONE));
        } else {
            for (from = 0; from < boardSize; from++) {
                if (gFlyRule && ((turn == X && numX == 3) || (turn == O && numO == 3))) {
                    legalTos = allBlanks;
                    numLegalTos = numBlanks;
                } else {
                    legalTos = adjacent[from];
                    numLegalTos = adjacent[from][4];
                }
                if (board[from] == turn) {
                    for (i = 0; i < numLegalTos; i++) {
                        if (board[legalTos[i]] == BLANK) {
                            if (closesMill(board, turn, from, legalTos[i]) && numLegalRemoves > 0)
                                for (j = 0; j < numLegalRemoves; j++)
                                    MoveArrayAppend(&moves, MOVE_ENCODE(from, legalTos[i], legalRemoves[j]));
                            else
                                MoveArrayAppend(&moves, MOVE_ENCODE(from, legalTos[i], NONE));
                        }
                    }
                }
            }
        }
    }

    return moves;
}

static Value MninemensmorrisPrimitive(TierPosition tier_position) {
    MoveArray moves = MninemensmorrisGenerateMoves(tier_position);
    if (moves.size == 0) {
        return isMisere ? kWin : kLose;
    }
    return kUndecided;
}

static TierPosition MninemensmorrisDoMove(TierPosition tier_position, Move move) {
    char board[boardSize];
    GenericHashUnhashLabel(tier_position.tier, tier_position.position, board);
    int t = GenericHashGetTurnLabel(tier_position.tier, tier_position.position);
    char turn;
    int oppT;
    if (t == 1) {
        turn = X;
        oppT = 2;
    } else {
        turn == O;
        oppT = 1;
    }
	int piecesLeft, numX, numO;
    UnhashTier(tier_position.tier, &piecesLeft, &numX, &numO);
    int from, to, remove;
    UnhashMove(move, &from, &to, &remove);

    board[to] = turn;
	if (turn == X) {
		turn = O;
		if (from != NONE) { // If sliding
			board[from] = BLANK;
		} else { // Phase 1
			piecesLeft--;
			numX++;
		}
		if (remove != NONE) {
			board[remove] = BLANK;
			numO--;
		}
	} else {
		turn = X;
		if (from != NONE) {
			board[from] = BLANK;
		} else {
			piecesLeft--;
			numO++;
		}
		if (remove != NONE) {
			board[remove] = BLANK;
			numX--;
		}
	}

    TierPosition ret;
    ret.tier = HashTier(piecesLeft, numX, numO);
    ret.position = GenericHashHashLabel(ret.tier, board, oppT);
	return ret; // TODO: ASK QUESTION ABOUT TTTIER ret.position 
}

static bool MninemensmorrisIsLegalPosition(TierPosition tier_position) {
    // Currently we do not have a filter for unreachable positions
    return true;
}

static Position MtttierGetCanonicalPosition(TierPosition tier_position) {
    Position canonical_position = tier_position.position;
    Position new_position;

    // for (int i = 0; i < kNumSymmetries; ++i) {
    //     new_position = DoSymmetry(tier_position, i);
    //     if (new_position < canonical_position) {
    //         // By GAMESMAN convention, the canonical position is the one with
    //         // the smallest hash value.
    //         canonical_position = new_position;
    //     }
    // }

    return canonical_position;
}

static PositionArray MtttierGetCanonicalParentPositions(
    TierPosition tier_position, Tier parent_tier) {
    //
    Tier tier = tier_position.tier;
    Position position = tier_position.position;
    PositionArray parents;
    PositionArrayInit(&parents);
    if (parent_tier != tier - 1) return parents;

    PositionHashSet deduplication_set;
    PositionHashSetInit(&deduplication_set, 0.5);

    char board[9] = {0};
    GenericHashUnhashLabel(tier, position, board);

    char prev_turn = WhoseTurn(board) == 'X' ? 'O' : 'X';
    for (int i = 0; i < 9; ++i) {
        if (board[i] == prev_turn) {
            // Take piece off the board.
            board[i] = '-';
            TierPosition parent = {
                .tier = tier - 1,
                .position = GenericHashHashLabel(tier - 1, board, 1),
            };
            // Add piece back to the board.
            board[i] = prev_turn;
            if (!MtttierIsLegalPosition(parent)) {
                continue;  // Illegal.
            }
            parent.position = MtttierGetCanonicalPosition(parent);
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

static Position MninemensmorrisGetPositionInSymmetricTier(TierPosition tier_position, Tier symmetric) {
    if (tier_position.tier == symmetric) {
        return tier_position.position;
    } else { // only other situation is colors and turn are flipped
        
        return MninemensmorrisTierGetCanonicalPosition(tp);
    }
}

static TierArray MninemensmorrisGetChildTiers(Tier tier) {
    TierArray children;
    TierArrayInit(&children);
    int piecesLeft, numX, numO;
    UnhashTier(tier, &piecesLeft, &numX, &numO);
	if (piecesLeft > 0) { // Phase 1
		if (piecesLeft & 1) { // i.e., it's O's turn
            TierArrayAppend(
                &children, HashTier(piecesLeft - 1, numX, numO + 1));
			if (numO > 1) {
				TierArrayAppend(
                    &children, HashTier(piecesLeft - 1, numX - 1, numO + 1));
			}
		} else {
			TierArrayAppend(
                &children, HashTier(piecesLeft - 1, numX + 1, numO));
			if (numX > 1) {
				TierArrayAppend(
                    &children, HashTier(piecesLeft - 1, numX + 1, numO - 1));
			}
		}
	} else { // Phase 2 or 3. Turn unknown, so we assume both paths as children
        TierArrayAppend(&children, tier);
		if (numX > 2 && numO > 2) {
            TierArrayAppend(&children, HashTier(piecesLeft, numX - 1, numO));
			TierArrayAppend(&children, HashTier(piecesLeft, numX, numO - 1));
		}
	}
    return children;
}

static TierArray MtttierGetParentTiers(Tier tier) {
    TierArray children;
    TierArrayInit(&children);
    int piecesLeft, numX, numO;
    UnhashTier(tier, &piecesLeft, &numX, &numO);

	if (piecesLeft > 0) { // Phase 1
		if (piecesLeft & 1) { // i.e., it's O's turn
            TierArrayAppend(
                &children, HashTier(piecesLeft - 1, numX, numO + 1));
			if (numO > 1) {
				TierArrayAppend(
                    &children, HashTier(piecesLeft - 1, numX - 1, numO + 1));
			}
		} else {
			TierArrayAppend(
                &children, HashTier(piecesLeft - 1, numX + 1, numO));
			if (numX > 1) {
				TierArrayAppend(
                    &children, HashTier(piecesLeft - 1, numX + 1, numO - 1));
			}
		}
	} else { // Phase 2 or 3. Turn unknown, so we assume both paths as children
        TierArrayAppend(&children, tier);
		if (numX > 2 && numO > 2) {
            TierArrayAppend(&children, HashTier(piecesLeft, numX - 1, numO));
			TierArrayAppend(&children, HashTier(piecesLeft, numX, numO - 1));
		}
	}
    
    return children;
}

static Tier MninemensmorrisGetCanonicalTier(Tier tier) {
    int piecesLeft, numX, numO;
    UnhashTier(tier, piecesLeft, numX, numO);
    if (piecesLeft > 0) {
        return tier;
    } else {
        return HashTier(piecesLeft, numO, numX);
    }
}

static int MtttTierPositionToString(TierPosition tier_position, char *buffer) {
    // char board[9] = {0};
    // bool success = GenericHashUnhashLabel(tier_position.tier,
    //                                       tier_position.position, board);
    // if (!success) return 1;

    // for (int i = 0; i < 9; ++i) {
    //     board[i] = ConvertBlankToken(board[i]);
    // }

    // static ConstantReadOnlyString kFormat =
    //     "         ( 1 2 3 )           : %c %c %c\n"
    //     "LEGEND:  ( 4 5 6 )  TOTAL:   : %c %c %c\n"
    //     "         ( 7 8 9 )           : %c %c %c";
    // int actual_length =
    //     snprintf(buffer, kGameplayApi.position_string_length_max + 1, kFormat,
    //              board[0], board[1], board[2], board[3], board[4], board[5],
    //              board[6], board[7], board[8]);
    // if (actual_length >= kGameplayApi.position_string_length_max + 1) {
    //     fprintf(
    //         stderr,
    //         "MtttierTierPositionToString: (BUG) not enough space was allocated "
    //         "to buffer. Please increase position_string_length_max.\n");
    //     return 1;
    // }
    // return 0;

    // char turn;
	// int piecesLeft;
	// int numx, numo;

	// char* board = unhash(position, &turn, &piecesLeft, &numx, &numo);

	// if (gameType==6) {

	// 	printf("\n");
	// 	printf("          0 ----- 1 ----- 2    %c ----- %c ----- %c    %s's turn (%c)\n", board[0], board[1], board[2], playersName, turn);
	// 	printf("          |       |       |    |       |       |    \n");
	// 	printf("          |   3 - 4 - 5   |    |   %c - %c - %c   |    Phase: ", board[3], board[4], board[5]);
	// 	if (piecesLeft != 0)
	// 		printf("1 : PLACING\n");
	// 	else {
	// 		if  (!gFlying || ((turn == X) && (numx > 3)) || ((turn == O) && (numo > 3)))
	// 			printf("2 : SLIDING\n");
	// 		else
	// 			printf("3 : FLYING\n");
	// 	}
	// 	printf("          |   |       |   |    |   |       |   |    ");
	// 	if (piecesLeft != 0)
	// 		printf("X has %d left to place\n",piecesLeft/2);
	// 	else
	// 		printf("X has %d on the board\n", numx);
	// 	printf("LEGEND:   6 - 7       8 - 9    %c - %c       %c - %c    ", board[6], board[7], board[8], board[9]);
	// 	if (piecesLeft != 0)
	// 		printf("O has %d left to place\n",piecesLeft/2 + piecesLeft%2);
	// 	else
	// 		printf("O has %d on the board\n", numo);
	// 	printf("          |   |       |   |    |   |       |   |    \n");
	// 	printf("          |  10 - 11- 12  |    |   %c - %c - %c   |  \n", board[10], board[11], board[12] );
	// 	printf("          |       |       |    |       |       |     \n");
	// 	printf("          13 ---- 14 ---- 15   %c ----- %c ----- %c    %s\n\n", board[13], board[14], board[15],
	// 	       GetPrediction(position,playersName,usersTurn));

	// } else {
	// 	printf("\n");
	// 	printf("        0 --------- 1 --------- 2       %c --------- %c --------- %c    %s's turn (%c)\n", board[0], board[1], board[2], playersName, turn );
	// 	printf("        |           |           |       |           |           |\n");
	// 	printf("        |   3 ----- 4 ----- 5   |       |   %c ----- %c ----- %c   |    Phase: ", board[3], board[4], board[5]);
	// 	if (piecesLeft != 0)
	// 		printf("1 : PLACING\n");
	// 	else {
	// 		if  (!gFlying || ((turn == X) && (numx > 3)) || ((turn == O) && (numo > 3)))
	// 			printf("2 : SLIDING\n");
	// 		else
	// 			printf("3 : FLYING\n");
	// 	}
	// 	printf("        |   |       |       |   |       |   |       |       |   |    ");
	// 	if (piecesLeft != 0)
	// 		printf("X has %d left to place\n",piecesLeft/2);
	// 	else
	// 		printf("X has %d on the board\n", numx);
	// 	printf("        |   |   6 - 7 - 8   |   |       |   |   %c - %c - %c   |   |    ", board[6], board[7], board[8] );
	// 	if (piecesLeft != 0)
	// 		printf("O has %d left to place\n",piecesLeft/2 + piecesLeft%2);
	// 	else
	// 		printf("O has %d on the board\n", numo);
	// 	printf("        |   |   |       |   |   |       |   |   |       |   |   |\n");
	// 	printf("LEGEND: 9 - 10- 11      12- 13- 14      %c - %c - %c       %c - %c - %c\n", board[9], board[10], board[11], board[12], board[13], board[14]);
	// 	printf("        |   |   |       |   |   |       |   |   |       |   |   |\n");
	// 	printf("        |   |   15- 16- 17  |   |       |   |   %c - %c - %c   |   |\n", board[15], board[16], board[17] );
	// 	printf("        |   |       |       |   |       |   |       |       |   |\n");
	// 	printf("        |   18 ---- 19 ---- 20  |       |   %c ----- %c ----- %c   |\n", board[18], board[19], board[20] );
	// 	printf("        |           |           |       |           |           |\n");
	// 	printf("        21 -------- 22 -------- 23      %c --------- %c --------- %c    %s\n\n", board[21], board[22], board[23], GetPrediction(position, playersName, usersTurn));

	// }
    return 0;
}

static int MninemensmorrisMoveToString(Move move, char *buffer) {
    // TODO: What's up with PRIDnt
    int actualLength;
    int from, to, remove;
    unhashMove(move, &from, &to, &remove);
    int maxStrLen = kGameplayApi.move_string_length_max + 1;

	if (from != NONE && to != NONE && remove != NONE) {
		actualLength = snprintf(buffer, maxStrLen, "%d-%dr%d", from, to, remove);
	} else if (from != NONE && to != NONE && remove == NONE) {
		actualLength = snprintf(buffer, maxStrLen, "%d-%d", from, to);
	} else if (from == NONE && to != NONE && remove == NONE) {
		actualLength = snprintf(buffer, maxStrLen, "%d", to);
	} else {
		actualLength = snprintf(buffer, maxStrLen, "%dr%d", to, remove);
	}

    if (actualLength >= maxStrLen) {
        fprintf(stderr,
                "MninemensmorrisMoveToString: (BUG) not enough space was allocated "
                "to buffer. Please increase move_string_length_max.\n");
        return 1;
    }
    return 0;
}

static bool MninemensmorrisIsValidMoveString(ReadOnlyString moveString) {
    int maxSlots = boardSize - 1;
	int i = 0;
	bool existsR = false;
	bool existsDash = false;
	int currNum = 0;
	while (moveString[i] != '\0') {
		if (!((moveString[i] >= '0' && moveString[i] <= '9') || moveString[i] == '-' || moveString[i] == 'r')) {
			return false;
		}
		if (moveString[i] == '-') {
			if (existsDash || existsR) {
				return false;
			} else {
				existsDash = true;
				currNum = 0;
			}
		} else if (moveString[i] == 'r') {
			if (existsR) {
				return false;
			} else {
				existsR = true;
				currNum = 0;			
			}
		} else {
			currNum = currNum * 10 + (moveString[i] - '0');
			if (currNum > maxSlots) {
				return false;
			}
		}
		i++;
	}
	return true;
}

static Move MninemensmorrisStringToMove(ReadOnlyString moveString) {
    assert(MninemensmorrisIsValidMoveString(moveString));
    int from = NONE;
	int to = NONE; //that way if no input for remove, it's equal to from. useful in function DoMove
	int remove = NONE; // for stage 1, if there is nothing to remove

	int i = 0;
	int phase = 0;
	int first = 0;
	int second = 0;
	int third = 0;
	bool existsSliding = false;
	while (moveString[i] != '\0') {
		if (moveString[i] == 'r' || moveString[i] == '-') {
			if (moveString[i] == '-') {
				existsSliding = true;
			}
			phase++;
			i++;
			continue;
		}
		if (phase == 0) {
			first = first * 10 + (moveString[i] - '0');
		} else if (phase == 1) {
			second = second * 10 + (moveString[i] - '0');
		} else {
			third = third * 10 + (moveString[i] - '0');
		}
		i++;
	}

	if (phase == 0) { // Placement without removal
		to = first;
	} else if (phase == 2) { // Sliding/flying with removal
		from = first;
		to = second;
		remove = third;
	} else if (existsSliding) { // Sliding/flying without removal
		from = first;
		to = second;
	} else { // Placement with removal.
		to = first;
		remove = second;
	}
	return MOVE_ENCODE(from, to, remove); //HASHES THE MOVE
}

// Helper functions implementation

static Tier HashTier(int piecesLeft, int numX, int numO) {
    return (piecesLeft << 4) | (numX << 2) | numO;
}

static void UnhashTier(Tier tier, int *piecesLeft, int *numX, int *numO) {
    *piecesLeft = tier >> 4;
    *numX = (tier >> 2) & 0b11;
    *numO = tier & 0b11;
}

static void UnhashMove(Move move, int *from, int *to, int *remove) {
    *from = move >> 10;
	*to = (move >> 5) & 0x1F;
	*remove = move & 0x1F;
}

static bool InitGenericHash(void) {
    GenericHashReinitialize();
    int player;  // No turn bit needed as we can infer the turn from board.
    int pieces_init_array[10] = {'X', 0, 0, 'O', 0, 0, '-', 0, 0, -1};
    bool success;
    Tier tier;

    for (int piecesLeft = 0; piecesLeft < 2 * gNumPiecesPerPlayer + 1; piecesLeft++) {
		for (int numX = 0; numX < gNumPiecesPerPlayer + 1; numX++) {
			for (int numO = 0; numO < gNumPiecesPerPlayer + 1; numO++) {
				tier = HashTier(piecesLeft, numX, numO);
				pieces_init_array[1] = pieces_init_array[2] = numX;
				pieces_init_array[4] = pieces_init_array[5] = numO;
				pieces_init_array[7] = pieces_init_array[8] = boardSize - numX - numO;
                // Odd piecesLeft means it's P1's turn. Nonzero even piecesLeft means it's P2's turn.
                player = (piecesLeft == 0) ? 0 : (piecesLeft & 1) ? 2 : 1;

                success = GenericHashAddContext(player, boardSize, pieces_init_array, NULL, tier);

                if (!success) {
                    fprintf(stderr,
                            "MninemensmorrisInit: failed to initialize generic hash context "
                            "for tier %" PRId64 ". Aborting...\n",
                            tier);
                    GenericHashReinitialize();
                    return false;
                }
			}
		}
	}

    return true;
}

/*
Returns true if mill would be created if current player
places a piece at toIdx (if fromIdx = 31) or slides a piece from fromIdx to toIdx (otherwise).
*/
bool closesMill(char *board, char turn, int from, int to) {
	char copy[boardSize];
	memcpy(copy, board, sizeof(copy));
	if (from != 31) copy[from] = BLANK; // If sliding.
	return checkMill(copy, to, turn);
}

int findLegalRemoves(char *board, char turn, int *legalRemoves) {
	int numLegalRemoves = 0;
	char oppTurn = turn == X ? O : X;
	if (removalRule == 0) { /* Standard. Removable if piece is not in a mill or all of opponent's pieces are in mills. */
		for (int i = 0; i < boardSize; i++)
			if (board[i] == oppTurn && (!checkMill(board, i, oppTurn) || all_mills(board, i, oppTurn)))
				legalRemoves[numLegalRemoves++] = i;
	} else if (removalRule == 1) { /* Any of opponent's pieces are removable. */
		for (int i = 0; i < boardSize; i++)
			if (board[i] == oppTurn)
				legalRemoves[numLegalRemoves++] = i;
	} else { /* Removable only if piece is not in mill. */
		for (int i = 0; i < boardSize; i++)
			if (board[i] == oppTurn && !checkMill(board, i, oppTurn))
				legalRemoves[numLegalRemoves++] = i;
	}
	return numLegalRemoves;
}

static Position DoSymmetry(TierPosition tier_position, int symmetry) {
    char board[9] = {0}, symmetry_board[9] = {0};
    GenericHashUnhashLabel(tier_position.tier, tier_position.position, board);

    // Copy from the symmetry matrix.
    // for (int i = 0; i < 9; ++i) {
    //     symmetry_board[i] = board[kSymmetryMatrix[symmetry][i]];
    // }

    return GenericHashHashLabel(tier_position.tier, symmetry_board, 1);
}