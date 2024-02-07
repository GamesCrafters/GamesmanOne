#ifdef USE_MPI

#ifndef GAMESMANONE_CORE_SOLVERS_TIER_SOLVER_TIER_MPI_H_
#define GAMESMANONE_CORE_SOLVERS_TIER_SOLVER_TIER_MPI_H_

#include <stdbool.h>  // bool

#include "core/types/gamesman_types.h"

enum TierMpiConstants {
    kMpiDefaultTag = 0,
    kMpiManagerRank = 0,
};

enum TierMpiCommands {
    kTierMpiCommandSolve,
    kTierMpiCommandForceSolve,
    kTierMpiCommandSleep,
    kTierMpiCommandTerminate,
};

enum TierMpiRequests {
    kTierMpiRequestCheck,
    kTierMpiRequestReportSolved,
    kTierMpiRequestReportLoaded,
    kTierMpiRequestReportError,
};

typedef struct TierMpiManagerMessage {
    Tier tier;
    int command;
} TierMpiManagerMessage;

typedef struct TierMpiWorkerMessage {
    int request;
    int error;
} TierMpiWorkerMessage;

void TierMpiManagerSendSolve(int dest, Tier tier, bool force);
void TierMpiManagerSendSleep(int dest);
void TierMpiManagerSendTerminate(int dest);
void TierMpiManagerRecvAnySource(TierMpiWorkerMessage *dest, int *src_rank);

void TierMpiWorkerSendCheck(void);
void TierMpiWorkerSendReportSolved(void);
void TierMpiWorkerSendReportLoaded(void);
void TierMpiWorkerSendReportError(int error);
void TierMpiWorkerRecv(TierMpiWorkerMessage *dest);

#endif  // GAMESMANONE_CORE_SOLVERS_TIER_SOLVER_TIER_MPI_H_

#endif  // USE_MPI
