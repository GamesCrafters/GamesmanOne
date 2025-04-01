/**
 * @file int64_queue.c
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Implementation of int64_t queue using dynamic array.
 * @version 1.0.0
 * @date 2023-08-19
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

#include <assert.h>   // assert
#include <stdbool.h>  // bool, true, false
#include <stddef.h>   // NULL
#include <stdint.h>   // int64_t
#include <stdio.h>    // fprintf, stderr
#include <string.h>   // memcpy

#include "core/gamesman_memory.h"

static bool Expand(Int64Queue *queue) {
    int64_t new_capacity = queue->capacity == 0 ? 1 : queue->capacity * 2;
    int64_t *new_array = (int64_t *)GamesmanRealloc(
        queue->array, queue->capacity * sizeof(int64_t),
        new_capacity * sizeof(int64_t));
    if (new_array == NULL) return false;
    memcpy(&new_array[queue->capacity], new_array,
           queue->front * sizeof(int64_t));
    queue->array = new_array;
    queue->capacity = new_capacity;
    return true;
}

// Function to initialize the queue
void Int64QueueInit(Int64Queue *queue) {
    queue->array = NULL;
    queue->capacity = 0;
    queue->front = 0;
    queue->size = 0;
}

// Function to destroy the queue and free the allocated memory
void Int64QueueDestroy(Int64Queue *queue) {
    GamesmanFree(queue->array);
    queue->array = NULL;
    queue->capacity = 0;
    queue->front = 0;
    queue->size = 0;
}

// Function to check if the queue is empty
bool Int64QueueIsEmpty(const Int64Queue *queue) { return (queue->size == 0); }

// Function to check the size of the queue
int64_t Int64QueueSize(const Int64Queue *queue) { return queue->size; }

// Function to add an element to the queue
bool Int64QueuePush(Int64Queue *queue, int64_t item) {
    if (queue->size == queue->capacity) {
        if (!Expand(queue)) return false;
    }
    assert(queue->size < queue->capacity);
    int64_t back = (queue->front + queue->size) % queue->capacity;
    queue->array[back] = item;
    ++queue->size;
    return true;
}

// Function to remove an element from the queue
int64_t Int64QueuePop(Int64Queue *queue) {
    if (Int64QueueIsEmpty(queue)) {
        fprintf(stderr, "Int64QueuePop: popping from an empty queue.\n");
        return 0;
    }

    int64_t element = queue->array[queue->front];
    queue->front = (queue->front + 1) % queue->capacity;
    --queue->size;

    return element;
}

int64_t Int64QueueFront(const Int64Queue *queue) {
    if (Int64QueueIsEmpty(queue)) {
        fprintf(stderr, "Int64QueueFront: peaking into an empty queue.\n");
        return 0;
    }

    return queue->array[queue->front];
}
