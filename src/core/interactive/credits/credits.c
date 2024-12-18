#include "core/interactive/credits/credits.h"

#include <stdio.h>

#include "core/types/gamesman_types.h"

int InteractiveCredits(ReadOnlyString key) {
    (void)key;
    printf("\n\t----- Credits -----\n\n");
    printf("Open Source Software Usage:\n");
    printf(
        "  - JSON-C - A JSON implementation in C: "
        "https://github.com/json-c/json-c\n");

    return 0;
}
