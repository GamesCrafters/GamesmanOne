#include "core/interactive/automenu.h"

#include <ctype.h>    // tolower
#include <stdbool.h>  // bool, true, false
#include <stddef.h>   // NULL
#include <stdio.h>    // printf, fgets, getchar
#include <stdlib.h>   // exit, EXIT_SUCCESS
#include <string.h>   // strncmp, strchr

#include "core/misc.h"  // GamesmanExit

static void FormatInput(char *input) {
    // Convert all characters to lowercase.
    for (int i = 0; input[i] != '\0'; ++i) {
        input[i] = tolower(input[i]);
    }
}

static bool StringEqual(ReadOnlyString s1, ReadOnlyString s2, size_t n) {
    return (strncmp(s1, s2, n) == 0);
}

int AutoMenu(ReadOnlyString title, int num_items, ConstantReadOnlyString *items,
             ConstantReadOnlyString *keys, const HookFunctionPointer *hooks,
             void (*Update)(void)) {
    while (1) {
        // Update menu contents if necessary.
        if (Update != NULL) Update();

        // Print menu.
        printf("\n\t----- %s -----\n\n", title);
        for (int i = 0; i < num_items; ++i) {
            printf("\t%s) %s\n", keys[i], items[i]);
        }
        printf("\n\tb) Go back\n");
        printf("\tq) Quit\n\n");

        // Prompt for input.
        bool accepted = false;
        do {
            static char input[kKeyLengthMax + 2];
            PromptForInput("", input, kKeyLengthMax);
            FormatInput(input);
            if (StringEqual(input, "b", kKeyLengthMax)) return 0;
            if (StringEqual(input, "q", kKeyLengthMax)) GamesmanExit();

            for (int i = 0; i < num_items; ++i) {
                if (StringEqual(input, keys[i], kKeyLengthMax)) {
                    accepted = true;
                    int ret = hooks[i](input);
                    if (ret > 0) return ret - 1;
                    break;
                }
            }
            if (!accepted) printf("Invalid key. Please enter again.\n");
        } while (!accepted);
    }

    return 0;
}
