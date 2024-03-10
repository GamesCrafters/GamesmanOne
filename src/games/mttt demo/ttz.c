
#include "games/Ttz/Ttz.h"
#include <stddef.h> //NULL
#include "core/solvers/regular_solver/regular_solver.h"
#include "core/types/gamesman_types.h"

// Game, Solver, Gameplay, and UWAPI Functions

static int TtzInit(void *aux);
static int TtzFinalize(void);

//static const GameVariant *TtzGetCurrentVariant(void);
//static int TtzSetVariantOption(int option, int selection);

//Solver API
static int64_t TtzGetNumPositions(void);
static Position TtzGetInitialPosition(void);
static MoveArray TtzGenerateMoves(Position position);
static Value TtzPrimitive(Position position);
static Position TtzDoMove(Position position, Move move);
static bool TtzIsLegalPosition(Position position);
static Position TtzGetCanonicalPosition(Position position);
static PositionArray TtzGetCanonicalParentPositions(Position position);

static int TtzPositionToString(Position position, char *buffer);
static int TtzMoveToString(Move move, char *buffer);
static bool TtzIsValidMoveString(ReadOnlyString move_string);
static Move TtzStringToMove(ReadOnlyString move_string);

static bool TtzIsLegalFormalPosition(ReadOnlyString formal_position);
static Position TtzFormalPositionToPosition(ReadOnlyString formal_position);
static CString TtzPositionToFormalPosition(Position position);
static CString TtzPositionToAutoGuiPosition(Position position);
static CString TtzMoveToFormalMove(Position position, Move move);
static CString TtzMoveToAutoGuiMove(Position position, Move move);

// Solver API Setup
static const RegularSolverApi kTtzSolverApi = {
    .GetNumPositions = &TtzGetNumPositions,
    .GetInitialPosition = &TtzGetInitialPosition,

    .GenerateMoves = &TtzGenerateMoves,
    .Primitive = &TtzPrimitive,
    .DoMove = &TtzDoMove,
    .IsLegalPosition = &TtzIsLegalPosition,
    .GetCanonicalPosition = &TtzGetCanonicalPosition,
    .GetCanonicalChildPositions = NULL,
    .GetCanonicalParentPositions = &TtzGetCanonicalParentPositions,
};

// Gameplay API Setup

static const GameplayApiCommon kTtzGameplayApiCommon = {
    .GetInitialPosition = &TtzGetInitialPosition,
    .position_string_length_max = 120,

    .move_string_length_max = 1,
    .MoveToString = &TtzMoveToString,

    .IsValidMoveString = &TtzIsValidMoveString,
    .StringToMove = &TtzStringToMove,
};

static const GameplayApiRegular kTtzGameplayApiRegular = {
    .PositionToString = &TtzPositionToString,

    .GenerateMoves = &TtzGenerateMoves,
    .DoMove = &TtzDoMove,
    .Primitive = &TtzPrimitive,
};

static const GameplayApi kTtzGameplayApi = {
    .common = &kTtzGameplayApiCommon,
    .regular = &kTtzGameplayApiRegular,
};

// UWAPI Setup

static const UwapiRegular kTtzUwapiRegular = {
    .GenerateMoves = &TtzGenerateMoves,
    .DoMove = &TtzDoMove,
    .IsLegalFormalPosition = &TtzIsLegalFormalPosition,
    .FormalPositionToPosition = &TtzFormalPositionToPosition,
    .PositionToFormalPosition = &TtzPositionToFormalPosition,
    .PositionToAutoGuiPosition = &TtzPositionToAutoGuiPosition,
    .MoveToFormalMove = &TtzMoveToFormalMove,
    .MoveToAutoGuiMove = &TtzMoveToAutoGuiMove,
    .GetInitialPosition = &TtzGetInitialPosition,
    .GetRandomLegalPosition = NULL,
};

static const Uwapi kTtzUwapi = {.regular = &kTtzUwapiRegular};

// -----------------------------------------------------------------------------

const Game kTtz = {
    .name = "ttz",
    .formal_name = "10 to 0 by 1 or 2",
    .solver = &kRegularSolver,
    .solver_api = (const void *)&kTtzSolverApi,
    .gameplay_api = (const GameplayApi *)&kTtzGameplayApi,
    .uwapi = NULL,

    .Init = &TtzInit,
    .Finalize = &TtzFinalize,

    .GetCurrentVariant = NULL,
    .SetVariantOption = NULL,
};

// -----------------------------------------------------------------------------

// Game, Solver, Gameplay, and UWAPI Functions

static int TtzInit(void *aux){
    (void)aux; //Unused.
    return kNoError;
}
static int TtzFinalize(void){
    return kNoError;
}

//static const GameVariant *TtzGetCurrentVariant(void);
//static int TtzSetVariantOption(int option, int selection);

//Solver API
static int64_t TtzGetNumPositions(void){
    return 11;
}

//Hash: position valuee directly used as hash
static Position TtzGetInitialPosition(void){
    return 10;
}

static MoveArray TtzGenerateMoves(Position position){
    MoveArray moves;
    MoveArrayInit(&moves);
    if (position == 0){
        return moves;
    } else if (position == 1){
        MoveArrayAppend(&moves, 1);
    } else { //position > 1
        MoveArrayAppend(&moves, 1);
        MoveArrayAppend(&moves, 2);
    }
}

static Value TtzPrimitive(Position position){
    //position is the hash entry, but here the game is so simple that it's just the same as the entry
    if (position == 0) return kLose;
    return kUndecided;

}

static Position TtzDoMove(Position position, Move move){
    //going to assume that the position is simple and move as legal in the position
    return position - move;

}

static bool TtzIsLegalPosition(Position position){
    //no illegal positions in this game
    return true;
}

static Position TtzGetCanonicalPosition(Position position){
    

}

static PositionArray TtzGetCanonicalParentPositions(Position position){

}