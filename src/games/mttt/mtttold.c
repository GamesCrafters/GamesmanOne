/************************************************************************
**
** NAME:        mttt.c
**
** DESCRIPTION: Tic-Tac-Toe
**
** AUTHOR:      Dan Garcia  -  University of California at Berkeley
**              Copyright (C) Dan Garcia, 1995. All rights reserved.
**
** DATE:        08/28/91
**
** UPDATE HIST:
**
**  1991-08-30 1.0a1 : Fixed the bug in reading the input - now 'q' doesn't barf.
**  1991-09-06 1.0a2 : Added the two extra arguments to PrintPosition
**                     Recoded the way to do "visited" - bitmask
**  1991-09-06 1.0a3 : Added Symmetry code - whew was that a lot of code!
**  1991-09-06 1.0a4 : Added ability to have random linked list in gNextMove.
**  1991-09-06 1.0a5 : Removed redundant code - replaced w/GetRawValueFromDatabase
**  1991-09-17 1.0a7 : Added graphics code.
**  1992-05-12 1.0a8 : Added Static Evaluation - it's far from perfect, but
**                     it works!
**  1995-05-15 1.0   : Final release code for M.S.
**  1997-05-12 1.1   : Removed gNextMove and any storage of computer's move
**  2023-07-05 2.0a1 : Adapt to new GamesmanClassic system.
** 
**
** Decided to check out how much space was wasted with the array:
**
** A Dartboard 9-slot hash has 6046 positions (all symmetries included)
**
** Without checking for symmetries
**
** Undecided = 14205 out of 19683
** Lose      =  1574 out of 19683
** Win       =  2836 out of 19683
** Tie       =  1068 out of 19683
** Unknown   =     0 out of 19683
** TOTAL     =  5478 out of 19683
**
** With SLIM = Symmetry-limiting Initial Move
** (only 1st move do we limit moves to 1,2,5)
**
** Lose      =  1274 out of 4163
** Win       =  2083 out of 4163
** Tie       =   806 out of 4163
** Unknown   =     0 out of 4163
** TOTAL     =  4163 out of 19683 allocated
**
** With SLIMFAST = Symmetry-LImiting Move Fast!
** (EVERY move we limit if there are symmetries)
**
** Lose      =  1084 out of 3481
** Win       =  1725 out of 3481
** Tie       =   672 out of 3481
** Unknown   =     0 out of 3481 (Sanity-check...should always be 0)
** TOTAL     =  3481 out of 19683 allocated
**
**     Time Loss : ??
** Space Savings : 1.573
**
** While checking for symmetries and storing a canonical elt from them.
**
** Evaluating the value of Tic-Tac-Toe...done in 5.343184 seconds!
** Undecided = 18917 out of 19682
** Lose      =   224 out of 19682
** Win       =   390 out of 19682
** Tie       =   151 out of 19682
** Unk       =     0 out of 19682
** TOTAL     =   765 out of 19682
**
**     Time Loss : 3.723
** Space Savings : 7.160 (why did I earlier write 6.279?)
**
**************************************************************************/

#include <stdio.h>
#include <stdbool.h>

#include "gamesman_types.h"

// Position gNumberOfPositions  = 19683;  /* 3^9 */

#define BOARD_SIZE     9           /* 3x3 board */
#define BOARD_ROWS     3
#define BOARD_COLS     3

typedef enum PossibleBoardPieces {
	kBlank, kO, kX
} BlankOX;

const char *gBlankOXString[] = { " ", "o", "x" };

/* Powers of 3 - this is the way I encode the position, as an integer */
int g3Array[] = { 1, 3, 9, 27, 81, 243, 729, 2187, 6561 };

/* Global position solver variables.*/
struct {
	BlankOX board[BOARD_SIZE];
	BlankOX nextPiece;
	int piecesPlaced;
} gPosition;

/** Function Prototypes **/
bool AllFilledIn(BlankOX theBlankOX[]);
Position BlankOXToPosition(BlankOX *theBlankOX);
Position GetCanonicalPosition(Position position);
void PositionToBlankOX(Position thePos,BlankOX *theBlankOX);
bool ThreeInARow(BlankOX[], int, int, int);
BlankOX WhoseTurn(BlankOX *theBlankOX);

char *MoveToString( Move );
Position ActualNumberOfPositions(int variant);

/**************************************************/
/**************** SYMMETRY FUN BEGIN **************/
/**************************************************/

bool kSupportsSymmetries = true; /* Whether we support symmetries */

#define NUMSYMMETRIES 8   /*  4 rotations, 4 flipped rotations */

int gSymmetryMatrix[NUMSYMMETRIES][BOARD_SIZE];

/* Proofs of correctness for the below arrays:
**
** FLIP						ROTATE
**
** 0 1 2	2 1 0		0 1 2		6 3 0		8 7 6		2 5 8
** 3 4 5  ->    5 4 3		3 4 5	->	7 4 1  ->	5 4 3	->	1 4 7
** 6 7 8	8 7 6		6 7 8		8 5 2		2 1 0		0 3 6
*/

/* This is the array used for flipping along the N-S axis */
int gFlipNewPosition[] = { 2, 1, 0, 5, 4, 3, 8, 7, 6 };

/* This is the array used for rotating 90 degrees clockwise */
int gRotate90CWNewPosition[] = { 6, 3, 0, 7, 4, 1, 8, 5, 2 };

/**************************************************/
/**************** SYMMETRY FUN END ****************/
/**************************************************/

/************************************************************************
**
** NAME:        InitializeDatabases
**
** DESCRIPTION: Initialize the gDatabase, a global variable.
**
************************************************************************/

void InitializeGame()
{
	/**************************************************/
	/**************** SYMMETRY FUN BEGIN **************/
	/**************************************************/

	gCanonicalPosition = GetCanonicalPosition;

	int i, j, temp; /* temp is used for debugging */

	if(kSupportsSymmetries) { /* Initialize gSymmetryMatrix[][] */
		for(i = 0; i < BOARD_SIZE; i++) {
			temp = i;
			for(j = 0; j < NUMSYMMETRIES; j++) {
				if(j == NUMSYMMETRIES/2)
					temp = gFlipNewPosition[i];
				if(j < NUMSYMMETRIES/2)
					temp = gSymmetryMatrix[j][i] = gRotate90CWNewPosition[temp];
				else
					temp = gSymmetryMatrix[j][i] = gRotate90CWNewPosition[temp];
			}
		}
	}

	/**************************************************/
	/**************** SYMMETRY FUN END ****************/
	/**************************************************/

	PositionToBlankOX(gInitialPosition, gPosition.board);
	gPosition.nextPiece = x;
	gPosition.piecesPlaced = 0;

	gMoveToStringFunPtr = &MoveToString;
	gActualNumberOfPositionsOptFunPtr = &ActualNumberOfPositions;
}

void FreeGame()
{
}

/************************************************************************
**
** NAME:        DebugMenu
**
** DESCRIPTION: Menu used to debub internal problems. Does nothing if
**              kDebugMenu == false
**
************************************************************************/

void DebugMenu()
{
	int tttppm();
	char GetMyChar();

	do {
		printf("\n\t----- Module DEBUGGER for %s -----\n\n", kGameName);

		printf("\tc)\tWrite PPM to s(C)reen\n");
		printf("\ti)\tWrite PPM to f(I)le\n");
		printf("\ts)\tWrite Postscript to (S)creen\n");
		printf("\tf)\tWrite Postscript to (F)ile\n");
		printf("\n\n\tb)\t(B)ack = Return to previous activity.\n");
		printf("\n\nSelect an option: ");

		switch(GetMyChar()) {
		case 'Q': case 'q':
			ExitStageRight();
			break;
		case 'H': case 'h':
			HelpMenus();
			break;
		case 'C': case 'c': /* Write PPM to s(C)reen */
			tttppm(0,0);
			break;
		case 'I': case 'i': /* Write PPM to f(I)le */
			tttppm(0,1);
			break;
		case 'S': case 's': /* Write Postscript to (S)creen */
			tttppm(1,0);
			break;
		case 'F': case 'f': /* Write Postscript to (F)ile */
			tttppm(1,1);
			break;
		case 'B': case 'b':
			return;
		default:
			BadMenuChoice();
			HitAnyKeyToContinue();
			break;
		}
	} while(true);

}

/************************************************************************
**
** NAME:        GameSpecificMenu
**
** DESCRIPTION: Menu used to change game-specific parmeters, such as
**              the side of the board in an nxn Nim board, etc. Does
**              nothing if kGameSpecificMenu == false
**
************************************************************************/

void GameSpecificMenu() {
}

// Anoto pen support - implemented in core/pen/pttt.c
extern void gPenHandleTclMessage(int options[], char *filename, Tcl_Interp *tclInterp, int debug);

/************************************************************************
**
** NAME:        SetTclCGameSpecificOptions
**
** DESCRIPTION: Set the C game-specific options (called from Tcl)
**              Ignore if you don't care about Tcl for now.
**
************************************************************************/

#ifndef NO_GRAPHICS
void SetTclCGameSpecificOptions(int theOptions[])
{
	// Anoto pen support
	if ((gPenFile != NULL) && (gTclInterp != NULL)) {
		gPenHandleTclMessage(theOptions, gPenFile, gTclInterp, gPenDebug);
	}
}
#endif

/************************************************************************
**
** NAME:        DoMove
**
** DESCRIPTION: Apply the move to the position.
**
** INPUTS:      Position position : The old position
**              Move     move     : The move to apply.
**
** OUTPUTS:     (Position) : The position that results after the move.
**
** CALLS:       PositionToBlankOX(Position,*BlankOX)
**              BlankOX WhosTurn(*BlankOX)
**
************************************************************************/

Position DoMove(Position position, Move move)
{
	if (gUseGPS) {
		gPosition.board[move] = gPosition.nextPiece;
		gPosition.nextPiece = gPosition.nextPiece == x ? o : x;
		++gPosition.piecesPlaced;

		return BlankOXToPosition(gPosition.board);
	}
	else {
		BlankOX board[BOARD_SIZE];

		PositionToBlankOX(position, board);

		return position + g3Array[move] * (int) WhoseTurn(board);
	}
}

/************************************************************************
**
** NAME:        GetInitialPosition
**
** DESCRIPTION: Ask the user for an initial position for testing. Store
**              it in the space pointed to by initialPosition;
**
** OUTPUTS:     Position initialPosition : The position to fill.
**
************************************************************************/

Position GetInitialPosition()
{
	BlankOX theBlankOX[BOARD_SIZE];
	signed char c;
	int i;


	printf("\n\n\t----- Get Initial Position -----\n");
	printf("\n\tPlease input the position to begin with.\n");
	printf("\tNote that it should be in the following format:\n\n");
	printf("O - -\nO - -            <----- EXAMPLE \n- X X\n\n");

	i = 0;
	getchar();
	while(i < BOARD_SIZE && (c = getchar()) != EOF) {
		if(c == 'x' || c == 'X')
			theBlankOX[i++] = x;
		else if(c == 'o' || c == 'O' || c == '0')
			theBlankOX[i++] = o;
		else if(c == '-')
			theBlankOX[i++] = Blank;
		/* else do nothing */
	}

	/*
	   getchar();
	   printf("\nNow, whose turn is it? [O/X] : ");
	   scanf("%c",&c);
	   if(c == 'x' || c == 'X')
	   whosTurn = x;
	   else
	   whosTurn = o;
	 */

	return(BlankOXToPosition(theBlankOX));
}

/************************************************************************
**
** NAME:        PrintComputersMove
**
** DESCRIPTION: Nicely format the computers move.
**
** INPUTS:      Move   *computersMove : The computer's move.
**              char * computersName : The computer's name.
**
************************************************************************/

void PrintComputersMove(computersMove,computersName)
Move computersMove;
char *computersName;
{
	printf("%8s's move              : %2d\n", computersName, computersMove+1);
}

/************************************************************************
**
** NAME:        Primitive
**
** DESCRIPTION: Return the value of a position if it fulfills certain
**              'primitive' constraints. Some examples of this is having
**              three-in-a-row with TicTacToe. TicTacToe has two
**              primitives it can immediately check for, when the board
**              is filled but nobody has one = primitive tie. Three in
**              a row is a primitive lose, because the player who faces
**              this board has just lost. I.e. the player before him
**              created the board and won. Otherwise undecided.
**
** INPUTS:      Position position : The position to inspect.
**
** OUTPUTS:     (VALUE) an enum which is oneof: (win,lose,tie,undecided)
**
** CALLS:       bool ThreeInARow()
**              bool AllFilledIn()
**              PositionToBlankOX()
**
************************************************************************/

VALUE Primitive(Position position)
{
	if (!gUseGPS)
		PositionToBlankOX(position, gPosition.board); // Temporary storage.

	if (ThreeInARow(gPosition.board, 0, 1, 2) ||
	    ThreeInARow(gPosition.board, 3, 4, 5) ||
	    ThreeInARow(gPosition.board, 6, 7, 8) ||
	    ThreeInARow(gPosition.board, 0, 3, 6) ||
	    ThreeInARow(gPosition.board, 1, 4, 7) ||
	    ThreeInARow(gPosition.board, 2, 5, 8) ||
	    ThreeInARow(gPosition.board, 0, 4, 8) ||
	    ThreeInARow(gPosition.board, 2, 4, 6))
		return gStandardGame ? lose : win;
	else if ((gUseGPS && (gPosition.piecesPlaced == BOARD_SIZE)) ||
	         ((!gUseGPS) && AllFilledIn(gPosition.board)))
		return tie;
	else
		return undecided;
}

/************************************************************************
**
** NAME:        PrintPosition
**
** DESCRIPTION: Print the position in a pretty format, including the
**              prediction of the game's outcome.
**
** INPUTS:      Position position   : The position to pretty print.
**              char *  playerName : The name of the player.
**              bool  usersTurn  : true <==> it's a user's turn.
**
** CALLS:       PositionToBlankOX()
**              GetValueOfPosition()
**              GetPrediction()
**
************************************************************************/

void PrintPosition(position,playerName,usersTurn)
Position position;
char *playerName;
bool usersTurn;
{
	BlankOX theBlankOx[BOARD_SIZE];

	PositionToBlankOX(position,theBlankOx);

	printf("\n         ( 1 2 3 )           : %s %s %s\n",
	       gBlankOXString[(int)theBlankOx[0]],
	       gBlankOXString[(int)theBlankOx[1]],
	       gBlankOXString[(int)theBlankOx[2]] );
	printf("LEGEND:  ( 4 5 6 )  TOTAL:   : %s %s %s\n",
	       gBlankOXString[(int)theBlankOx[3]],
	       gBlankOXString[(int)theBlankOx[4]],
	       gBlankOXString[(int)theBlankOx[5]] );
	printf("         ( 7 8 9 )           : %s %s %s %s\n\n",
	       gBlankOXString[(int)theBlankOx[6]],
	       gBlankOXString[(int)theBlankOx[7]],
	       gBlankOXString[(int)theBlankOx[8]],
	       GetPrediction(position,playerName,usersTurn));
}

/************************************************************************
**
** NAME:        GenerateMoves
**
** DESCRIPTION: Create a linked list of every move that can be reached
**              from this position. Return a pointer to the head of the
**              linked list.
**
** INPUTS:      Position position : The position to branch off of.
**
** OUTPUTS:     (MOVELIST *), a pointer that points to the first item
**              in the linked list of moves that can be generated.
**
** CALLS:       MOVELIST *CreateMovelistNode(Move,MOVELIST *)
**
************************************************************************/

MOVELIST *GenerateMoves(Position position)
{
	int index;
	MOVELIST *moves = NULL;

	if (!gUseGPS)
		PositionToBlankOX(position, gPosition.board); // Temporary storage.

	for (index = 0; index < BOARD_SIZE; ++index)
		if (gPosition.board[index] == Blank)
			moves = CreateMovelistNode(index, moves);

	return moves;
}

/**************************************************/
/**************** SYMMETRY FUN BEGIN **************/
/**************************************************/

/************************************************************************
**
** NAME:        GetCanonicalPosition
**
** DESCRIPTION: Go through all of the positions that are symmetrically
**              equivalent and return the SMALLEST, which will be used
**              as the canonical element for the equivalence set.
**
** INPUTS:      Position position : The position return the canonical elt. of.
**
** OUTPUTS:     Position          : The canonical element of the set.
**
************************************************************************/

Position GetCanonicalPosition(Position position)
{
	Position newPosition, theCanonicalPosition, DoSymmetry();
	int i;

	theCanonicalPosition = position;

	for(i = 0; i < NUMSYMMETRIES; i++) {

		newPosition = DoSymmetry(position, i); /* get new */
		if(newPosition < theCanonicalPosition) /* THIS is the one */
			theCanonicalPosition = newPosition; /* set it to the ans */
	}

	return(theCanonicalPosition);
}

/************************************************************************
**
** NAME:        DoSymmetry
**
** DESCRIPTION: Perform the symmetry operation specified by the input
**              on the position specified by the input and return the
**              new position, even if it's the same as the input.
**
** INPUTS:      Position position : The position to branch the symmetry from.
**              int      symmetry : The number of the symmetry operation.
**
** OUTPUTS:     Position, The position after the symmetry operation.
**
************************************************************************/

Position DoSymmetry(Position position, int symmetry)
{
	int i;
	BlankOX theBlankOx[BOARD_SIZE], symmBlankOx[BOARD_SIZE];
	Position BlankOXToPosition();

	PositionToBlankOX(position,theBlankOx);
	PositionToBlankOX(position,symmBlankOx); /* Make copy */

	/* Copy from the symmetry matrix */

	for(i = 0; i < BOARD_SIZE; i++)
		symmBlankOx[i] = theBlankOx[gSymmetryMatrix[symmetry][i]];

	return(BlankOXToPosition(symmBlankOx));
}

/**************************************************/
/**************** SYMMETRY FUN END ****************/
/**************************************************/


/************************************************************************
**
** NAME:        PositionToBlankOX
**
** DESCRIPTION: convert an internal position to that of a BlankOX.
**
** INPUTS:      Position thePos     : The position input.
**              BlankOX *theBlankOx : The converted BlankOX output array.
**
** CALLS:       BadElse()
**
************************************************************************/

void PositionToBlankOX(thePos,theBlankOX)
Position thePos;
BlankOX *theBlankOX;
{
	int i;
	for(i = 8; i >= 0; i--) {
		if(thePos >= (Position)(x * g3Array[i])) {
			theBlankOX[i] = x;
			thePos -= x * g3Array[i];
		}
		else if(thePos >= (Position)(o * g3Array[i])) {
			theBlankOX[i] = o;
			thePos -= o * g3Array[i];
		}
		else if(thePos >= (Position)(Blank * g3Array[i])) {
			theBlankOX[i] = Blank;
			thePos -= Blank * g3Array[i];
		}
		else
			BadElse("PositionToBlankOX");
	}
}

/************************************************************************
**
** NAME:        BlankOXToPosition
**
** DESCRIPTION: convert a BlankOX to that of an internal position.
**
** INPUTS:      BlankOX *theBlankOx : The converted BlankOX output array.
**
** OUTPUTS:     Position: The equivalent position given the BlankOX.
**
************************************************************************/

Position BlankOXToPosition(theBlankOX)
BlankOX *theBlankOX;
{
	int i;
	Position position = 0;

	for(i = 0; i < BOARD_SIZE; i++)
		position += g3Array[i] * (int)theBlankOX[i]; /* was (int)position... */

	return(position);
}

/************************************************************************
**
** NAME:        ThreeInARow
**
** DESCRIPTION: Return true iff there are three-in-a-row.
**
** INPUTS:      BlankOX theBlankOX[BOARD_SIZE] : The BlankOX array.
**              int a,b,c                     : The 3 positions to check.
**
** OUTPUTS:     (bool) true iff there are three-in-a-row.
**
************************************************************************/

bool ThreeInARow(BlankOX theBlankOX[] theBlankOX, int a, int b, int c)
{
	return        theBlankOX[a] == theBlankOX[b] &&
	              theBlankOX[b] == theBlankOX[c] &&
	              theBlankOX[c] != Blank;
}

/************************************************************************
**
** NAME:        AllFilledIn
**
** DESCRIPTION: Return true iff all the blanks are filled in.
**
** INPUTS:      BlankOX theBlankOX[BOARD_SIZE] : The BlankOX array.
**
** OUTPUTS:     (bool) true iff all the blanks are filled in.
**
************************************************************************/

bool AllFilledIn(BlankOX theBlankOX[] theBlankOX)
{
	bool answer = true;
	int i;

	for(i = 0; i < BOARD_SIZE; i++)
		answer &= (theBlankOX[i] == o || theBlankOX[i] == x);

	return(answer);
}

/************************************************************************
**
** NAME:        WhoseTurn
**
** DESCRIPTION: Return whose turn it is - either x or o. Since x always
**              goes first, we know that if the board has an equal number
**              of x's and o's, that it's x's turn. Otherwise it's o's.
**
** INPUTS:      BlankOX theBlankOX : The input board
**
** OUTPUTS:     (BlankOX) : Either x or o, depending on whose turn it is
**
************************************************************************/

BlankOX WhoseTurn(BlankOX *theBlankOX theBlankOX)
{
	int i, xcount = 0, ocount = 0;

	for(i = 0; i < BOARD_SIZE; i++)
		if(theBlankOX[i] == x)
			xcount++;
		else if(theBlankOX[i] == o)
			ocount++;
		/* else don't count blanks */

	if(xcount == ocount)
		return(x);  /* in our TicTacToe, x always goes first */
	else
		return(o);
}

int NumberOfOptions()
{
	return 2;
}

int getOption()
{
	int option = 0;
	option += gStandardGame;
	option *= 2;
	option += gSymmetries;
	return option+1;
}

void setOption(int option)
{
	option -= 1;
	gSymmetries = option % 2;
	option /= 2;
	gStandardGame = option;
}
