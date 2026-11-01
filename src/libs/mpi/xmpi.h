/**
 * @file xmpi.h
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Library of MPI wrapper functions.
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

#ifndef GAMESMANONE_LIBS_MPI_XMPI_H_
#define GAMESMANONE_LIBS_MPI_XMPI_H_

#include <mpi.h>

/**
 * @brief Bail-on-error \c MPI_Init_thread.
 *
 * @param argc Pointer to the number of arguments.
 * @param argv Pointer to the argument vector.
 * @param required Level of desired thread support.
 * @param provided (Output parameter) level of provided thread support.
 */
void SafeMpiInitThread(int *argc, char ***argv, int required, int *provided);

/**
 * @brief Bail-on-error \c MPI_Init.
 *
 * @param argc Pointer to the number of arguments.
 * @param argv Pointer to the argument vector.
 */
void SafeMpiInit(int *argc, char ***argv);

/**
 * @brief Bail-on-error \c MPI_Finalize.
 */
void SafeMpiFinalize(void);

/**
 * @brief Bail-on-error \c MPI_Comm_size.
 *
 * @param comm Communicator (handle).
 * @return Number of processes in the group of \p comm.
 */
int SafeMpiCommSize(MPI_Comm comm);

/**
 * @brief Bail-on-error \c MPI_Comm_rank.
 *
 * @param comm Communicator (handle).
 * @return Rank of the calling process in the group of \c comm.
 */
int SafeMpiCommRank(MPI_Comm comm);

/**
 * @brief Bail-on-error \c MPI_Send.
 *
 * @param buf Initial address of send buffer (choice).
 * @param count Number of elements in send buffer (non-negative integer).
 * @param datatype Datatype of each send buffer element (handle).
 * @param dest Rank of destination (integer).
 * @param tag Message tag (integer).
 * @param comm Communicator (handle).
 */
void SafeMpiSend(void *buf, int count, MPI_Datatype datatype, int dest, int tag,
                 MPI_Comm comm);

/**
 * @brief Bail-on-error \c MPI_Recv.
 *
 * @param buf (Output parameter) initial address of receive buffer (choice).
 * @param count Communicator (handle).
 * @param datatype Maximum number of elements in receive buffer (integer).
 * @param source Datatype of each receive buffer element (handle).
 * @param tag Rank of source (integer).
 * @param comm Message tag (integer).
 * @param status (Output parameter) status object.
 */
void SafeMpiRecv(void *buf, int count, MPI_Datatype datatype, int source,
                 int tag, MPI_Comm comm, MPI_Status *status);

#endif  // GAMESMANONE_LIBS_MPI_XMPI_H_
