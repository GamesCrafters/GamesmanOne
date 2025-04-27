#include "core/interactive/credits/credits.h"

#include <stdio.h>

#include "core/types/gamesman_types.h"

static ConstantReadOnlyString kOpenSource =
    "Open Source Software Usage:\n"
    "  - JSON-C - A JSON implementation in C: "
    "<https://github.com/json-c/json-c>\n"
    "  - XZ Utils: "
    "<https://github.com/tukaani-project/xz>\n"
    "  - LZ4 - Extremely fast compression: "
    "<https://github.com/lz4/lz4>\n";

int InteractiveCredits(ReadOnlyString key) {
    (void)key;
    printf("\n\t----- Credits -----\n\n");
    printf("%s\n", kOpenSource);

    return 0;
}
