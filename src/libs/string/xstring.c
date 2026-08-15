#include "libs/string/xstring.h"

#include <regex.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

void AppendSnprintf(char *__restrict buffer, size_t max_len, size_t *offset,
                    const char *__restrict format, ...) {
    // Early exit if invalid pointers or if a previous call failed with an
    // encoding error
    if (buffer == NULL || offset == NULL || *offset == SIZE_MAX) {
        return;
    }

    if (*offset >= max_len) {
        return;  // Buffer is already full or offset is out of bounds
    }

    size_t remain = max_len - *offset;

    va_list args;
    va_start(args, format);
    int n = vsnprintf(buffer + *offset, remain, format, args);
    va_end(args);

    if (n < 0) {
        *offset = SIZE_MAX;
    } else if ((size_t)n >= remain) {
        // If truncation occurred, clamp the offset to the last valid character
        // index
        *offset = max_len - 1;
    } else {
        *offset += (size_t)n;  // Advance offset by the number of bytes written
    }
}

bool RegexMatch(const char *pattern, const char *target) {
    if (!pattern || !target) {
        return false;
    }

    char msgbuf[128];

    // 1. Compile the regular expression
    // REG_EXTENDED allows modern regex syntax (like {1,3}, +, etc.)
    // REG_NOSUB tells the compiler we don't need to capture subgroups,
    // which speeds up execution.
    regex_t regex;
    int rc = regcomp(&regex, pattern, REG_EXTENDED | REG_NOSUB);  // Return code
    if (rc) {
        regerror(rc, &regex, msgbuf, sizeof(msgbuf));
        fprintf(stderr, "Could not compile regex pattern: %s\n", msgbuf);
        return false;
    }

    // 2. Execute the regular expression
    bool is_match = false;
    rc = regexec(&regex, target, 0, NULL, 0);
    if (!rc) {
        is_match = true;
    } else if (rc != REG_NOMATCH) {
        regerror(rc, &regex, msgbuf, sizeof(msgbuf));
        fprintf(stderr, "Regex match failed: %s\n", msgbuf);
    }

    // 3. Free the memory allocated by regcomp
    regfree(&regex);

    return is_match;
}
