/**
 * @file int64_queue.h
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Dynamic int64_t queue.
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

#ifndef GAMESMANONE_CORE_DATA_STRUCTURES_INT64_QUEUE_H_
#define GAMESMANONE_CORE_DATA_STRUCTURES_INT64_QUEUE_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "core/gamesman_memory.h"

/**
 * @brief int64_t queue using dynamic array.
 */
typedef struct Int64Queue {
    int64_t *array;   /**< Internal dynamic array storing the items. */
    int64_t front;    /**< Index to the element at the front of the queue. */
    int64_t size;     /**< Number of elements in the queue. */
    int64_t capacity; /**< Current capacity of the queue. */
} Int64Queue;

/** @brief Initializes QUEUE. */
static inline void Int64QueueInit(Int64Queue *queue) {
    queue->array = NULL;
    queue->capacity = 0;
    queue->front = 0;
    queue->size = 0;
}

/** @brief Destroys QUEUE. */
static inline void Int64QueueDestroy(Int64Queue *queue) {
    GamesmanFree(queue->array);
    queue->array = NULL;
    queue->capacity = 0;
    queue->front = 0;
    queue->size = 0;
}

/** @brief Returns true if QUEUE is empty, or false otherwise. */
static inline bool Int64QueueIsEmpty(const Int64Queue *queue) {
    return (queue->size == 0);
}

/** @brief Returns the number of items in QUEUE. */
static inline int64_t Int64QueueSize(const Int64Queue *queue) {
    return queue->size;
}

/**
 * @brief Pushes ELEMENT into the QUEUE.
 *
 * @return true on success,
 * @return false otherwise.
 */
bool Int64QueuePush(Int64Queue *queue, int64_t item);

/** @brief Pops the item at the front of the QUEUE and returns it. */
static inline int64_t Int64QueuePop(Int64Queue *queue) {
    int64_t element = queue->array[queue->front];
    queue->front = (queue->front + 1) % queue->capacity;
    --queue->size;

    return element;
}

/** @brief Returns the item at the front of the QUEUE without popping it. */
static inline int64_t Int64QueueFront(const Int64Queue *queue) {
    return queue->array[queue->front];
}

#endif  // GAMESMANONE_CORE_DATA_STRUCTURES_INT64_QUEUE_H_
