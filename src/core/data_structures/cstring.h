#ifndef GAMESMANONE_CORE_DATA_STRUCTURES_CSTRING_H_
#define GAMESMANONE_CORE_DATA_STRUCTURES_CSTRING_H_

#include <stdbool.h>  // bool
#include <stdint.h>   // int64_t

typedef struct CString {
    char *str;
    int64_t length;
    int64_t capacity;
} CString;

bool CStringInit(CString *cstring, const char *src);
void CStringDestroy(CString *cstring);

#endif  // GAMESMANONE_CORE_DATA_STRUCTURES_CSTRING_H_
