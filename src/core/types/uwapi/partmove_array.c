#include "core/types/uwapi/partmove_array.h"

#include <assert.h>  // assert
#include <stddef.h>  // NULL
#include <stdint.h>  // int64_t
#include <stdlib.h>  // realloc, free
#include <string.h>  // memset

#include "core/data_structures/cstring.h"
#include "core/types/gamesman_error.h"
#include "core/types/uwapi/partmove.h"

void PartmoveArrayInit(PartmoveArray *pa) { memset(pa, 0, sizeof(*pa)); }

void PartmoveArrayDestroy(PartmoveArray *pa) {
    for (int64_t i = 0; i < pa->size; ++i) {
        PartMoveDestroy(&pa->array[i]);
    }
    free(pa->array);
    memset(pa, 0, sizeof(*pa));
}

static int PartmoveArrayExpand(PartmoveArray *pa) {
    int64_t new_capacity = (pa->capacity == 0) ? 1 : pa->capacity * 2;
    Partmove *new_array =
        (Partmove *)realloc(pa->array, new_capacity * sizeof(Partmove));
    if (new_array == NULL) return 1;

    pa->array = new_array;
    pa->capacity = new_capacity;

    return 0;
}

int PartmoveArrayEmplaceBack(PartmoveArray *pa, const char *autogui_move,
                             const char *formal_move, const char *from,
                             const char *to, const char *full) {
    // Make sure there is enough space for the new entry in PA.
    if (pa->size == pa->capacity) {
        int error = PartmoveArrayExpand(pa);
        if (error) return kMallocFailureError;
    }
    assert(pa->size < pa->capacity);

    CStringInitCopy(&pa->array[pa->size].autogui_move, autogui_move);
    CStringInitCopy(&pa->array[pa->size].formal_move, formal_move);
    CStringInitCopy(&pa->array[pa->size].from, from);
    CStringInitCopy(&pa->array[pa->size].to, to);
    CStringInitCopy(&pa->array[pa->size].full, full);
    ++pa->size;

    return kNoError;
}
