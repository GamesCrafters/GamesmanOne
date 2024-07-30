/**
 * @file base.h
 * @author Robert Shi (robertyishi@berkeley.edu)
 *         GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Enumeration of errors.
 *
 * @version 1.0.1
 * @date 2024-02-02
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

#ifndef GAMESMANONE_CORE_GAMESMAN_ERROR_H_
#define GAMESMANONE_CORE_GAMESMAN_ERROR_H_

enum GamesmanError {
    kNoError = 0,              /**< No error should always be 0. */
    kMallocFailureError,       /**< Malloc-like function returned NULL. */
    kNotImplementedError,      /**< Feature not implemented. */
    kNotReachedError,          /**< Reaching a branch marked as unreached. */
    kIntegerOverflowError,     /**< Integer overflow. */
    kMemoryOverflowError,      /**< Memory overflow. */
    kFileSystemError,          /**< A file system call returned error. */
    kIllegalArgumentError,     /**< Illegal argument passed into function. */
    kIllegalGameNameError,     /**< Illegal game name encountered. */
    kIllegalGameVariantError,  /**< Illegal game variant encountered. */
    kIllegalGameTierError,     /**< Illegal game tier encountered. */
    kIllegalGamePositionError, /**< Illegal game position encountered. */
    kIllegalGamePositionValueError, /**< Illegal position value encountered. */
    kIllegalGameTierGraphError,     /**< Tier graph contains a loop. */
    kIllegalSolverOptionError,      /**< Illegal solver option encountered. */
    kIncompleteGameplayApiError,    /**< Gameplay API not fully implemented. */
    kGameInitFailureError,          /**< Game initialization failed. */
    kUseBeforeInitializationError,  /**< Module used before initialization. */
    kMpiError,                      /**< Error in MPI function. */
    kHeadlessError,                 /**< Headless command parsing error. */
    kGenericHashError,              /**< Generic Hash system error. */
    kRuntimeError,                  /**< Generic runtime error. */
};

#endif  // GAMESMANONE_CORE_GAMESMAN_ERROR_H_
