/**
 * @file int64_queue.h
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @author GamesCrafters Research Group, UC Berkeley
 * @brief Dynamic int64_t queue.
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
 * @brief `int64_t` queue using dynamic array.
 *
 * @details No internal states are intended to be inspect or manipulated
 * directly. Use the provided API functions instead.
 */
typedef struct Int64Queue {
    int64_t *array; /**< Internal dynamic array storing the items. */
    int64_t front;  /**< Index to the element at the front of the queue. */
    int64_t size;   /**< Number of elements in the queue. */
    uint64_t capacity_mask; /**< Current capacity of the queue - 1. */
} Int64Queue;

/**
 * @brief Initializes `queue`.
 *
 * @param[out] queue Pointer to the queue to initialize.
 */
static inline void Int64QueueInit(Int64Queue *queue) {
    queue->array = NULL;
    queue->front = 0;
    queue->size = 0;
    queue->capacity_mask = 0ULL;
}

/**
 * @brief Destroys `queue`.
 *
 * @param[in,out] queue Pointer to the queue to destroy.
 */
static inline void Int64QueueDestroy(Int64Queue *queue) {
    GamesmanFree(queue->array);
    queue->array = NULL;
    queue->front = 0;
    queue->size = 0;
    queue->capacity_mask = 0ULL;
}

/**
 * @brief Returns true if `queue` is empty, or false otherwise.
 *
 * @param[in] queue Pointer to the queue.
 *
 * @retval true If the `queue` is empty.
 * @retval false Otherwise.
 */
static inline bool Int64QueueIsEmpty(const Int64Queue *queue) {
    return (queue->size == 0);
}

/**
 * @brief Returns the number of items in `queue`.
 *
 * @param[in] queue Pointer to the queue.
 *
 * @return The number of elements currently stored in the `queue`.
 */
static inline int64_t Int64QueueSize(const Int64Queue *queue) {
    return queue->size;
}

/**
 * @brief Pushes `item` into the `queue`.
 *
 * @param[in,out] queue Pointer to the queue.
 * @param[in] item The element to push.
 *
 * @retval true On success.
 * @retval false On failure due to memory allocation errors.
 */
bool Int64QueuePush(Int64Queue *queue, int64_t item);

/**
 * @brief Pops the item at the front of the `queue` and returns it.
 *
 * @warning Popping an empty queue results in undefined behavior.
 *
 * @param[in,out] queue Pointer to the queue.
 *
 * @return The item at the front of the `queue`.
 */
static inline int64_t Int64QueuePop(Int64Queue *queue) {
    int64_t element = queue->array[queue->front];
    queue->front = (queue->front + 1) & queue->capacity_mask;
    --queue->size;

    return element;
}

/**
 * @brief Returns the item at the front of the `queue` without popping it.
 *
 * @warning Peeking into an empty queue results in undefined behavior.
 *
 * @param[in] queue Pointer to the queue.
 *
 * @return The item at the front of the `queue`.
 */
static inline int64_t Int64QueueFront(const Int64Queue *queue) {
    return queue->array[queue->front];
}

#endif  // GAMESMANONE_CORE_DATA_STRUCTURES_INT64_QUEUE_H_
