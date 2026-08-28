/**
 * @file int64_queue.c
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Implementation of Int64Queue.
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

#include "core/data_structures/int64_queue.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "core/gamesman_memory.h"

static bool Expand(Int64Queue *queue) {
    int64_t new_capacity = queue->capacity == 0 ? 16 : queue->capacity * 2;
    int64_t *new_array =
        (int64_t *)GamesmanMalloc(new_capacity * sizeof(int64_t));
    if (!new_array) {
        return false;
    }

    if (queue->array) {
        memcpy(new_array, queue->array, queue->capacity * sizeof(int64_t));
        GamesmanFree(queue->array);
    }

    // Copy wrapped-around elements in the original array. This is safe if
    // the capacity is at least doubled.
    memcpy(&new_array[queue->capacity], new_array,
           queue->front * sizeof(int64_t));
    queue->array = new_array;
    queue->capacity = new_capacity;

    return true;
}

// Function to add an element to the queue
bool Int64QueuePush(Int64Queue *queue, int64_t item) {
    if (queue->size == queue->capacity) {
        if (!Expand(queue)) {
            return false;
        }
    }

    int64_t back = (queue->front + queue->size) % queue->capacity;
    queue->array[back] = item;
    ++queue->size;

    return true;
}
