#include "core/types/uwapi/partmove.h"

#include "core/data_structures/cstring.h"

void PartMoveDestroy(Partmove *pm) {
    CStringDestroy(&pm->autogui_move);
    CStringDestroy(&pm->formal_move);
    CStringDestroy(&pm->from);
    CStringDestroy(&pm->to);
    CStringDestroy(&pm->full);
}
