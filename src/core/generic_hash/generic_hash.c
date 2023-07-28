#include "core/generic_hash/generic_hash.h"

#include <assert.h>   // assert
#include <malloc.h>   // free, realloc
#include <stdbool.h>  // bool, true, false
#include <stdint.h>   // int64_t
#include <stdio.h>    // fprintf, stderr
#include <string.h>   // memset

#include "core/data_structures/int64_hash_map.h"
#include "core/gamesman_types.h"
#include "core/generic_hash/context.h"

typedef struct ContextManager {
    GenericHashContext *contexts;
    int64_t size;
    int64_t capacity;
    Int64HashMap labels;
} ContextManager;
static ContextManager manager;

static bool multi_context_warning_shown;

static bool ManagerExpand(void) {
    int64_t new_capacity = manager.capacity == 0 ? 1 : manager.capacity * 2;
    GenericHashContext *new_contexts = (GenericHashContext *)realloc(
        manager.contexts, sizeof(GenericHashContext) * new_capacity);
    if (new_contexts == NULL) return false;
    manager.contexts = new_contexts;
    manager.capacity = new_capacity;
    return true;
}

bool GenericHashAddContext(
    int player, int board_size, const int *pieces_init_array,
    bool (*IsValidConfiguration)(const int *configuration), int64_t label) {
    // Return false if label already exists.
    if (Int64HashMapContains(&manager.labels, label)) return false;

    if (manager.size == manager.capacity) {
        if (!ManagerExpand()) return false;
    }
    assert(manager.capacity > manager.size);

    GenericHashContext *target = &manager.contexts[manager.size];
    bool success = GenericHashContextInit(
        target, board_size, player, pieces_init_array, IsValidConfiguration);
    if (!success) return false;
    return Int64HashMapSet(&manager.labels, label, manager.size++);
}

void GenericHashReinitialize(void) {
    if (manager.contexts != NULL) {
        for (int64_t i = 0; i < manager.size; ++i) {
            GenericHashContextDestroy(&manager.contexts[i]);
        }
        free(manager.contexts);
        Int64HashMapDestroy(&manager.labels);
    }
    memset(&manager, 0, sizeof(manager));
    multi_context_warning_shown = false;
}

// Convenience functions for working with only one context.

static bool ManagerCheckUniqueContext(void) {
    if (manager.size <= 0) return false;
    if (!multi_context_warning_shown && manager.size > 1) {
        fprintf(stderr,
                "ManagerCheckUniqueContext: warning - a convenience function "
                "which assumes a single-context environment was called in a "
                "multi-context environment. This message will only show up "
                "once.\n");
        multi_context_warning_shown = true;
    }
    return true;
}

Position GenericHashNumPositions(void) {
    if (!ManagerCheckUniqueContext()) return -1;
    return GenericHashContextNumPositions(&manager.contexts[0]);
}

Position GenericHashHash(const char *board, int turn) {
    if (!ManagerCheckUniqueContext()) return -1;
    return GenericHashContextHash(&manager.contexts[0], board, turn);
}

bool GenericHashUnhash(Position hash, char *board) {
    if (!ManagerCheckUniqueContext()) return false;
    return GenericHashContextUnhash(&manager.contexts[0], hash, board);
}

// Multi-context hashing and unhashing functions.

static int64_t ManagerGetContextIndex(int64_t context_label) {
    Int64HashMapIterator it = Int64HashMapGet(&manager.labels, context_label);
    if (!Int64HashMapIteratorIsValid(&it)) return -1;
    return Int64HashMapIteratorValue(&it);
}

Position GenericHashNumPositionsLabel(int64_t context_label) {
    int64_t context_index = ManagerGetContextIndex(context_label);
    if (context_index < 0) return -1;
    return GenericHashContextNumPositions(&manager.contexts[context_index]);
}

Position GenericHashHashLabel(int64_t context_label, const char *board,
                              int turn) {
    int64_t context_index = ManagerGetContextIndex(context_label);
    if (context_index < 0) return -1;
    return GenericHashContextHash(&manager.contexts[context_index], board,
                                  turn);
}

bool GenericHashUnhashLabel(int64_t context_label, Position hash, char *board) {
    int64_t context_index = ManagerGetContextIndex(context_label);
    if (context_index < 0) return false;
    return GenericHashContextUnhash(&manager.contexts[context_index], hash,
                                    board);
}
