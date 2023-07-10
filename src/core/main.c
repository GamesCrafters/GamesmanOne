#include <stdio.h>

#include "gamesman.h"
#include "games/mttt/mttt.h"

int main(void) {
    printf("main() begins.\n");

    MtttInitialize();
    
    SolverSolve();

    return 0;
}
