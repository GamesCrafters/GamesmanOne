/**
 * @file generic_hash.c
 * @author Dan Garcia: Designer of the original (3-variable only) version
 * @author Attila Gyulassy: Developer of the original (3-variable only) version
 * @author Michel D'Sa: Designer and developer of user-specified variables
 * version
 * @author Scott Lindeneau: Designer and developer of multiple contexts;
 * optimized for efficiency and readability.
 * @author Robert Shi (robertyishi@berkeley.edu): further optimized for
 * efficiency and readability, adapted to the new system and implemented
 * thread-safety
 *         GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Implementation of the Generic Hash system for finite board games with
 * fixed sets of pieces.
 * @version 1.0.2
 * @date 2024-03-08
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

#include "core/generic_hash/generic_hash.h"

#include <assert.h>   // assert
#include <stdbool.h>  // bool, true, false
#include <stdint.h>   // int64_t
#include <stdio.h>    // fprintf, stderr
#include <stdlib.h>   // free, realloc
#include <string.h>   // memset

#include "core/data_structures/int64_hash_map.h"
#include "core/generic_hash/context.h"
#include "core/types/gamesman_types.h"

typedef struct ContextManager {
    GenericHashContext *contexts;
    int64_t size;
    int64_t capacity;
    Int64HashMap labels;
} ContextManager;
static ContextManager manager;

static bool multi_context_warning_shown;

static bool ManagerExpand(void);
static bool ManagerCheckUniqueContext(void);
static int64_t ManagerGetContextIndex(int64_t context_label);

// -----------------------------------------------------------------------------

void GenericHashReinitialize(void) {
    if (manager.contexts != NULL) {
        for (int64_t i = 0; i < manager.size; ++i) {
            GenericHashContextDestroy(&manager.contexts[i]);
        }
        free(manager.contexts);
        Int64HashMapDestroy(&manager.labels);
    }
    memset(&manager, 0, sizeof(manager));
    Int64HashMapInit(&manager.labels, 0.5);
    multi_context_warning_shown = false;
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

// Convenience functions for working with only one context.

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

int GenericHashGetTurn(Position hash) {
    if (!ManagerCheckUniqueContext()) return -1;
    return GenericHashContextGetTurn(&manager.contexts[0], hash);
}

// Multi-context hashing and unhashing functions.

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

int GenericHashGetTurnLabel(int64_t context_label, Position hash) {
    int64_t context_index = ManagerGetContextIndex(context_label);
    if (context_index < 0) return -1;
    return GenericHashContextGetTurn(&manager.contexts[context_index], hash);
}

// -----------------------------------------------------------------------------

static bool ManagerExpand(void) {
    int64_t new_capacity = manager.capacity == 0 ? 1 : manager.capacity * 2;
    GenericHashContext *new_contexts = (GenericHashContext *)realloc(
        manager.contexts, sizeof(GenericHashContext) * new_capacity);
    if (new_contexts == NULL) return false;
    manager.contexts = new_contexts;
    manager.capacity = new_capacity;
    return true;
}

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

static int64_t ManagerGetContextIndex(int64_t context_label) {
    Int64HashMapIterator it = Int64HashMapGet(&manager.labels, context_label);
    if (!Int64HashMapIteratorIsValid(&it)) return -1;
    return Int64HashMapIteratorValue(&it);
}
