/**
 * @file mninemensmorris.c
 * @author Patricia Fong, Kevin Liu, Erwin A. Vedar, Wei Tu, Elmer Lee,
 * Cameron Cheung: developed the first version in GamesmanClassic (m369mm.c).
 * @author Cameron Cheung (cameroncheung@berkeley.edu): Added to system.
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

/* Game, Solver, and Gameplay API Functions */

static int MninemensmorrisInit(void *aux);
static int MninemensmorrisFinalize(void);

static const GameVariant *MninemensmorrisGetCurrentVariant(void);
static int MninemensmorrisSetVariantOption(int option, int selection);

static Tier MninemensmorrisGetInitialTier(void);
static Position MninemensmorrisGetInitialPosition(void);

static int64_t MninemensmorrisGetTierSize(Tier tier);
static MoveArray MninemensmorrisGenerateMoves(TierPosition tier_position);
static Value MninemensmorrisPrimitive(TierPosition tier_position);
static TierPosition MninemensmorrisDoMove(TierPosition tier_position,
                                          Move move);
static bool MninemensmorrisIsLegalPosition(TierPosition tier_position);
static Position MninemensmorrisGetCanonicalPosition(TierPosition tier_position);
static TierPositionArray MninemensmorrisGetCanonicalChildPositions(
    TierPosition tier_position);
static PositionArray MninemensmorrisGetCanonicalParentPositions(
    TierPosition tier_position);
static TierArray MninemensmorrisGetChildTiers(Tier tier);
static TierArray MninemensmorrisGetParentTiers(Tier tier);
static Tier MninemensmorrisGetCanonicalTier(Tier tier);
static Position MninemensmorrisGetPositionInSymmetricTier(
    TierPosition tier_position, Tier symmetric);

static int MninemensmorrisTierPositionToString(TierPosition tier_position,
                                               char *buffer);
static int MninemensmorrisMoveToString(Move move, char *buffer);
static bool MninemensmorrisIsValidMoveString(ReadOnlyString move_string);
static Move MninemensmorrisStringToMove(ReadOnlyString move_string);

/* Solver API Setup */
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

/* Gameplay API Setup */
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

/* Defines */
#define MOVE_ENCODE(from, to, remove) ((from << 10) | (to << 5) | remove)
#define MILL(board, slot1, slot2, player) \
    (board[slot1] == player && board[slot2] == player)
#define NONE 0b11111
#define X 'X'
#define O 'O'
#define BLANK '-'

/**************************** ADJACENCY ARRAYS ********************************
 * Adjacencies: Each slot has at most 4 adjacent slots. The last number
 * indicates the number of adjacencies. For example, on the 24-Board, to get
 * the adjacencies for slot 6, we see that at adjacent24[6],
 * the array is {7, 11, -1, -1, 2}. That means that slot 6 has
 * two adjacent interesction slots on the 24Board, 7 and 11.
 *****************************************************************************/

static const int adjacent16[16][5] = {
    {1, 6, -1, -1, 2},  {0, 2, 4, -1, 3},   {1, 9, -1, -1, 2},
    {4, 7, -1, -1, 2},  {1, 3, 5, -1, 3},   {4, 8, -1, -1, 2},
    {0, 7, 13, -1, 3},  {3, 6, 10, -1, 3},  {5, 9, 12, -1, 3},
    {2, 8, 15, -1, 3},  {7, 11, -1, -1, 2}, {10, 12, 14, -1, 3},
    {8, 11, -1, -1, 2}, {6, 14, -1, -1, 2}, {11, 13, 15, -1, 3},
    {9, 14, -1, -1, 2}};

static const int adjacent24[24][5] = {
    {1, 9, -1, -1, 2},   {0, 2, 4, -1, 3},    {1, 14, -1, -1, 2},
    {4, 10, -1, -1, 2},  {1, 3, 5, 7, 4},     {4, 13, -1, -1, 2},
    {7, 11, -1, -1, 2},  {4, 6, 8, -1, 3},    {7, 12, -1, -1, 2},
    {0, 10, 21, -1, 3},  {3, 9, 11, 18, 4},   {6, 10, 15, -1, 3},
    {8, 13, 17, -1, 3},  {5, 12, 14, 20, 4},  {2, 13, 23, -1, 3},
    {11, 16, -1, -1, 2}, {15, 17, 19, -1, 3}, {12, 16, -1, -1, 2},
    {10, 19, -1, -1, 2}, {16, 18, 20, 22, 4}, {13, 19, -1, -1, 2},
    {9, 22, -1, -1, 2},  {19, 21, 23, -1, 3}, {14, 22, -1, -1, 2}};

static const int adjacent24Plus[24][5] = {
    {1, 3, 9, -1, 3},    {0, 2, 4, -1, 3},    {1, 5, 14, -1, 3},
    {0, 4, 6, 10, 4},    {1, 3, 5, 7, 4},     {2, 4, 8, 13, 4},
    {3, 7, 11, -1, 3},   {4, 6, 8, -1, 3},    {5, 7, 12, -1, 3},
    {0, 10, 21, -1, 3},  {3, 9, 11, 18, 4},   {6, 10, 15, -1, 3},
    {8, 13, 17, -1, 3},  {5, 12, 14, 20, 4},  {2, 13, 23, -1, 3},
    {11, 16, 18, -1, 3}, {15, 17, 19, -1, 3}, {12, 16, 20, -1, 3},
    {10, 15, 19, 21, 4}, {16, 18, 20, 22, 4}, {13, 17, 19, 23, 4},
    {9, 18, 22, -1, 3},  {19, 21, 23, -1, 3}, {14, 20, 22, -1, 3}};

/****************************** LINES ARRAYS **********************************
 * Lines are three-in-a-rows of board slots that we check to see if there is
 * a mill along them. The lines are indexed by slot.
 * Suppose we are interested in the lines that slot 5 is a part of on the
 * 16-Board. Let arr = linesArray16[5] = {  3,  4,  8, 12, -1, -1, 2}.
 * The last element of the array is the number of lines that slot 5 is a part
 * of. Slot 5 is a part of a line also containing 3 and 4 and a line also
 * containing 8 and 12.
 * linesArray24 is for 24-Board and 24-Board-Plus, but has the lineCounts of
 * 24-Board-Plus. The lines for linesArray24 are listed in the order: two non-
 * diagonal lines first, then, if the slot is part of one, the diagonal line.
 *****************************************************************************/

static const int linesArray16[16][7] = {
    {1, 2, 6, 13, -1, -1, 2},    {0, 2, -1, -1, -1, -1, 1},
    {0, 1, 9, 15, -1, -1, 2},    {4, 5, 7, 10, -1, -1, 2},
    {3, 5, -1, -1, -1, -1, 1},   {3, 4, 8, 12, -1, -1, 2},
    {0, 13, -1, -1, -1, -1, 1},  {3, 10, -1, -1, -1, -1, 1},
    {5, 12, -1, -1, -1, -1, 1},  {2, 15, -1, -1, -1, -1, 1},
    {3, 7, 11, 12, -1, -1, 2},   {10, 12, -1, -1, -1, -1, 1},
    {5, 8, 10, 11, -1, -1, 2},   {0, 6, 14, 15, -1, -1, 2},
    {13, 15, -1, -1, -1, -1, 1}, {2, 9, 13, 14, -1, -1, 2}};

static const int linesArray24[16][7] = {
    {1, 2, 9, 21, 3, 6, 3},      {0, 2, 4, 7, -1, -1, 2},
    {0, 1, 14, 23, 5, 8, 3},     {4, 5, 10, 18, 0, 6, 3},
    {1, 7, 3, 5, -1, -1, 2},     {3, 4, 13, 20, 2, 8, 3},
    {7, 8, 11, 15, 0, 3, 3},     {1, 4, 6, 8, -1, -1, 2},
    {6, 7, 12, 17, 2, 5, 3},     {0, 21, 10, 11, -1, -1, 2},
    {3, 18, 9, 11, -1, -1, 2},   {6, 15, 9, 10, -1, -1, 2},
    {8, 17, 13, 14, -1, -1, 2},  {5, 20, 12, 14, -1, -1, 2},
    {2, 23, 12, 13, -1, -1, 2},  {6, 11, 16, 17, 18, 21, 3},
    {15, 17, 19, 22, -1, -1, 2}, {8, 12, 15, 16, 20, 23, 3},
    {3, 10, 19, 20, 15, 21, 3},  {16, 22, 18, 20, -1, -1, 2},
    {5, 13, 18, 19, 17, 23, 3},  {0, 9, 22, 23, 15, 18, 3},
    {16, 19, 21, 23, -1, -1, 2}, {2, 14, 21, 22, 17, 20, 3}};

/************************** VARIANTS EXPLANATION ******************************
 *
 * OPTION: Misere
 *
 * If playing Misere, the objective is to be left with no legal
 * moves or to have your own piece count reduced to 2.
 *
 * ----------------------------------------------------------------------------
 *
 * OPTION: FlyRule
 *
 * If Fly Rule is True, then a player when left with three pieces (the total
 * pieces they have on the board and yet to place is 3), can move one of
 * their pieces on the board to ANY slot, rather than just to adjacent slots.
 *
 * ----------------------------------------------------------------------------
 *
 * OPTION: LaskerRule
 *
 * If using the Lasker Rule, then if you can choose to slide/fly a piece that
 * you have on the board even if you have not placed all of your
 * pieces on the board.
 *
 * ----------------------------------------------------------------------------
 *
 * OPTION: RemovalRule
 *
 * Defines which of your opponent's pieces you can remove if you form a mill.
 *
 * 0) If using STANDARD rules, a piece can be removed as long as it is in a
 * mill, unless all of the opponent's pieces are in a mill, in which case
 * any piece can be removed (note that there is always at least one piece
 * that can be removed).
 *
 * 1) If using LENIENT rules, then a piece can be removed regardless
 * of whether it is in a mill or not.
 *
 * 2) If using STRICT rules, then a piece can only be removed if it is in a
 * mill and it does not matter if all the pieces are in a mill. If a player
 * creates a mill and all the opponent's pieces are in mills,
 * then no piece is removed.
 *
 * ----------------------------------------------------------------------------
 *
 * OPTION: BoardType
 *
 * There are three types of boards supported.
 *
 *    16-Board                 24-Board                   24-Board-Plus
 *
 *  0-----1-----2     0 --------- 1 --------- 2     0 --------- 1 --------- 2
 *  |     |     |     |           |           |     | \         |         / |
 *  |  3--4--5  |     |   3 ----- 4 ----- 5   |     |   3 ----- 4 ----- 5   |
 *  |  |     |  |     |   |       |       |   |     |   | \     |     / |   |
 *  6--7     8--9     |   |   6 - 7 - 8   |   |     |   |   6 - 7 - 8   |   |
 *  |  |     |  |     |   |   |       |   |   |     |   |   |       |   |   |
 *  | 10-11-12  |     9 - 10- 11      12- 13- 14    9 - 10- 11      12- 13- 14
 *  |     |     |     |   |   |       |   |   |     |   |   |       |   |   |
 *  13---14----15     |   |   15- 16- 17  |   |     |   |   15- 16- 17  |   |
 *                    |   |       |       |   |     |   | /     |     \ |   |
 *                    |   18 ---- 19 ---- 20  |     |   18 ---- 19 ---- 20  |
 *                    |           |           |     | /         |         \ |
 *                    21 -------- 22 -------- 23    21 -------- 22 -------- 23
 *
 * The 16-Board is used for standard 6 Men's Morris. The 24-Board is used
 * for standard 9 Men's Morris. The 24-Board-Plus has added diagonals (along
 * which mills can be created) and is used for standard 12 Men's Morris.
 *
 * ----------------------------------------------------------------------------
 *
 * OPTION: NumPiecesPerPlayer
 *
 * Defines how many pieces total each player can place on the board over the
 * course of the game. This ranges from 3 to 12 pieces; however, if playing
 * on the 16-Board, then each player may have no more than 8 pieces each.
 *
 *****************************************************************************/

static ConstantReadOnlyString choicesMisere[2] = {"False", "True"};
static const GameVariantOption optionMisere = {
    .name = "Misere", .num_choices = 2, .choices = choicesMisere};

static ConstantReadOnlyString choicesFlyRule[2] = {"False", "True"};
static const GameVariantOption optionFlyRule = {
    .name = "Fly When Left With Three Pieces",
    .num_choices = 2,
    .choices = choicesFlyRule};

static ConstantReadOnlyString choicesLaskerRule[2] = {"False", "True"};
static const GameVariantOption optionLaskerRule = {
    .name = "Lasker Rule Enabled",
    .num_choices = 2,
    .choices = choicesLaskerRule};

static ConstantReadOnlyString choicesRemovalRule[2] = {"Standard", "Lenient",
                                                       "Strict"};
const struct GameVariantOption optionRemovalRule = {
    .name = "Removal Rule", .num_choices = 3, .choices = choicesRemovalRule};

static ConstantReadOnlyString choicesBoardType[2] = {"16-Board", "24-Board",
                                                     "24-Board-Plus"};
static const GameVariantOption optionBoardType = {
    .name = "Board Type", .num_choices = 3, .choices = choicesBoardType};

static ConstantReadOnlyString choicesNumPiecesPerPlayer[2] = {
    "3", "4", "5", "6", "7", "8", "9", "10", "11", "12"};
const struct GameVariantOption optionNumPiecesPerPlayer = {
    .name = "Number of Pieces Per Player",
    .num_choices = 10,
    .choices = choicesNumPiecesPerPlayer};

static int numOptions = 6;
static GameVariantOption variantOptions[7];  // 6 options and 1 zero-terminator
static int currentSelections[7];

/***************************** SYMMETRY ARRAYS ********************************
 * Here are the permutations of the slots of the boards that make for symmetric
 * positions. Note that the arrays here only address rotation, reflection,
 * and inner-outer flip.
 * Color symmetries are handled in GetCanonicalPosition.
 * There is an additional symmetry for the
 * (xToPlace=0, oToPlace=0, xOnBoard=3, oOnBoard=3) tier for the 24-Boards
 * where any permutation of the 3 rings is symmetric. We do not implement
 * this because it does relatively little to compress the database.
 *****************************************************************************/

static const int numBoardSymmetries16 = 16;
static const int numBoardSymmetries24 = 16;

static const int symmetries16[16][24] = {
    {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11,
     12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1},
    {13, 6,  0, 10, 7,  3,  14, 11, 4,  1,  12, 8,
     5,  15, 9, 2,  -1, -1, -1, -1, -1, -1, -1, -1},
    {15, 14, 13, 12, 11, 10, 9,  8,  7,  6,  5,  4,
     3,  2,  1,  0,  -1, -1, -1, -1, -1, -1, -1, -1},
    {2,  9, 15, 5,  8,  12, 1,  4,  11, 14, 3,  7,
     10, 0, 6,  13, -1, -1, -1, -1, -1, -1, -1, -1},
    {3,  4,  5,  0,  1,  2,  7,  6,  9,  8,  13, 14,
     15, 10, 11, 12, -1, -1, -1, -1, -1, -1, -1, -1},
    {10, 7,  3, 13, 6,  0,  11, 14, 1,  4,  15, 9,
     2,  12, 8, 5,  -1, -1, -1, -1, -1, -1, -1, -1},
    {12, 11, 10, 15, 14, 13, 8,  9,  6,  7,  2,  1,
     0,  5,  4,  3,  -1, -1, -1, -1, -1, -1, -1, -1},
    {5,  8, 12, 2,  9,  15, 4,  1,  14, 11, 0,  6,
     13, 3, 7,  10, -1, -1, -1, -1, -1, -1, -1, -1},
    {2,  1,  0,  5,  4,  3,  9,  8,  7,  6,  12, 11,
     10, 15, 14, 13, -1, -1, -1, -1, -1, -1, -1, -1},
    {0,  6, 13, 3,  7,  10, 1,  4,  11, 14, 5,  8,
     12, 2, 9,  15, -1, -1, -1, -1, -1, -1, -1, -1},
    {13, 14, 15, 10, 11, 12, 6,  7,  8,  9,  3,  4,
     5,  0,  1,  2,  -1, -1, -1, -1, -1, -1, -1, -1},
    {15, 9,  2, 12, 8,  5,  14, 11, 4,  1,  10, 7,
     3,  13, 6, 0,  -1, -1, -1, -1, -1, -1, -1, -1},
    {5,  4,  3,  2,  1,  0,  8,  9,  6,  7,  15, 14,
     13, 12, 11, 10, -1, -1, -1, -1, -1, -1, -1, -1},
    {3,  7, 10, 0,  6,  13, 4,  1,  14, 11, 2,  9,
     15, 5, 8,  12, -1, -1, -1, -1, -1, -1, -1, -1},
    {10, 11, 12, 13, 14, 15, 7,  6,  9,  8,  0,  1,
     2,  3,  4,  5,  -1, -1, -1, -1, -1, -1, -1, -1},
    {12, 8,  5, 15, 9,  2,  11, 14, 1,  4,  13, 6,
     0,  10, 7, 3,  -1, -1, -1, -1, -1, -1, -1, -1}};

static const int symmetries24[16][24] = {
    {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11,
     12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23},
    {6,  7,  8,  3,  4,  5,  0,  1,  2,  11, 10, 9,
     14, 13, 12, 21, 22, 23, 18, 19, 20, 15, 16, 17},
    {2,  1,  0, 5,  4,  3,  8,  7,  6,  14, 13, 12,
     11, 10, 9, 17, 16, 15, 20, 19, 18, 23, 22, 21},
    {8, 7,  6,  5,  4,  3,  2,  1,  0,  12, 13, 14,
     9, 10, 11, 23, 22, 21, 20, 19, 18, 17, 16, 15},
    {21, 22, 23, 18, 19, 20, 15, 16, 17, 9, 10, 11,
     12, 13, 14, 6,  7,  8,  3,  4,  5,  0, 1,  2},
    {15, 16, 17, 18, 19, 20, 21, 22, 23, 11, 10, 9,
     14, 13, 12, 0,  1,  2,  3,  4,  5,  6,  7,  8},
    {23, 14, 2, 20, 13, 5, 17, 12, 8, 22, 19, 16,
     7,  4,  1, 15, 11, 6, 18, 10, 3, 21, 9,  0},
    {17, 12, 8, 20, 13, 5, 23, 14, 2, 16, 19, 22,
     1,  4,  7, 21, 9,  0, 18, 10, 3, 15, 11, 6},
    {0,  9,  21, 3, 10, 18, 6, 11, 15, 1, 4,  7,
     16, 19, 22, 8, 12, 17, 5, 13, 20, 2, 14, 23},
    {6,  11, 15, 3, 10, 18, 0, 9,  21, 7, 4,  1,
     22, 19, 16, 2, 14, 23, 5, 13, 20, 8, 12, 17},
    {21, 9, 0, 18, 10, 3, 15, 11, 6, 22, 19, 16,
     7,  4, 1, 17, 12, 8, 20, 13, 5, 23, 14, 2},
    {15, 11, 6, 18, 10, 3, 21, 9,  0, 16, 19, 22,
     1,  4,  7, 23, 14, 2, 20, 13, 5, 17, 12, 8},
    {23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12,
     11, 10, 9,  8,  7,  6,  5,  4,  3,  2,  1,  0},
    {17, 16, 15, 20, 19, 18, 23, 22, 21, 12, 13, 14,
     9,  10, 11, 2,  1,  0,  5,  4,  3,  8,  7,  6},
    {2,  14, 23, 5, 13, 20, 8, 12, 17, 1, 4, 7,
     16, 19, 22, 6, 11, 15, 3, 10, 18, 0, 9, 21},
    {8,  12, 17, 5, 13, 20, 2, 14, 23, 7, 4,  1,
     22, 19, 16, 0, 9,  21, 3, 10, 18, 6, 11, 15}};

/* Variant-Related Global Variables (can be set in SetVariantOption) */
bool isMisere = false;
bool gFlyRule = true;
bool laskerRule = false;
int removalRule = 0;
int boardType = 1;
int gNumPiecesPerPlayer = 9;
int boardSize = 24;                    // Tied to boardType
int (*adjacent)[5] = adjacent24;       // Tied to boardType
int (*linesArray)[7] = linesArray24;   // Tied to boardType
int (*symmetries)[24] = symmetries24;  // Tied to boardType
int totalNumBoardSymmetries = numBoardSymmetries24;

/* Helper Functions */
static Tier HashTier(int xToPlace, int oToPlace, int xOnBoard, int oOnBoard);
static void UnhashTier(Tier tier, int *xToPlace, int *oToPlace, int *xOnBoard,
                       int *oOnBoard);
static void UnhashMove(Move move, int *from, int *to, int *remove);
static bool IsSlotInMill(char *board, int slot, char player);
static bool ClosesMill(char *board, char player, int from, int to);
static bool AllPiecesInMills(char *board, char player);
static int FindLegalRemoves(char *board, char player, int *legalRemoves);
static bool InitGenericHash(void);

/* Game, Solver, and Gameplay API Functions Implementation */

static int MninemensmorrisInit(void *aux) {
    (void)aux;  // Unused.

    memset(&variantOptions, 0, sizeof(variantOptions));
    variantOptions[0] = optionMisere;
    variantOptions[1] = optionFlyRule;
    variantOptions[2] = optionLaskerRule;
    variantOptions[3] = optionRemovalRule;
    variantOptions[4] = optionBoardType;
    variantOptions[5] = optionNumPiecesPerPlayer;

    memset(&currentSelections, 0, sizeof(currentSelections));

    return !InitGenericHash();
}

static int MninemensmorrisFinalize(void) { return 0; }

static const GameVariant *MninemensmorrisGetCurrentVariant(void) {
    return NULL;
}

static int MninemensmorrisSetVariantOption(int option, int selection) {
    if (option >= numOptions) {
        fprintf(stderr,
                "MninemensmorrisSetVariantOption: (BUG) option index out of "
                "bounds. "
                "Aborting...\n");
        return 1;
    }
    if (selection < 0 || selection > 1) {
        fprintf(stderr,
                "MninemensmorrisSetVariantOption: (BUG) selection index out of "
                "bounds. "
                "Aborting...\n");
        return 1;
    }
    switch (option) {
        case 0:  // Misere
            isMisere = selection == 1;
            break;
        case 1:  // Flying Rule
            gFlyRule = selection == 1;
            break;
        case 3:  // Lasker Rule
            laskerRule = selection == 1;
        case 4:  // Removal Rule
            removalRule = selection;
            break;
        case 5:  // BoardType
            boardType = selection;
            if (selection == 0) {
                boardSize = 16;
                adjacent = adjacent16;
                linesArray = linesArray16;
                totalNumBoardSymmetries = numBoardSymmetries16;
                symmetries = symmetries16;
            } else if (selection == 1) {
                boardSize = 24;
                adjacent = adjacent24;
                linesArray = linesArray24;
                totalNumBoardSymmetries = numBoardSymmetries24;
                symmetries = symmetries24;
            } else {
                boardSize = 24;
                adjacent = adjacent24Plus;
                linesArray = linesArray24;
                totalNumBoardSymmetries = numBoardSymmetries24;
                symmetries = symmetries24;
            }
            break;
        case 6:  // NumPiecesPerPlayer
            gNumPiecesPerPlayer = selection - 3;
            break;
        default:
            return 1;
    }
    currentSelections[option] = selection;
    return 0;
}

static Tier MninemensmorrisGetInitialTier(void) {
    return HashTier(gNumPiecesPerPlayer, gNumPiecesPerPlayer, 0, 0);
}

/* Assumes Generic Hash has been initialized. */
static Position MninemensmorrisGetInitialPosition(void) {
    char board[boardSize];
    memset(board, BLANK, boardSize);
    return GenericHashHashLabel(
        HashTier(gNumPiecesPerPlayer, gNumPiecesPerPlayer, 0, 0), board, 1);
}

static int64_t MninemensmorrisGetTierSize(Tier tier) {
    return GenericHashNumPositionsLabel(tier);
}

static MoveArray MninemensmorrisGenerateMoves(TierPosition tier_position) {
    MoveArray moves;
    MoveArrayInit(&moves);

    char board[boardSize];
    GenericHashUnhashLabel(tier_position.tier, tier_position.position, board);
    int turn =
        GenericHashGetTurnLabel(tier_position.tier, tier_position.position);
    char player = (turn == 1) ? X : O;
    int xToPlace, oToPlace, xOnBoard, oOnBoard;
    UnhashTier(tier_position.tier, &xToPlace, &oToPlace, &xOnBoard, &oOnBoard);

    int totalX = xToPlace + xOnBoard;
    int totalO = oToPlace + oOnBoard;

    if (totalX > 2 && totalO > 2) {
        int toPlace = (player == X) ? xToPlace : oToPlace;
        int legalRemoves[boardSize];
        int numLegalRemoves = FindLegalRemoves(board, player, legalRemoves);
        int i, j, from, numLegalTos, *legalTos;

        int allBlanks[boardSize];
        int numBlanks = 0;
        for (i = 0; i < boardSize; i++) {
            if (board[i] == BLANK) {
                allBlanks[numBlanks++] = i;
            }
        }

        if (toPlace == 0 || laskerRule) {  // Sliding or flying moves
            for (from = 0; from < boardSize; from++) {
                if (gFlyRule && ((player == X && totalX == 3) ||
                                 (player == O && totalO == 3))) {
                    legalTos = allBlanks;
                    numLegalTos = numBlanks;
                } else {
                    legalTos = adjacent[from];
                    numLegalTos = adjacent[from][4];
                }
                if (board[from] == player) {
                    for (i = 0; i < numLegalTos; i++) {
                        if (board[legalTos[i]] == BLANK) {
                            if (ClosesMill(board, player, from, legalTos[i]) &&
                                numLegalRemoves > 0) {
                                for (j = 0; j < numLegalRemoves; j++) {
                                    MoveArrayAppend(
                                        &moves, MOVE_ENCODE(from, legalTos[i],
                                                            legalRemoves[j]));
                                }
                            } else {
                                MoveArrayAppend(
                                    &moves,
                                    MOVE_ENCODE(from, legalTos[i], NONE));
                            }
                        }
                    }
                }
            }
        }

        if (toPlace > 0) {  // Placing moves
            for (i = 0; i < numBlanks; i++) {
                if (ClosesMill(board, player, NONE, allBlanks[i]) &&
                    numLegalRemoves > 0) {
                    for (j = 0; j < numLegalRemoves; j++) {
                        MoveArrayAppend(&moves, MOVE_ENCODE(NONE, allBlanks[i],
                                                            legalRemoves[j]));
                    }
                } else {
                    MoveArrayAppend(&moves,
                                    MOVE_ENCODE(NONE, allBlanks[i], NONE));
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

static TierPosition MninemensmorrisDoMove(TierPosition tier_position,
                                          Move move) {
    char board[boardSize];
    GenericHashUnhashLabel(tier_position.tier, tier_position.position, board);
    int turn =
        GenericHashGetTurnLabel(tier_position.tier, tier_position.position);
    char player;
    int oppTurn;
    if (turn == 1) {
        player = X;
        oppTurn = 2;
    } else {
        player == O;
        oppTurn = 1;
    }
    int xToPlace, oToPlace, xOnBoard, oOnBoard;
    UnhashTier(tier_position.tier, &xToPlace, &oToPlace, &xOnBoard, &oOnBoard);
    int from, to, remove;
    UnhashMove(move, &from, &to, &remove);

    board[to] = player;
    if (player == X) {
        if (from != NONE) {  // Moving a piece
            board[from] = BLANK;
        } else {  // Placing a piece
            xToPlace--;
            xOnBoard++;
        }
        if (remove != NONE) {
            board[remove] = BLANK;
            oOnBoard--;
        }
    } else {
        if (from != NONE) {  // Moving a piece
            board[from] = BLANK;
        } else {  // Placing a piece
            oToPlace--;
            oOnBoard++;
        }
        if (remove != NONE) {
            board[remove] = BLANK;
            xOnBoard--;
        }
    }

    TierPosition ret;
    ret.tier = HashTier(xToPlace, oToPlace, xOnBoard, oOnBoard);
    ret.position = GenericHashHashLabel(ret.tier, board, oppTurn);
    return ret;
}

static bool MninemensmorrisIsLegalPosition(TierPosition tier_position) {
    // Currently we do not have a filter for unreachable positions
    return true;
}

static Position MtttierGetCanonicalPosition(TierPosition tier_position) {
    char board[boardSize];
    GenericHashUnhashLabel(tier_position.tier, tier_position.position, board);
    int turn =
        GenericHashGetTurnLabel(tier_position.tier, tier_position.position);

    int xToPlace, oToPlace, xOnBoard, oOnBoard;
    UnhashTier(tier_position.tier, &xToPlace, &oToPlace, &xOnBoard, &oOnBoard);

    char canonicalBoard[24];
    int bestSymmetryNum = 0;
    char pieceInSymmetry, pieceInCurrentCanonical;
    int i;

    for (int symmetryNum = 1; symmetryNum < totalNumBoardSymmetries;
         symmetryNum++) {
        for (i = 0; i < boardSize; i++) {
            pieceInSymmetry = board[symmetries[symmetryNum][i]];
            pieceInCurrentCanonical = board[symmetries[bestSymmetryNum][i]];
            if (pieceInSymmetry != pieceInCurrentCanonical) {
                if (pieceInSymmetry > pieceInCurrentCanonical) {
                    bestSymmetryNum = symmetryNum;
                }
                break;
            }
        }
    }

    if (bestSymmetryNum == 0) {
        return tier_position.position;
    } else {
        for (i = 0; i < boardSize; i++) {  // Transform the rest of the board.
            canonicalBoard[i] = board[symmetries[bestSymmetryNum][i]];
        }
        return GenericHashHashLabel(tier_position.tier, canonicalBoard, turn);
    }
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

/**
 * @brief Given `tier_position` = (T, P) where P is a position within T, and a
 * `symmetric` tier of the T, return a position in `symmetric_tier`
 * (not necessarily canonical within `symmetric_tier`) that is symmetric to P.
 *
 * @note A position P belonging to a tier T is symmetric to a position P'
 * that results from flipping the piece colors and turn of P.
 * P' belongs to a tier T' that is T with (1) xToPlace and oToPlace
 * swapped and (2) xOnBoard and oOnBoard swapped. Most tiers have one
 * non-identity symmetric tier. One exception is in non-Lasker-Rule games,
 * placing-phase tiers have no reachable non-identity symmetric tier.
 */
static Position MninemensmorrisGetPositionInSymmetricTier(
    TierPosition tier_position, Tier symmetric) {
    if (tier_position.tier == symmetric) {
        return tier_position.position;
    } else {
        /*
            We reach this case only after all pieces have been placed AND the
            players have different numbers of pieces on the board. A position
            in the m-white-pieces-and-n-black-pieces-on-the-board tier is
            symmetric to a position whose turn and pieces' colors are flipped
            which is in the n-white-pieces-and-m-black-pieces-on-the-board tier.
        */
        char board[boardSize];
        GenericHashUnhashLabel(tier_position.tier, tier_position.position,
                               board);
        int turn =
            GenericHashGetTurnLabel(tier_position.tier, tier_position.position);

        // Swap turn
        int oppTurn = (turn == 1) ? 2 : 1;

        // Swap colors of pieces
        for (int i = 0; i < boardSize; i++) {
            if (board[i] == X) {
                board[i] = O;
            } else if (board[i] == O) {
                board[i] = X;
            }
        }

        // This position in the symmetric tier is not necessarily canonical
        return GenericHashHashLabel(symmetric, board, oppTurn);
    }
}

static TierArray MninemensmorrisGetChildTiers(Tier tier) {
    TierArray children;
    TierArrayInit(&children);
    int xToPlace, oToPlace, xOnBoard, oOnBoard;
    UnhashTier(tier, &xToPlace, &oToPlace, &xOnBoard, &oOnBoard);

    int totalX = xToPlace + xOnBoard;
    int totalO = oToPlace + oOnBoard;

    if (totalX > 2 && totalO > 2) {
        /* Child tiers reachable through sliding/flying moves. If not using
        Lasker Rule, oToPlace == 0 means we are in the sliding phase. */
        if (laskerRule || oToPlace == 0) {
            /* Tier resulting from X sliding/flying a piece, forming a
            mill, then removing one of O's pieces */
            if (xOnBoard >= 3 && oOnBoard > 0) {
                TierArrayAppend(&children, HashTier(xToPlace, oToPlace,
                                                    xOnBoard, oOnBoard - 1));
            }

            /* Tier resulting from O sliding/flying a piece, forming a
            mill, then removing one of X's pieces */
            if (oOnBoard >= 3 && xOnBoard > 0) {
                TierArrayAppend(&children, HashTier(xToPlace, oToPlace,
                                                    xOnBoard - 1, oOnBoard));
            }
        }

        /* Child tiers resulting from X-placing moves. Note: in non-Lasker-Rule
        games, in the placing phase, xToPlace == oToPlace means X to place */
        if ((laskerRule || xToPlace == oToPlace) && xToPlace > 0) {
            /* Tier resulting from X sliding/flying a piece, without removal */
            TierArrayAppend(&children, HashTier(xToPlace - 1, oToPlace,
                                                xOnBoard + 1, oOnBoard));

            /* Tier resulting from X sliding/flying a piece, forming a
            mill, then removing one of O's pieces */
            if (oOnBoard > 1 && xOnBoard >= 2) {
                TierArrayAppend(&children,
                                HashTier(xToPlace - 1, oToPlace, xOnBoard + 1,
                                         oOnBoard - 1));
            }
        }

        /* Child tiers resulting from O-placing moves. Note: in non-Lasker-Rule
        games, in the placing phase, xToPlace > oToPlace means O to place */
        if ((laskerRule || xToPlace > oToPlace) && oToPlace > 0) {
            /* Tier resulting from O sliding/flying a piece, without removal */
            TierArrayAppend(&children, HashTier(xToPlace, oToPlace - 1,
                                                xOnBoard, oOnBoard + 1));

            /* Tier resulting from O sliding/flying a piece, forming a
            mill, then removing one of X's pieces */
            if (xOnBoard > 1 && oOnBoard >= 2) {
                TierArrayAppend(&children,
                                HashTier(xToPlace, oToPlace - 1, xOnBoard - 1,
                                         oOnBoard + 1));
            }
        }
    }

    return children;
}

static TierArray MninemensmorrisGetParentTiers(Tier tier) {
    TierArray parents;
    TierArrayInit(&parents);

    int xToPlace, oToPlace, xOnBoard, oOnBoard;
    UnhashTier(tier, &xToPlace, &oToPlace, &xOnBoard, &oOnBoard);

    int totalX = xToPlace + xOnBoard;
    int totalO = oToPlace + oOnBoard;

    /* Add parent tiers of `tier` from which `tier` is reachable via
    sliding/flying moves. If not using Lasker Rule, oToPlace == 0
    means that we are in the sliding phase. */
    if (laskerRule || oToPlace == 0) {
        /* Tier from which X could've slid/flown a piece, formed a
        mill, then removed one of O's pieces */
        if (xOnBoard >= 3 && totalO < gNumPiecesPerPlayer) {
            TierArrayAppend(
                &parents, HashTier(xToPlace, oToPlace, xOnBoard, oOnBoard + 1));
        }

        /* Tier from which O could've slid/flown a piece, formed a
        mill, then removed one of X's pieces */
        if (oOnBoard >= 3 && totalX < gNumPiecesPerPlayer) {
            TierArrayAppend(
                &parents, HashTier(xToPlace, oToPlace, xOnBoard + 1, oOnBoard));
        }
    }

    /* Add parent tiers from which current tier could've resulted from an
    X-placing move. Note: in non-Lasker-Rule games, in the placing phase,
    xToPlace > oToPlace means it is O's turn. */
    if ((laskerRule || xToPlace > oToPlace) && xOnBoard > 0) {
        /* Tier from which X could've placed a piece without removal */
        TierArrayAppend(
            &parents, HashTier(xToPlace + 1, oToPlace, xOnBoard - 1, oOnBoard));

        /* Tier from which X could've placed a piece, formed a
        mill, then removed one of O's pieces */
        if (xOnBoard >= 3 && totalO < gNumPiecesPerPlayer) {
            TierArrayAppend(&parents, HashTier(xToPlace + 1, oToPlace,
                                               xOnBoard - 1, oOnBoard + 1));
        }
    }

    /* Add parent tiers from which current tier could've resulted from an
    O-placing move. Note: in non-Lasker-Rule games, in the placing phase,
    xToPlace == oToPlace means it is X's turn. Also, when not using the
    Lasker rule, any Phase 2 tier will go through here. */
    if ((laskerRule || xToPlace == oToPlace) && oOnBoard > 0) {
        /* Tier from which O could've placed a piece without removal */
        TierArrayAppend(
            &parents, HashTier(xToPlace, oToPlace + 1, xOnBoard, oOnBoard - 1));

        /* Tier from which O could've placed a piece, formed a
        mill, then removed one of X's pieces */
        if (oOnBoard >= 3 && totalX < gNumPiecesPerPlayer) {
            TierArrayAppend(&parents, HashTier(xToPlace, oToPlace + 1,
                                               xOnBoard + 1, oOnBoard - 1));
        }
    }

    return parents;
}

static Tier MninemensmorrisGetCanonicalTier(Tier tier) {
    int xToPlace, oToPlace, xOnBoard, oOnBoard;
    UnhashTier(tier, &xToPlace, &oToPlace, &xOnBoard, &oOnBoard);
    Tier symmetricTier = HashTier(oToPlace, xToPlace, oOnBoard, xOnBoard);

    /* A position P belonging to a tier T is symmetric to a position P'
    that results from flipping the piece colors and turn of P.
    P' belongs to a tier T' that is T with the xToPlace and oToPlace
    swapped and the xOnBoard and oOnBoard swapped.

    One exception: For non-Lasker rule games, placing-phase tiers
    have no reachable non-identity symmetric tier. */
    if (!laskerRule && oToPlace > 0) {
        return tier;
    } else {
        return tier < symmetricTier ? tier : symmetricTier;
    }
}

static int MninemensmorrisTierPositionToString(TierPosition tier_position,
                                               char *buffer) {
    char board[boardSize];
    bool success = GenericHashUnhashLabel(tier_position.tier,
                                          tier_position.position, board);
    if (!success) return 1;

    int turn =
        GenericHashGetTurnLabel(tier_position.tier, tier_position.position);
    char player = (turn == 1) ? X : O;

    int xToPlace, oToPlace, xOnBoard, oOnBoard;
    UnhashTier(tier_position.tier, &xToPlace, &oToPlace, &xOnBoard, &oOnBoard);

    /* Replace all blanks with '.' */
    for (int i = 0; i < boardSize; ++i) {
        if (board[i] == BLANK) {
            board[i] = '.';
        }
    }

    static ConstantReadOnlyString kFormat16 =
        "\n"
        "          0 ----- 1 ----- 2    %c ----- %c ----- %c     It is %s's "
        "turn.\n"
        "          |       |       |    |       |       |    \n"
        "          |   3 - 4 - 5   |    |   %c - %c - %c   |     X has %d "
        "pieces left to place.\n"
        "          |   |       |   |    |   |       |   |     X has %d pieces "
        "on the board.\n"
        "LEGEND:   6 - 7       8 - 9    %c - %c       %c - %c\n"
        "          |   |       |   |    |   |       |   |     O has %d left to "
        "place.\n"
        "          |  10 - 11- 12  |    |   %c - %c - %c   |    O has %d "
        "pieces on the board.\n"
        "          |       |       |    |       |       |     \n"
        "          13 ---- 14 ---- 15   %c ----- %c ----- %c    \n\n";

    static ConstantReadOnlyString kFormat24 =
        "\n"
        "        0 --------- 1 --------- 2       %c --------- %c --------- %c  "
        "   It is %s's turn.\n"
        "        |           |           |       | %c         |         %c |\n"
        "        |   3 ----- 4 ----- 5   |       |   %c ----- %c ----- %c   |"
        "        |   |       |       |   |       |   |       |       |   |     "
        "X has %d pieces left to place.\n"
        "        |   |   6 - 7 - 8   |   |       |   |   %c - %c - %c   |   |  "
        "   X has %d pieces on the board.\n"
        "        |   |   |       |   |   |       |   |   |       |   |   |\n"
        "LEGEND: 9 - 10- 11      12- 13- 14      %c - %c - %c       %c - %c - "
        "%c\n"
        "        |   |   |       |   |   |       |   |   |       |   |   |\n"
        "        |   |   15- 16- 17  |   |       |   |   %c - %c - %c   |   |  "
        "   O has %d left to place.\n"
        "        |   |       |       |   |       |   |       |       |   |     "
        "O has %d pieces on the board.\n"
        "        |   18 ---- 19 ---- 20  |       |   %c ----- %c ----- %c   |\n"
        "        |           |           |       | %c         |         %c |\n"
        "        21 -------- 22 -------- 23      %c --------- %c --------- %c "
        "\n\n";

    int actualLength = 0;
    if (boardSize == 16) {
        actualLength =
            snprintf(buffer, kGameplayApi.position_string_length_max + 1,
                     kFormat16, board[0], board[1], board[2], player, board[3],
                     board[4], board[5], xToPlace, xOnBoard, board[6], board[7],
                     board[8], board[9], oToPlace, board[10], board[11],
                     board[12], oOnBoard, board[13], board[14], board[15]);
    } else {
        /* If using 24BoardPlus, set characters to indicate diagonals. */
        char diag = ' ';
        char antiDiag = ' ';
        if (boardType == 2) {
            diag = '/';
            antiDiag = '\\';
        }
        actualLength = snprintf(
            buffer, kGameplayApi.position_string_length_max + 1, kFormat24,
            board[0], board[1], board[2], player, antiDiag, diag, board[3],
            board[4], board[5], xToPlace, board[6], board[7], board[8],
            xOnBoard, board[9], board[10], board[11], board[12], board[13],
            board[14], board[15], board[16], board[17], oToPlace, oOnBoard,
            board[18], board[19], board[20], diag, antiDiag, board[21],
            board[22], board[23]);
    }

    if (actualLength >= kGameplayApi.position_string_length_max + 1) {
        fprintf(stderr,
                "MninemensmorrisTierPositionToString: (BUG) not enough space "
                "was allocated "
                "to buffer. Please increase position_string_length_max.\n");
        return 1;
    }
    return 0;
}

static int MninemensmorrisMoveToString(Move move, char *buffer) {
    // TODO: What's up with PRIDnt
    int actualLength;
    int from, to, remove;
    unhashMove(move, &from, &to, &remove);
    int maxStrLen = kGameplayApi.move_string_length_max + 1;

    if (from != NONE && to != NONE && remove != NONE) {
        actualLength =
            snprintf(buffer, maxStrLen, "%d-%dr%d", from, to, remove);
    } else if (from != NONE && to != NONE && remove == NONE) {
        actualLength = snprintf(buffer, maxStrLen, "%d-%d", from, to);
    } else if (from == NONE && to != NONE && remove == NONE) {
        actualLength = snprintf(buffer, maxStrLen, "%d", to);
    } else {
        actualLength = snprintf(buffer, maxStrLen, "%dr%d", to, remove);
    }

    if (actualLength >= maxStrLen) {
        fprintf(
            stderr,
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
        if (!((moveString[i] >= '0' && moveString[i] <= '9') ||
              moveString[i] == '-' || moveString[i] == 'r')) {
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
    int to = NONE;      // that way if no input for remove, it's equal to from.
                        // useful in function DoMove
    int remove = NONE;  // for stage 1, if there is nothing to remove

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

    if (phase == 0) {  // Placement without removal
        to = first;
    } else if (phase == 2) {  // Sliding/flying with removal
        from = first;
        to = second;
        remove = third;
    } else if (existsSliding) {  // Sliding/flying without removal
        from = first;
        to = second;
    } else {  // Placement with removal.
        to = first;
        remove = second;
    }
    return MOVE_ENCODE(from, to, remove);  // HASHES THE MOVE
}

/* Implementation of Helper Functions */

static Tier HashTier(int xToPlace, int oToPlace, int xOnBoard, int oOnBoard) {
    return (xToPlace << 12) | (oToPlace << 8) | (xOnBoard << 4) | oOnBoard;
}

static void UnhashTier(Tier tier, int *xToPlace, int *oToPlace, int *xOnBoard,
                       int *oOnBoard) {
    *xToPlace = (tier >> 12) & 0xF;
    *oToPlace = (tier >> 8) & 0xF;
    *xOnBoard = (tier >> 4) & 0xF;
    *oOnBoard = tier & 0xF;
}

static void UnhashMove(Move move, int *from, int *to, int *remove) {
    *from = move >> 10;
    *to = (move >> 5) & 0x1F;
    *remove = move & 0x1F;
}

static bool InitGenericHash(void) {
    GenericHashReinitialize();
    int player = 0;
    int pieces_init_array[10] = {X, 0, 0, O, 0, 0, BLANK, 0, 0, -1};
    bool success;
    Tier tier;
    int xToPlace, oToPlace, xOnBoard, oOnBoard;

    if (laskerRule) {
        for (xToPlace = 0; xToPlace <= gNumPiecesPerPlayer; xToPlace++) {
            for (oToPlace = 0; oToPlace <= gNumPiecesPerPlayer; oToPlace++) {
                for (xOnBoard = 0; xOnBoard <= gNumPiecesPerPlayer - xToPlace;
                     xOnBoard++) {
                    for (oOnBoard = 0;
                         oOnBoard <= gNumPiecesPerPlayer - oToPlace;
                         oOnBoard++) {
                        tier = HashTier(xToPlace, oToPlace, xOnBoard, oOnBoard);
                        pieces_init_array[1] = pieces_init_array[2] = xOnBoard;
                        pieces_init_array[4] = pieces_init_array[5] = oOnBoard;
                        pieces_init_array[7] = pieces_init_array[8] =
                            boardSize - xOnBoard - oOnBoard;

                        success = GenericHashAddContext(
                            player, boardSize, pieces_init_array, NULL, tier);

                        if (!success) {
                            fprintf(stderr,
                                    "MninemensmorrisInit: failed to initialize "
                                    "generic hash context "
                                    "for tier %" PRId64 ". Aborting...\n",
                                    tier);
                            GenericHashReinitialize();
                            return false;
                        }
                    }
                }
            }
        }
    } else {
        for (int piecesLeft = 0; piecesLeft <= 2 * gNumPiecesPerPlayer;
             piecesLeft++) {
            oToPlace = piecesLeft / 2;
            xToPlace = 2 * gNumPiecesPerPlayer - oToPlace;
            for (xOnBoard = 0; xOnBoard <= gNumPiecesPerPlayer - xToPlace;
                 xOnBoard++) {
                for (oOnBoard = 0; oOnBoard <= gNumPiecesPerPlayer - oToPlace;
                     oOnBoard++) {
                    tier = HashTier(xToPlace, oToPlace, xOnBoard, oOnBoard);
                    pieces_init_array[1] = pieces_init_array[2] = xOnBoard;
                    pieces_init_array[4] = pieces_init_array[5] = oOnBoard;
                    pieces_init_array[7] = pieces_init_array[8] =
                        boardSize - xOnBoard - oOnBoard;

                    // Odd piecesLeft means it's P1's turn.
                    // Nonzero even piecesLeft means it's P2's turn.
                    player = (piecesLeft == 0) ? 0 : (piecesLeft & 1) ? 2 : 1;

                    success = GenericHashAddContext(
                        player, boardSize, pieces_init_array, NULL, tier);

                    if (!success) {
                        fprintf(stderr,
                                "MninemensmorrisInit: failed to initialize "
                                "generic hash context "
                                "for tier %" PRId64 ". Aborting...\n",
                                tier);
                        GenericHashReinitialize();
                        return false;
                    }
                }
            }
        }
    }

    return true;
}

/**
 * @brief We are given a `board` that has a piece at `slot` belonging to a
 * given `player`. Return whether the piece at `slot` is part of a mill of
 * other pieces belonging to that player.
 *
 * @note In the regular 24-Board, each slot is part of 2 lines, so we do 2
 * mill checks. In the 16-Board, each slot is part of either 1 or 2 lines.
 * In the 24-Board-Plus each slot is part of either 2 or 3 lines. The
 * if-statement cases are ordered with consideration for what is more
 * efficient for solving variants using 24-Board or 24-Board-Plus.
 * See the linesArray explanation above for more information.
 *
 * @note MILL() assumes that `slot` already contains the piece
 * indicated by `turn`.
 */
static bool IsSlotInMill(char *board, int slot, char player) {
    int *lines = linesArray[slot];
    bool firstLineIsMill = MILL(board, lines[0], lines[1], player);
    if (boardType == 2 || lines[7] == 2) {
        return firstLineIsMill || MILL(board, lines[2], lines[3], player);
    } else if (lines[7] == 3) {
        return firstLineIsMill || MILL(board, lines[2], lines[3], player) ||
               MILL(board, lines[4], lines[5], player);
    } else if (lines[7] == 1) {
        return firstLineIsMill;
    }
}

/**
 * @brief Suppose the `player` whose turn it is either (1) moves a piece from
 * the slot indicated by `from` to the slot indicated by `to` or (2)
 * places a piece at the slot indicated by `to`. Return whether
 * doing so creates a mill of `player`'s pieces.
 *
 * @note `board` is the board before `player` makes their move. Assume
 * that the move indicated by `from` and `to` is legal.
 *
 * @note We only need to make a change to the `from` slot but
 * not to the `to` slot before passing the board into IsSlotInMill() because
 * IsSlotInMill does not check the input slot; it just checks the other
 * slot in the lines that the input slot belongs to.
 */
static bool ClosesMill(char *board, char player, int from, int to) {
    char pieceAtFrom = board[from];
    if (from != NONE) {  // If move is sliding move
        board[from] = BLANK;
    }
    bool ret = IsSlotInMill(board, to, player);
    board[from] = pieceAtFrom;  // Undo change
    return ret;
}

/**
 * @brief Return whether each piece on the `board` belonging
 * to `player` is part of a mill.
 */
static bool AllPiecesInMills(char *board, char player) {
    for (int slot = 0; slot < boardSize; slot++) {
        if (board[slot] == player) {
            if (!IsSlotInMill(board, slot, player)) {
                return false;
            }
        }
    }
    return true;
}

/**
 * @brief Given a `board`, store the slots of all of the `player`'s
 * opponent's pieces that are removable according to the removalRule in
 * `legalRemoves` and return how many of the opponent's pieces are removable.
 */
static int FindLegalRemoves(char *board, char player, int *legalRemoves) {
    int numLegalRemoves = 0;
    char opponent = player == X ? O : X;
    int slot;
    if (removalRule == 0) {
        /* Standard. An opponent's piece is removable if either all of the
        opponent's pieces are in mills or if the piece is not in a mill. */
        for (slot = 0; slot < boardSize; slot++) {
            if (board[slot] == opponent &&
                (!IsSlotInMill(board, slot, opponent) ||
                 AllPiecesInMills(board, opponent))) {
                legalRemoves[numLegalRemoves++] = slot;
            }
        }
    } else if (removalRule == 1) {
        /* An opponent's piece can be removed regardless of whether it
        is in a mill or not. */
        for (slot = 0; slot < boardSize; slot++) {
            if (board[slot] == opponent) {
                legalRemoves[numLegalRemoves++] = slot;
            }
        }
    } else {
        /* An opponent's piece can only be removed if it is not in a mill. */
        for (slot = 0; slot < boardSize; slot++) {
            if (board[slot] == opponent &&
                !IsSlotInMill(board, slot, opponent)) {
                legalRemoves[numLegalRemoves++] = slot;
            }
        }
    }
    return numLegalRemoves;
}