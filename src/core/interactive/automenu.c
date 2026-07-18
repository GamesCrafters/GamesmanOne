#include "core/interactive/automenu.h"

#include <ctype.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "core/misc.h"
#include "core/types/base.h"
#include "libs/io/xterminal.h"

static void FormatInput(char *input) {
    // Convert all characters to lowercase.
    for (int i = 0; input[i] != '\0'; ++i) {
        input[i] = tolower(input[i]);
    }
}

static bool StringEqual(ReadOnlyString s1, ReadOnlyString s2, size_t n) {
    return (strncmp(s1, s2, n) == 0);
}

static int MaxKeyLength(ConstantReadOnlyString *keys, int num_items) {
    int max_key_len = 1;
    for (int i = 0; i < num_items; ++i) {
        int len = strlen(keys[i]);
        if (len > max_key_len) max_key_len = len;
    }
    return max_key_len;
}

int AutoMenu(ReadOnlyString title, int num_items, ConstantReadOnlyString *items,
             ConstantReadOnlyString *keys, const HookFunctionPointer *hooks,
             void (*Update)(void)) {
    for (;;) {
        // Update menu contents if necessary.
        if (Update != NULL) Update();

        // Print menu.
        printf("\n\t----- %s -----\n\n", title);
        static ConstantReadOnlyString spaces[] = {" ", "  ", "   "};
        const int max_key_len = MaxKeyLength(keys, num_items);
        for (int i = 0; i < num_items; ++i) {
            printf("\t%s)%s%s\n", keys[i],
                   spaces[max_key_len - strlen(keys[i])], items[i]);
        }
        puts("\n\tb) Go back\n");
        puts("\tq) Quit\n\n");

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
            if (!accepted) puts("Invalid key. Please enter again.\n");
        } while (!accepted);
    }

    return 0;
}
