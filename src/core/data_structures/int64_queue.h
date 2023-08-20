/**
 * @file int64_queue.h
 * @author Robert Shi (robertyishi@berkeley.edu)
 *         GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief int64_t queue using dynamic array.
 * @version 1.0
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

#ifndef GAMESMANEXPERIMENT_CORE_DATA_STRUCTURES_INT64_QUEUE_H_
#define GAMESMANEXPERIMENT_CORE_DATA_STRUCTURES_INT64_QUEUE_H_

#include <stdbool.h>  // bool
#include <stdint.h>   // int64_t

/**
 * @brief int64_t queue using dynamic array.
 *
 * @example
 * #include <inttypes.h>
 * #include <stdio.h>
 *
 * Int64Queue myqueue;
 * Int64QueueInit(&myqueue);
 * Int64QueuePush(&myqueue, -1);
 * Int64QueuePush(&myqueue, 2);
 * printf("%" PRId64, Int64QueuePop(&myqueue));  // -1
 * Int64QueuePush(&myqueue, -3);
 * printf("%" PRId64, Int64QueuePop(&myqueue));  // 2
 * printf("%" PRId64, Int64QueuePop(&myqueue));  // -3
 * Int64QueueDestroy(&myqueue);
 */
typedef struct Int64Queue {
    int64_t *array;   /**< Internal dynamic array storing the items. */
    int64_t front;    /**< Index to the element at the front of the queue. */
    int64_t size;     /**< Number of elements in the queue. */
    int64_t capacity; /**< Current capacity of the queue. */
} Int64Queue;

/**
 * @brief Initializes QUEUE.
 */
void Int64QueueInit(Int64Queue *queue);

/**
 * @brief Deallocates QUEUE.
 */
void Int64QueueDestroy(Int64Queue *queue);

/**
 * @brief Returns true if QUEUE is empty, false otherwise.
 */
bool Int64QueueIsEmpty(const Int64Queue *queue);

/**
 * @brief Returns the number of items in the queue.
 */
int64_t Int64QueueSize(const Int64Queue *queue);

/**
 * @brief Pushes ELEMENT to the back of the QUEUE and returns true. Returns
 * false on failure.
 */
bool Int64QueuePush(Int64Queue *queue, int64_t item);

/**
 * @brief Pops the item at the front of the QUEUE and returns it.
 */
int64_t Int64QueuePop(Int64Queue *queue);

#endif  // GAMESMANEXPERIMENT_CORE_DATA_STRUCTURES_INT64_QUEUE_H_
