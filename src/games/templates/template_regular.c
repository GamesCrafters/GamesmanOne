/**
 * @file template_regular.c
 * @author Robert Shi (robertyishi@berkeley.edu)
 *         GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Template for regular (non-tier) games - implementation.
 * @version 1.0.0
 * @date 2024-02-07
 *
 *
 * @todo 1. Copy this file and template_regular.h to your game's directory.
 *       2. Rename both file to your game's name. Make sure they have the same
 * name.
 *       3. Modify the file name above in this comment section to match the name
 * of your game.
 *       4. Add your name, a one-line brief description of your game, version,
 * and date.
 *       5. Fix the include path for your game. Make sure you are including the
 * header of your game instead of the template header.
 *       6. Go to the "Core Game Definition" section and fill in the definition
 * of your game.
 *       7. Go to the "API Setup" section and fill in the APIs that you need or
 * wish to setup.
 *       8. Go to the "Game API" section, rename the functions to match your
 * game, and fill in the function definitions.
 *       9. Go to the "Regular Solver API" section, rename the functions to
 * match your game, and fill in the function definitions.
 *       10. Go to the "Gameplay API" section, rename the functions to match
 * your game, and fill in the function definitions.
 *       11. (OPTIONAL) If you wish to add your game to GamesmanUni, go to the
 * "UWAPI" section and fill in the functions.
 *       12. Go to src/games/game_list.c, include your game's header file, and
 * follow the instructions to add your game to the list of all games.
 *       13. Go to Makefile.am in the GamesmanOne's source directory and add
 * all .c source files that you created to the bottom of the file under "All
 * Games".
 *       14. Delete all TODO labels before releasing your game. HINT: search for
 * "todo" in your editor to check if all tasks are complete. HINT: search for
 * "template" in case-insensitive mode to make sure you renamed all functions
 * and variables to match your game.
 *
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

// TODO: Rename this to match the path of your game.
#include "games/templates/template_regular.h"

#include <stddef.h>  // NULL
// TODO: You may need to include additional headers here.
// E.g. #include <stdio.h>  // printf

// TODO: Go inside each one of these files and learn about the types and
// data structures that are defined in GAMESMAN before you implement your
// own.
#include "core/constants.h"
#include "core/solvers/regular_solver/regular_solver.h"
#include "core/types/gamesman_types.h"
// TODO: (optional) If you decide to split your game implementation into
// several modules, you may want to include them here.

// ---------------------------------- Game API ---------------------------------

static int TemplateRegularInit(void *aux);
static int TemplateRegularFinalize(void);
static const GameVariant *TemplateRegularGetCurrentVariant(void);  // (OPTIONAL)
static int TemplateRegularSetVariantOption(int option,
                                           int selection);  // (OPTIONAL)

// -----------------------------------------------------------------------------

// ----------------------------- Regular Solver API ----------------------------

static int64_t TemplateRegularGetNumPositions(void);
static Position TemplateRegularGetInitialPosition(void);

static MoveArray TemplateRegularGenerateMoves(Position position);
static Value TemplateRegularPrimitive(Position position);
static Position TemplateRegularDoMove(Position position, Move move);
static bool TemplateRegularIsLegalPosition(Position position);

static Position TemplateRegularGetCanonicalPosition(
    Position position);  // (OPTIONAL)
static PositionArray TemplateRegularGetCanonicalParentPositions(
    Position position);  // (OPTIONAL, but important if you run out of RAM)

// -----------------------------------------------------------------------------

// -------------------------------- Gameplay API -------------------------------

static int TemplateRegularPositionToString(Position position, char *buffer);
static int TemplateRegularMoveToString(Move move, char *buffer);
static bool TemplateRegularIsValidMoveString(ReadOnlyString move_string);
static Move TemplateRegularStringToMove(ReadOnlyString move_string);

// -----------------------------------------------------------------------------

// ----------------------------- UWAPI (OPTIONAL) ------------------------------

static bool TemplateRegularIsLegalFormalPosition(
    ReadOnlyString formal_position);
static Position TemplateRegularFormalPositionToPosition(
    ReadOnlyString formal_position);
static CString TemplateRegularPositionToFormalPosition(Position position);
static CString TemplateRegularPositionToAutoGuiPosition(Position position);
static CString TemplateRegularMoveToFormalMove(Position position, Move move);
static CString TemplateRegularMoveToAutoGuiMove(Position position, Move move);

// -----------------------------------------------------------------------------

// --------------------------------- API Setup ---------------------------------

// TODO: Read the type definition of RegularSolverApi in
// src/core/solvers/regular_solver/regular_solver.h and then fill in the API
// with functions of your game. Note that these functions need not be defined
// as a static (local) function within this file. If your game is complicated,
// feel free to add more files under your game path.
// All fields except those marked as (OPTIONAL) are required of your game
// to run.
// All required fields and some (OPTIONAL) fields have already been filled out
// for you. You only need to rename those functions to match the name of your
// game.
static const RegularSolverApi kTemplateRegularSolverApi = {
    .GetNumPositions = &TemplateRegularGetNumPositions,
    .GetInitialPosition = &TemplateRegularGetInitialPosition,

    .GenerateMoves = &TemplateRegularGenerateMoves,
    .Primitive = &TemplateRegularPrimitive,
    .DoMove = &TemplateRegularDoMove,
    .IsLegalPosition = &TemplateRegularIsLegalPosition,

    .GetCanonicalPosition = &TemplateRegularGetCanonicalPosition,  // (OPTIONAL)
    .GetNumberOfCanonicalChildPositions = NULL,                    // (OPTIONAL)
    .GetCanonicalChildPositions = NULL,                            // (OPTIONAL)
    .GetCanonicalParentPositions =
        &TemplateRegularGetCanonicalParentPositions,  // (OPTIONAL)
};

// TODO: Read the type definition of GameplayApiCommon in
// src/core/types/gameplay_api/gameplay_api_common.h and then fill in the API
// with functions of your game.
// All fields are required and have already been filled out for you. However,
// some constant values may need to be modified to match the definition of the
// functions you provided.
static const GameplayApiCommon kTemplateRegularGameplayApiCommon = {
    .GetInitialPosition = &TemplateRegularGetInitialPosition,
    .position_string_length_max = 120,  // TODO: Adjust this.

    .move_string_length_max = 1,  // TODO: Adjust this.
    .MoveToString = &TemplateRegularMoveToString,

    .IsValidMoveString = &TemplateRegularIsValidMoveString,
    .StringToMove = &TemplateRegularStringToMove,
};

// TODO: Read the type definition of GameplayApiRegular in
// src/core/types/gameplay_api/gameplay_api_regular.h and then fill in the API
// with functions of your game.
// All fields are required and have already been filled out for you.
static const GameplayApiRegular kTemplateRegularGameplayApiRegular = {
    .PositionToString = &TemplateRegularPositionToString,

    .GenerateMoves = &TemplateRegularGenerateMoves,
    .DoMove = &TemplateRegularDoMove,
    .Primitive = &TemplateRegularPrimitive,
};

// TODO: Read the type definition of GameplayApi in
// src/core/types/gameplay_api/gameplay_api.h and then fill in the API
// with the objects that you defined in the above sections.
// Only the two fields below should be filled out and we already did it for you.
// Don't forget to rename them to match the name of your game.
static const GameplayApi kTemplateRegularGameplayApi = {
    .common = &kTemplateRegularGameplayApiCommon,
    .regular = &kTemplateRegularGameplayApiRegular,
};

// TODO: (OPTIONAL) Read the type definition of UwapiRegular in
// src/core/types/uwapi/uwapi_regular.h and then fill in the API
// with functions of your game.
// All required fields have been filled out for you.
static const UwapiRegular kTemplateRegularUwapiRegular = {
    .GenerateMoves = &TemplateRegularGenerateMoves,
    .DoMove = &TemplateRegularDoMove,
    .IsLegalFormalPosition = &TemplateRegularIsLegalFormalPosition,
    .FormalPositionToPosition = &TemplateRegularFormalPositionToPosition,
    .PositionToFormalPosition = &TemplateRegularPositionToFormalPosition,
    .PositionToAutoGuiPosition = &TemplateRegularPositionToAutoGuiPosition,
    .MoveToFormalMove = &TemplateRegularMoveToFormalMove,
    .MoveToAutoGuiMove = &TemplateRegularMoveToAutoGuiMove,
    .GetInitialPosition = &TemplateRegularGetInitialPosition,
    .GetRandomLegalPosition = NULL,  // (OPTIONAL)
};

// TODO: (OPTIONAL) Read the type definition of Uwapi in
// src/core/types/uwapi/uwapi.h and then fill in the API with the UwapiRegular
// object that you defined in the section above.
// You should only fill out the Uwapi::regular field and we did it for you.
// Don't forget to change the name of the object to match your game.
static const Uwapi kTemplateRegularUwapi = {.regular =
                                                &kTemplateRegularUwapiRegular};

// -----------------------------------------------------------------------------

// --------------------------- Core Game Definition ----------------------------

// TODO: Read the description of a Game type in src/core/types/game/game.h,
// then fill in the name, formal name, and the relavent functions here.
// All fields are required unless marked with (OPTIONAL). You can delete
// all optional fields if you are not implementing them. You can also set them
// to NULL, which is equivalent to deleting them.
const Game kTemplateRegular = {
    .name = "TemplateRegular",
    .formal_name = "Template for a regular (non-tier) game",

    .solver = &kRegularSolver,
    .solver_api = (const void *)&kTemplateRegularSolverApi,
    .gameplay_api = (const GameplayApi *)&kTemplateRegularGameplayApi,
    .uwapi = (const Uwapi *)&kTemplateRegularUwapi,  // (OPTIONAL)

    .Init = &TemplateRegularInit,
    .Finalize = &TemplateRegularFinalize,

    .GetCurrentVariant = &TemplateRegularGetCurrentVariant,  // (OPTIONAL)
    .SetVariantOption = &TemplateRegularSetVariantOption,    // (OPTIONAL)
};

// -----------------------------------------------------------------------------

// TODO: fill in the function definitions below.

// ---------------------------------- Game API ---------------------------------

static int TemplateRegularInit(void *aux) {
    // TODO
    return kNotImplementedError;
}

static int TemplateRegularFinalize(void) {
    // TODO
    return kNotImplementedError;
}

static const GameVariant *TemplateRegularGetCurrentVariant(void) {
    // TODO: (OPTIONAL)
    return NULL;
}

static int TemplateRegularSetVariantOption(int option, int selection) {
    // TODO: (OPTIONAL)
    return kNotImplementedError;
}

// -----------------------------------------------------------------------------

// ----------------------------- Regular Solver API ----------------------------

static int64_t TemplateRegularGetNumPositions(void) {
    // TODO
    return -1;
}

static Position TemplateRegularGetInitialPosition(void) {
    // TODO
    return -1;
}

static MoveArray TemplateRegularGenerateMoves(Position position) {
    // TODO
    MoveArray moves;
    MoveArrayInit(&moves);  // This creates an empty array.

    return moves;
}

static Value TemplateRegularPrimitive(Position position) {
    // TODO
    return kUndecided;
}

static Position TemplateRegularDoMove(Position position, Move move) {
    // TODO
    return -1;
}

static bool TemplateRegularIsLegalPosition(Position position) {
    // TODO
    return false;
}

static Position TemplateRegularGetCanonicalPosition(Position position) {
    // TODO: (OPTIONAL)
    return -1;
}

static PositionArray TemplateRegularGetCanonicalParentPositions(
    Position position) {
    // TODO: (OPTIONAL)
    PositionArray ret;
    PositionArrayInit(&ret);  // This creates an empty array.

    return ret;
}

// -----------------------------------------------------------------------------

// -------------------------------- Gameplay API -------------------------------

static int TemplateRegularPositionToString(Position position, char *buffer) {
    // TODO
    return kNotImplementedError;
}

static int TemplateRegularMoveToString(Move move, char *buffer) {
    // TODO
    return kNotImplementedError;
}

static bool TemplateRegularIsValidMoveString(ReadOnlyString move_string) {
    // TODO
    return false;
}

static Move TemplateRegularStringToMove(ReadOnlyString move_string) {
    // TODO
    return -1;
}

// -----------------------------------------------------------------------------

// ----------------------------- UWAPI (OPTIONAL) ------------------------------
// NOTE: all functions below are optional if you are not adding the game to
// GamesmanUni.

static bool TemplateRegularIsLegalFormalPosition(
    ReadOnlyString formal_position) {
    // TODO
    return false;
}

static Position TemplateRegularFormalPositionToPosition(
    ReadOnlyString formal_position) {
    // TODO
    return -1;
}

static CString TemplateRegularPositionToFormalPosition(Position position) {
    // TODO
    CString formal_position;
    CStringInit(&formal_position, "");  // This creates an empty CString.

    return formal_position;
}

static CString TemplateRegularPositionToAutoGuiPosition(Position position) {
    // TODO
    CString autogui_position;
    CStringInit(&autogui_position, "");  // This creates an empty CString.

    return autogui_position;
}

static CString TemplateRegularMoveToFormalMove(Position position, Move move) {
    // TODO
    CString formal_move;
    CStringInit(&formal_move, "");  // This creates an empty CString.

    return formal_move;
}

static CString TemplateRegularMoveToAutoGuiMove(Position position, Move move) {
    // TODO
    CString autogui_move;
    CStringInit(&autogui_move, "");  // This creates an empty CString.

    return autogui_move;
}

// -----------------------------------------------------------------------------
