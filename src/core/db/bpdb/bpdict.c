/**
 * @file bpdict.c
 * @author Dan Garcia: designed the "lookup table" compression algorithm
 * @author Max Fierro: improved the algorithm for BpArray compression
 * @author Sameer Nayyar: improved the algorithm for BpArray compression
 * @author Robert Shi (robertyishi@berkeley.edu): improved and implemented
 * BpDict.
 *         GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Implementation of compression and decompression dictionaries for
 * Bit-Perfect Array.
 * @version 1.0.1
 * @date 2024-10-16
 *
 * @copyright This file is part of GAMESMAN, The Finite, Two-person
 * Perfect-Information Game Generator released under the GPL:
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "core/db/bpdb/bpdict.h"

#include <assert.h>  // assert
#include <stddef.h>  // NULL
#include <stdint.h>  // int32_t
#include <stdio.h>   // fprintf, stderr
#include <stdlib.h>  // realloc, free
#include <string.h>  // memset

#include "core/types/gamesman_types.h"

static const int32_t kCompDictSizeMax = (INT32_MAX - 1) / 2 + 1;
static const int32_t kDecompDictCapacityMax = (INT32_MAX - 1) / 2 + 1;

static int ExpandCompDict(BpDict *dict, int32_t key);
static int ExpandDecompDict(BpDict *dict);

// -----------------------------------------------------------------------------

int BpDictInit(BpDict *dict) {
    memset(dict, 0, sizeof(*dict));

    int error = ExpandCompDict(dict, 0);
    if (error != 0) {
        BpDictDestroy(dict);
        return error;
    }

    error = ExpandDecompDict(dict);
    if (error != 0) {
        BpDictDestroy(dict);
        return error;
    }

    dict->num_unique = 1;
    dict->comp_dict[0] = 0;
    dict->decomp_dict[0] = 0;
    return kNoError;
}

void BpDictDestroy(BpDict *dict) {
    free(dict->comp_dict);
    free(dict->decomp_dict);
    memset(dict, 0, sizeof(*dict));
}

// Assumes KEY is not in DICT.
int BpDictSet(BpDict *dict, int32_t key) {
    if (dict->comp_dict_size <= key) {
        int error = ExpandCompDict(dict, key);
        if (error == kMallocFailureError) {
            fprintf(stderr,
                    "BpDictSet: failed to realloc compression dictionary\n");
            return error;
        } else if (error == kMemoryOverflowError) {
            fprintf(stderr,
                    "BpDictSet: compression dictionary size exceeds limit\n");
            return error;
        }
    }

    if (dict->num_unique + 1 > dict->decomp_dict_capacity) {
        int error = ExpandDecompDict(dict);
        if (error == kMallocFailureError) {
            fprintf(stderr,
                    "BpDictSet: failed to realloc decompression dictionary\n");
            return error;
        } else if (error == kMemoryOverflowError) {
            fprintf(stderr,
                    "BpDictSet: decompression dictionary capacity exceeds "
                    "limit\n");
            return error;
        }
    }

    assert(dict->comp_dict[key] < 0);
    dict->comp_dict[key] = dict->num_unique;
    dict->decomp_dict[dict->num_unique++] = key;

    return kNoError;
}

int32_t BpDictGet(const BpDict *dict, int32_t key) {
    if (key >= dict->comp_dict_size) return -1;
    return dict->comp_dict[key];
}

int32_t BpDictGetKey(const BpDict *dict, int32_t value) {
    assert(value < dict->num_unique);
    return dict->decomp_dict[value];
}

// -----------------------------------------------------------------------------

static int ExpandCompDict(BpDict *dict, int32_t key) {
    int32_t new_size = dict->comp_dict_size;

    if (dict->comp_dict_size == 0) new_size = 1;
    while (new_size <= key) {
        if (new_size >= kCompDictSizeMax) return kMemoryOverflowError;
        new_size *= 2;
    }

    int32_t *new_comp_dict =
        (int32_t *)realloc(dict->comp_dict, new_size * sizeof(int32_t));
    if (new_comp_dict == NULL) return kMallocFailureError;

    for (int32_t i = dict->comp_dict_size; i < new_size; ++i) {
        new_comp_dict[i] = -1;
    }

    dict->comp_dict = new_comp_dict;
    dict->comp_dict_size = new_size;
    return kNoError;
}

static int ExpandDecompDict(BpDict *dict) {
    if (dict->decomp_dict_capacity >= kDecompDictCapacityMax)
        return kMemoryOverflowError;

    int32_t new_capacity = dict->decomp_dict_capacity * 2;
    if (dict->decomp_dict_capacity == 0) new_capacity = 1;

    int32_t *new_decomp_dict =
        (int32_t *)realloc(dict->decomp_dict, new_capacity * sizeof(int32_t));
    if (new_decomp_dict == NULL) return kMallocFailureError;

    dict->decomp_dict = new_decomp_dict;
    dict->decomp_dict_capacity = new_capacity;
    return kNoError;
}
