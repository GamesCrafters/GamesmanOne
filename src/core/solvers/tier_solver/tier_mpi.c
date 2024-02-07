#ifdef USE_MPI

#include "core/solvers/tier_solver/tier_mpi.h"

#include <mpi.h>
#include <stdbool.h>  // bool

#include "core/misc.h"
#include "core/types/gamesman_types.h"

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
    SafeMpiRecv(dest, sizeof(*dest), MPI_UINT8_T, MPI_ANY_SOURCE,
                kMpiDefaultTag, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
}

#endif  // USE_MPI
