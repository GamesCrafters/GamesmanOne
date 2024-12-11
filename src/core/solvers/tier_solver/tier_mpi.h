/**
 * @file tier_mpi.h
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief MPI utilities for the Tier Solver.
 * @version 1.0.1
 * @date 2024-07-11
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

#ifndef GAMESMANONE_CORE_SOLVERS_TIER_SOLVER_TIER_MPI_H_
#define GAMESMANONE_CORE_SOLVERS_TIER_SOLVER_TIER_MPI_H_

#include <stdbool.h>  // bool

#include "core/types/gamesman_types.h"

/** @brief Tier manager to worker MPI commands. */
enum TierMpiCommands {
    kTierMpiCommandSolve,      /**< Solve the provided tier. */
    kTierMpiCommandForceSolve, /**< Force Re-solve the provided tier. */
    kTierMpiCommandSleep,      /**< Sleep for 1 second. */
    kTierMpiCommandTerminate,  /**< Terminate the worker. */
};

/** @brief Tier worker to manager MPI requests. */
enum TierMpiRequests {
    kTierMpiRequestCheck,        /**< Check for available work. */
    kTierMpiRequestReportSolved, /**< Report solved tier. */
    kTierMpiRequestReportLoaded, /**< Report loaded tier from existing DB. */
    kTierMpiRequestReportError,  /**< Report error while solving. */
};

/** @brief Packed manager-to-worker message. */
typedef struct TierMpiManagerMessage {
    /**
     * Tier to solve; ignored if command is not one of \c kTierMpiCommandSolve
     * and \c kTierMpiCommandForceSolve.
     */
    Tier tier;
    int command; /**< Manager-to-worker command. */
} TierMpiManagerMessage;

/** @brief Packed worker-to-manager message. */
typedef struct TierMpiWorkerMessage {
    int request; /**< Worker-to-manager request. */

    /** Error code; ignored if request is not \c kTierMpiRequestReportError. */
    int error;
} TierMpiWorkerMessage;

// -----------------------------------------------------------------------------
// ----------------------------- Manager Utilities -----------------------------
// -----------------------------------------------------------------------------

/**
 * @brief Send a "solve" command to the worker node of rank \p dest.
 *
 * @param dest Rank of the worker node.
 * @param tier Tier to solve.
 * @param force Force re-solve \p tier if set to true.
 */
void TierMpiManagerSendSolve(int dest, Tier tier, bool force);

/**
 * @brief Send a "sleep" command to the worker node of rank \p dest.
 *
 * @param dest Rank of the worker node.
 */
void TierMpiManagerSendSleep(int dest);

/**
 * @brief Send a "terminate" command to the worker node of rank \p dest.
 *
 * @param dest Rank of the worker node.
 */
void TierMpiManagerSendTerminate(int dest);

/**
 * @brief Block until a message is received from a worker node of any rank, then
 * store the received message in \p dest and the rank of the sender in
 * \p src_rank.
 *
 * @param dest (Output parameter) Buffer for received message.
 * @param src_rank (Output parameter) Buffer for the rank of the message sender.
 */
void TierMpiManagerRecvAnySource(TierMpiWorkerMessage *dest, int *src_rank);

// ----------------------------------------------------------------------------
// ----------------------------- Worker Utilities -----------------------------
// ----------------------------------------------------------------------------

/**
 * @brief Send a "check" request to the manager node to check if any tiers are
 * available to solve.
 */
void TierMpiWorkerSendCheck(void);

/**
 * @brief Report that the previously assigned tier has been solved to the
 * manager node.
 */
void TierMpiWorkerSendReportSolved(void);

/**
 * @brief Report that the previously assigned tier has been loaded from existing
 * database to the manager node.
 */
void TierMpiWorkerSendReportLoaded(void);

/**
 * @brief Report an error while solving the previously assigned tier to the
 * manager node.
 *
 * @param error Error code encountered.
 */
void TierMpiWorkerSendReportError(int error);

/**
 * @brief Block until a message is received from the manager node, then store
 * the received message in \p dest.
 *
 * @param dest (Output parameter) Buffer for the received message from the
 * manager node.
 */
void TierMpiWorkerRecv(TierMpiManagerMessage *dest);

#endif  // GAMESMANONE_CORE_SOLVERS_TIER_SOLVER_TIER_MPI_H_
