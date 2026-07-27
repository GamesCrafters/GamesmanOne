/**
 * @file int64_array.h
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Dynamic int64_t array.
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

#include <stdbool.h>
#include <stdint.h>

#include "core/gamesman_memory.h"

/**
 * @brief Dynamic `int64_t` array.
 */
typedef struct Int64Array {
    GamesmanAllocator *allocator; /**< Allocator to use. */
    int64_t *array;               /**< The actual array. */
    int64_t size;                 /**< Number of items in the array. */
    int64_t capacity;             /**< Current capacity of the array. */
} Int64Array;

/**
 * @brief Initializes `array`.
 *
 * @param[out] array Array to initialize.
 */
void Int64ArrayInit(Int64Array *array);

/**
 * @brief Initializes `array` using `allocator` as the underlying memory
 * allocator.
 *
 * @details If `allocator` is `NULL`, the function call is equivalent to
 * `Int64ArrayInit(array)`. Note that this function does not transfer the
 * ownership of `allocator` to the new array object. The caller is responsible
 * for releasing its own copy of the allocator.
 *
 * @param[out] array Array to initialize.
 * @param[in,out] allocator Memory allocator to use.
 */
void Int64ArrayInitAllocator(Int64Array *array, GamesmanAllocator *allocator);

/**
 * @brief Initializes `dest` array to be a copy of the `src` array.
 *
 * @details If `src` uses a custom memory allocator, a new reference will be
 * copied to the `dest` array.
 *
 * @param[out] dest Array to initialize.
 * @param[in] src Source array to copy from.
 *
 * @retval true on success.
 * @retval false otherwise.
 */
bool Int64ArrayInitCopy(Int64Array *dest, const Int64Array *src);

/**
 * @brief Deallocates `array`.
 *
 * @param[in,out] array Array to deallocate.
 */
void Int64ArrayDestroy(Int64Array *array);

/**
 * @brief Pushes a new `item` to the back of the `array`.
 *
 * @param[in,out] array Destination array.
 * @param[in] item New item.
 *
 * @retval true on success.
 * @retval false otherwise.
 */
bool Int64ArrayPushBack(Int64Array *array, int64_t item);

/**
 * @brief Pops the item at the back of the `array`.
 *
 * @details Calling this function on an empty `array` results in undefined
 * behavior.
 *
 * @param[in,out] array Array to pop the item from.
 */
void Int64ArrayPopBack(Int64Array *array);

/**
 * @brief Returns the item at the back of `array`.
 *
 * @details Calling this function on an empty `array` results in undefined
 * behavior.
 *
 * @param[in] array Array to get the item from.
 *
 * @return Item at the back of `array`.
 */
int64_t Int64ArrayBack(const Int64Array *array);

/**
 * @brief Returns whether the given `array` is empty.
 *
 * @param[in] array Array to check.
 *
 * @retval true if the array is empty.
 * @retval false otherwise.
 */
bool Int64ArrayEmpty(const Int64Array *array);

/**
 * @brief Returns whether the given `array` contains the given `item`.
 *
 * @param[in] array Array to check.
 * @param[in] item Item to look for.
 *
 * @retval true if the array contains the item.
 * @retval false otherwise.
 */
bool Int64ArrayContains(const Int64Array *array, int64_t item);

/**
 * @brief Sorts the given `array` according to the given comparison function.
 *
 * @param[in,out] array The array to be sorted.
 * @param[in] comp Comparison function which returns a negative integer value if
 * the first argument is less than the second, a positive integer value if the
 * first argument is greater than the second, and zero if the arguments are
 * equivalent.
 */
void Int64ArraySortExplicit(Int64Array *array,
                            int (*comp)(const void *, const void *));

/**
 * @brief Resizes `array` to have `size` elements.
 *
 * @details If the current size of `array` is greater than `size`, the content
 * is reduced to its first `size` elements. If the current size of `array` is
 * less than `size`, zeros shall be inserted to the back of the array.
 *
 * @param[in,out] array Array to resize, assumed to be initialized.
 * @param[in] size New size of the array.
 *
 * @retval true on success.
 * @retval false otherwise.
 */
bool Int64ArrayResize(Int64Array *array, int64_t size);

/**
 * @brief Removes the first occurrence of `item` from `array`, if it exists.
 *
 * @param[in,out] array Array of `int64_t`.
 * @param[in] item Value to remove.
 *
 * @retval true if `item` exists in `array` and was removed.
 * @retval false otherwise.
 */
bool Int64ArrayRemoveUnordered(Int64Array *array, int64_t item);

#endif  // GAMESMANONE_CORE_DATA_STRUCTURES_INT64_ARRAY_H_
