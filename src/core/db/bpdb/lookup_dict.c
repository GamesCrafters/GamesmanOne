#include "core/db/bpdb/lookup_dict.h"

#include <assert.h>  // assert
#include <stddef.h>  // NULL
#include <stdint.h>  // int32_t
#include <stdio.h>   // fprintf, stderr
#include <stdlib.h>  // realloc, free
#include <string.h>  // memset

static const int32_t kCompDictSizeMax = (INT32_MAX - 1) / 2 + 1;
static const int32_t kDecompDictCapacityMax = (INT32_MAX - 1) / 2 + 1;

static int ExpandCompDict(LookupDict *dict, int32_t key);
static int ExpandDecompDict(LookupDict *dict);

// -----------------------------------------------------------------------------

int LookupDictInit(LookupDict *dict) {
    memset(dict, 0, sizeof(*dict));

    int error = ExpandCompDict(dict, 0);
    if (error != 0) {
        memset(dict, 0, sizeof(*dict));
        return error;
    }

    error = ExpandDecompDict(dict);
    if (error != 0) {
        memset(dict, 0, sizeof(*dict));
        return error;
    }

    dict->num_unique = 1;
    dict->comp_dict[0] = 0;
    dict->decomp_dict[0] = 0;
    return 0;
}

void LookupDictDestroy(LookupDict *dict) {
    free(dict->comp_dict);
    free(dict->decomp_dict);
    memset(dict, 0, sizeof(*dict));
}

// Assumes KEY is not in DICT.
int LookupDictSet(LookupDict *dict, int32_t key) {
    if (dict->comp_dict_size <= key) {
        int error = ExpandCompDict(dict, key);
        if (error == 1) {
            fprintf(
                stderr,
                "LookupDictSet: failed to realloc compression dictionary\n");
            return 1;
        } else if (error == 2) {
            fprintf(
                stderr,
                "LookupDictSet: compression dictionary size exceeds limit\n");
            return 2;
        }
    }

    if (dict->num_unique + 1 > dict->decomp_dict_capacity) {
        int error = ExpandDecompDict(dict);
        if (error == 1) {
            fprintf(
                stderr,
                "LookupDictSet: failed to realloc decompression dictionary\n");
            return 1;
        } else if (error == 2) {
            fprintf(stderr,
                    "LookupDictSet: decompression dictionary capacity exceeds "
                    "limit\n");
            return 2;
        }
    }

    assert (dict->comp_dict[key] < 0);
    dict->comp_dict[key] = dict->num_unique;
    dict->decomp_dict[dict->num_unique++] = key;

    return 0;
}

int32_t LookupDictGet(const LookupDict *dict, int32_t key) {
    if (key >= dict->comp_dict_size) return -1;
    return dict->comp_dict[key];
}

int32_t LookupDictGetKey(const LookupDict *dict, int32_t value) {
    assert(value < dict->num_unique);
    return dict->decomp_dict[value];
}

// -----------------------------------------------------------------------------

static int ExpandCompDict(LookupDict *dict, int32_t key) {
    int32_t new_size = dict->comp_dict_size;

    if (dict->comp_dict_size == 0) new_size = 1;
    while (new_size <= key) {
        if (new_size >= kCompDictSizeMax) return 2;
        new_size *= 2;
    }

    int32_t *new_comp_dict =
        (int32_t *)realloc(dict->comp_dict, new_size * sizeof(int32_t));
    if (new_comp_dict == NULL) return 1;

    for (int32_t i = dict->comp_dict_size; i < new_size; ++i) {
        new_comp_dict[i] = -1;
    }

    dict->comp_dict = new_comp_dict;
    dict->comp_dict_size = new_size;
    return 0;
}

static int ExpandDecompDict(LookupDict *dict) {
    if (dict->decomp_dict_capacity >= kDecompDictCapacityMax) return 2;

    int32_t new_capacity = dict->decomp_dict_capacity * 2;
    if (dict->decomp_dict_capacity == 0) new_capacity = 1;

    int32_t *new_decomp_dict =
        (int32_t *)realloc(dict->decomp_dict, new_capacity * sizeof(int32_t));
    if (new_decomp_dict == NULL) return 1;

    dict->decomp_dict = new_decomp_dict;
    dict->decomp_dict_capacity = new_capacity;
    return 0;
}
