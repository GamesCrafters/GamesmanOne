#include "core/data_structures/cstring.h"

#include <stdbool.h>  // bool, true, false
#include <stddef.h>   // NULL
#include <stdlib.h>   // malloc, free
#include <string.h>   // memset, strlen

bool CStringInit(CString *cstring, const char *src) {
    cstring->length = -1;
    cstring->capacity = -1;
    
    int64_t length = strlen(src);
    cstring->str = (char *)malloc(length + 1);
    if (cstring->str == NULL) return false;

    strcpy(cstring->str, src);
    cstring->length = length;
    cstring->capacity = length + 1;
    return true;
}

void CStringDestroy(CString *cstring) {
    free(cstring->str);
    memset(cstring, 0, sizeof(*cstring));
}
