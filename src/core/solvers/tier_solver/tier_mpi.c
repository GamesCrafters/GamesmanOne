/**
 * @file tier_mpi.c
 * @author Robert Shi (robertyishi@berkeley.edu)
 *         GamesCrafters Research Group, UC Berkeley
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

#include "core/solvers/tier_solver/tier_mpi.h"

#include <mpi.h>      // MPI_*
#include <stdbool.h>  // bool

#include "core/misc.h"
#include "core/types/gamesman_types.h"

static const int kMpiDefaultTag = 0;
static const int kMpiManagerRank = 0;

void TierMpiManagerSendSolve(int dest, Tier tier, bool force) {
    TierMpiManagerMessage msg = {
        .command = force ? kTierMpiCommandForceSolve : kTierMpiCommandSolve,
        .tier = tier,
    };
    SafeMpiSend(&msg, sizeof(msg), MPI_UINT8_T, dest, kMpiDefaultTag,
                MPI_COMM_WORLD);
}

void TierMpiManagerSendSleep(int dest) {
    TierMpiManagerMessage msg = {.command = kTierMpiCommandSleep};
    SafeMpiSend(&msg, sizeof(msg), MPI_UINT8_T, dest, kMpiDefaultTag,
                MPI_COMM_WORLD);
}

void TierMpiManagerSendTerminate(int dest) {
    TierMpiManagerMessage msg = {.command = kTierMpiCommandTerminate};
    SafeMpiSend(&msg, sizeof(msg), MPI_UINT8_T, dest, kMpiDefaultTag,
                MPI_COMM_WORLD);
}

void TierMpiManagerRecvAnySource(TierMpiWorkerMessage *dest, int *src_rank) {
    MPI_Status status;
    SafeMpiRecv(dest, sizeof(*dest), MPI_UINT8_T, MPI_ANY_SOURCE,
                kMpiDefaultTag, MPI_COMM_WORLD, &status);
    *src_rank = status.MPI_SOURCE;
}

void TierMpiWorkerSendCheck(void) {
    TierMpiWorkerMessage msg = {.request = kTierMpiRequestCheck};
    SafeMpiSend(&msg, sizeof(msg), MPI_UINT8_T, kMpiManagerRank, kMpiDefaultTag,
                MPI_COMM_WORLD);
}

void TierMpiWorkerSendReportSolved(void) {
    TierMpiWorkerMessage msg = {.request = kTierMpiRequestReportSolved};
    SafeMpiSend(&msg, sizeof(msg), MPI_UINT8_T, kMpiManagerRank, kMpiDefaultTag,
                MPI_COMM_WORLD);
}

void TierMpiWorkerSendReportLoaded(void) {
    TierMpiWorkerMessage msg = {.request = kTierMpiRequestReportLoaded};
    SafeMpiSend(&msg, sizeof(msg), MPI_UINT8_T, kMpiManagerRank, kMpiDefaultTag,
                MPI_COMM_WORLD);
}

void TierMpiWorkerSendReportError(int error) {
    TierMpiWorkerMessage msg = {
        .request = kTierMpiRequestReportError,
        .error = error,
    };
    SafeMpiSend(&msg, sizeof(msg), MPI_UINT8_T, kMpiManagerRank, kMpiDefaultTag,
                MPI_COMM_WORLD);
}

void TierMpiWorkerRecv(TierMpiManagerMessage *dest) {
    SafeMpiRecv(dest, sizeof(*dest), MPI_UINT8_T, kMpiManagerRank,
                kMpiDefaultTag, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
}
