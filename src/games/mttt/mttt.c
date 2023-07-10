#include "mttt.h"

#include <stdbool.h>
#include <stdio.h>

#include "gamesman.h"
#include "gamesman_types.h"

/* Mandatory API */

MoveList MtttGenerateMoves(Position position);
Value MtttPrimitive(Position position);
Position MtttDoMove(Position position, Move move);

/* API required to enable undo-moves. */

PositionArray MtttParentPositions(Position position);

void MtttInitialize(void) {
    global_num_positions = 19683; // 3**9.
    GenerateMoves = &MtttGenerateMoves;
    Primitive = &MtttPrimitive;
    DoMove = &MtttDoMove;
    ParentPositions = &MtttParentPositions;
}


/* API Implementation. */


