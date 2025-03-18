/**
 * @file int64_array.h
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Dynamic int64_t array.
 * @version 2.0.2
 * @date 2024-12-20
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

#ifndef GAMESMANONE_CORE_DATA_STRUCTURES_INT64_ARRAY_H_
#define GAMESMANONE_CORE_DATA_STRUCTURES_INT64_ARRAY_H_

#include <stdbool.h>  // bool
#include <stdint.h>   // int64_t

/**
 * @brief Dynamic int64_t array.
 *
 * @example
 * #include <stdio.h>
 * #include <inttypes.h>
 *
 * Int64Array array;
 * Int64ArrayInit(&array);
 * Int64ArrayPushBack(&array, 0);
 * Int64ArrayPushBack(&array, -3);
 * Int64ArrayPushBack(&array, 5);
 * for (int64_t i = 0; i < array.size; ++i) {
 *     printf("%" PRId64, array.array[i]);  // 0 -3 5
 * }
 * printf("\n");
 * Int64ArrayDestroy(&array);
 */
typedef struct Int64Array {
    int64_t *array;   /**< The actual array. */
    int64_t size;     /**< Number of items in the array. */
    int64_t capacity; /**< Current capacity of the array. */
} Int64Array;

/**
 * @brief Initializes ARRAY.
 *
 * @param array Array to initialize.
 */
void Int64ArrayInit(Int64Array *array);

/**
 * @brief Initializes DEST array to be a copy of the SRC array.
 *
 * @param dest Array to initialize.
 * @param src Source array to copy from.
 * @return true on success, or
 * @return false otherwise.
 */
bool Int64ArrayInitCopy(Int64Array *dest, const Int64Array *src);

/**
 * @brief Deallocates ARRAY.
 *
 * @param array Array to deallocate.
 */
void Int64ArrayDestroy(Int64Array *array);

/**
 * @brief Pushes a new ITEM to the back of the ARRAY.
 *
 * @param array Destination.
 * @param item New item.
 * @return true on success,
 * @return false otherwise.
 */
bool Int64ArrayPushBack(Int64Array *array, int64_t item);

/**
 * @brief Pops the item at the back of the ARRAY. Calling this function on an
 * empty ARRAY results in undefined behavior.
 *
 * @param array Array to pop the item from.
 */
void Int64ArrayPopBack(Int64Array *array);

/**
 * @brief Returns the item at the back of ARRAY. Calling this function on an
 * empty ARRAY results in undefined behavior.
 *
 * @param array Array to get the item from.
 * @return Item at the back of ARRAY.
 */
int64_t Int64ArrayBack(const Int64Array *array);

/** @brief Returns true if the given ARRAY is empty, or false otherwise. */
bool Int64ArrayEmpty(const Int64Array *array);

/**
 * @brief Returns true if the given ARRAY contains the given ITEM, or false
 * otherwise.
 */
bool Int64ArrayContains(const Int64Array *array, int64_t item);

/** @brief Sorts the given ARRAY in ascending order. */
void Int64ArraySortAscending(Int64Array *array);

/**
 * @brief Sorts the given ARRAY according to the given comparison function.
 * @param array The array to be sorted.
 * @param comp 	Comparison function which returns ​a negative integer value if
 * the first argument is less than the second, a positive integer value if the
 * first argument is greater than the second, and zero if the arguments are
 * equivalent.
 */
void Int64ArraySortExplicit(Int64Array *array,
                            int (*comp)(const void *, const void *));

/**
 * @brief Resizes ARRAY to have SIZE elements. If the current size of ARRAY is
 * greater than SIZE, the content is reduced to its first SIZE elements. If the
 * current size of ARRAY is less than SIZE, zeros shall be inserted to the back
 * of the array.
 *
 * @param array Array to resize, assumed to be initialized.
 * @param size New size of the array.
 * @return true on success,
 * @return false otherwise.
 */
bool Int64ArrayResize(Int64Array *array, int64_t size);

/**
 * @brief Removes the item at index INDEX in ARRAY, if exists.
 *
 * @param array Array of int64_t.
 * @param index Index of the item to remove.
 * @return true if INDEX exists in ARRAY, or
 * @return false otherwise.
 */
bool Int64ArrayRemoveIndex(Int64Array *array, int64_t index);

/**
 * @brief Removes the first occurrence of ITEM from ARRAY, if exists.
 *
 * @param array Array of int64_t.
 * @param item Value to remove.
 * @return true if ITEM exists in ARRAY, or
 * @return false otherwise.
 */
bool Int64ArrayRemove(Int64Array *array, int64_t item);

#endif  // GAMESMANONE_CORE_DATA_STRUCTURES_INT64_ARRAY_H_
