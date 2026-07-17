/**
 * @file xmpi.c
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief MPI wrapper functions implementation.
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

#include "libs/mpi/xmpi.h"

#include <mpi.h>

void SafeMpiInitThread(int *argc, char ***argv, int required, int *provided) {
    int error = MPI_Init_thread(argc, argv, required, provided);
    if (error != MPI_SUCCESS) {
        fprintf(stderr, "SafeMpiInitThread: failed with code %d\n", error);
        fflush(stderr);
        _exit(kMpiError);
    }
}

void SafeMpiInit(int *argc, char ***argv) {
    int error = MPI_Init(argc, argv);
    if (error != MPI_SUCCESS) {
        fprintf(stderr, "SafeMpiInit: failed with code %d\n", error);
        exit(kMpiError);
    }
}

void SafeMpiFinalize(void) {
    int error = MPI_Finalize();
    if (error != MPI_SUCCESS) {
        fprintf(stderr, "SafeMpiFinalize: failed with code %d\n", error);
        fflush(stderr);
        _exit(kMpiError);
    }
}

int SafeMpiCommSize(MPI_Comm comm) {
    int ret;
    int error = MPI_Comm_size(comm, &ret);
    if (error != MPI_SUCCESS) {
        fprintf(stderr, "SafeMpiCommSize: failed with code %d\n", error);
        fflush(stderr);
        _exit(kMpiError);
    }

    return ret;
}

int SafeMpiCommRank(MPI_Comm comm) {
    int ret;
    int error = MPI_Comm_rank(comm, &ret);
    if (error != MPI_SUCCESS) {
        fprintf(stderr, "SafeMpiCommRank: failed with code %d\n", error);
        fflush(stderr);
        _exit(kMpiError);
    }

    return ret;
}

void SafeMpiSend(void *buf, int count, MPI_Datatype datatype, int dest, int tag,
                 MPI_Comm comm) {
    int error = MPI_Send(buf, count, datatype, dest, tag, comm);
    if (error != MPI_SUCCESS) {
        fprintf(stderr, "SafeMpiSend: failed with code %d\n", error);
        fflush(stderr);
        _exit(kMpiError);
    }
}

void SafeMpiRecv(void *buf, int count, MPI_Datatype datatype, int source,
                 int tag, MPI_Comm comm, MPI_Status *status) {
    int error = MPI_Recv(buf, count, datatype, source, tag, comm, status);
    if (error != MPI_SUCCESS) {
        fprintf(stderr, "SafeMpiRecv: failed with code %d\n", error);
        fflush(stderr);
        _exit(kMpiError);
    }
}
