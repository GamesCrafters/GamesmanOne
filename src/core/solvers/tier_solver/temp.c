#include "common.h"
#include "misc.h"
#include "solvermpi.h"
#include "tiersolver.h"
#include "tiertree.h"
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#define MPI_MANAGER_NODE 0
#define MPI_MSG_TAG 0
#define MPI_STAT_TAG 1
#define MPI_MSG_LEN (TIER_STR_LENGTH_MAX + 1)

/* Global statistics. Used by the manager node and worker nodes. */
static tier_solver_stat_t globalStat;

/* The following constants are used only by the manager node. */
static TierTreeEntryList *solvableTiersHead = NULL;
static tier_tree_entry_t *solvableTiersTail = NULL;
static TierTreeEntryList *solvingTiers = NULL;
static int solvedTiers = 0;
static int skippedTiers = 0;
static int failedTiers = 0;
struct timeval globalStartTime, globalEndTime;
struct timeval intervalStart, intervalEnd;
double msgTime = 0.0;

static tier_tree_entry_t *get_tail(TierTreeEntryList *list) {
    if (!list) return NULL;
    while (list->next)  list = list->next;
    return list;
}

static void print_stat(tier_solver_stat_t stat) {
    printf("total legal positions: %"PRIu64"\n", stat.numLegalPos);
    printf("number of winning positions: %"PRIu64"\n", stat.numWin);
    printf("number of losing positions: %"PRIu64"\n", stat.numLose);
    printf("number of drawing positions: %"PRIu64"\n",
        stat.numLegalPos - stat.numWin - stat.numLose);
    printf("longest win for red is %"PRIu64" steps at position %"PRIu64
        "\n", stat.longestNumStepsToRedWin, stat.longestPosToRedWin);
    printf("longest win for black is %"PRIu64" steps at position %"PRIu64
        "\n", stat.longestNumStepsToBlackWin, stat.longestPosToBlackWin);
}

static void update_global_stat(tier_solver_stat_t stat) {
    globalStat.numWin += stat.numWin;
    globalStat.numLose += stat.numLose;
    globalStat.numLegalPos += stat.numLegalPos;
    if (stat.longestNumStepsToRedWin > globalStat.longestNumStepsToRedWin) {
        globalStat.longestNumStepsToRedWin = stat.longestNumStepsToRedWin;
        globalStat.longestPosToRedWin = stat.longestPosToRedWin;
    }
    if (stat.longestNumStepsToBlackWin > globalStat.longestNumStepsToBlackWin) {
        globalStat.longestNumStepsToBlackWin = stat.longestNumStepsToBlackWin;
        globalStat.longestPosToBlackWin = stat.longestPosToBlackWin;
    }
}

static void solvable_tiers_append(tier_tree_entry_t *e) {
    if (!solvableTiersHead) {
        solvableTiersHead = solvableTiersTail = e;
    } else {
        solvableTiersTail->next = e;
        solvableTiersTail = e;
    }
    e->next = NULL;
}

static void solvable_tiers_remove_head(void) {
    if (!solvableTiersHead) return;
    tier_tree_entry_t *toRemove = solvableTiersHead;
    solvableTiersHead = toRemove->next;
    free(toRemove);
    if (!solvableTiersHead) solvableTiersTail = NULL;
}

/* Update the tier tree and the solvable tier list by removing
   the SOLVEDTIER from its canonical parent tiers' number of
   unsolved children. If this number reaches zero, the parent
   tier is removed from the tier tree and appended to the end
   of the solvable tier list. */
static void update_tier_tree(const char *solvedTier) {
    tier_tree_entry_t *canonicalParentTierTreeEntry;
    TierList *parentTiers = tier_get_parent_tier_list(solvedTier);
    TierList *canonicalParents = NULL;
    for (struct TierListElem *walker = parentTiers; walker; walker = walker->next) {
        /* Update canonical parent's number of unsolved children only. */
        struct TierListElem *canonicalParent = tier_get_canonical_tier(walker->tier);
        if (tier_list_contains(canonicalParents, canonicalParent->tier)) {
            /* It is possible that a child has two parents that are symmetrical
               to each other. In this case, we should only decrement the child
               counter once. */
            free(canonicalParent);
            continue;
        }
        canonicalParent->next = canonicalParents;
        canonicalParents = canonicalParent;

        canonicalParentTierTreeEntry = tier_tree_find(canonicalParent->tier);
        if (canonicalParentTierTreeEntry && --canonicalParentTierTreeEntry->numUnsolvedChildren == 0) {
            canonicalParentTierTreeEntry = tier_tree_remove(canonicalParent->tier);
            solvable_tiers_append(canonicalParentTierTreeEntry);
        }
    }
    tier_list_destroy(canonicalParents);
    tier_list_destroy(parentTiers);
}

static void remove_tier_from_solving(const char *tier) {
    if (!solvingTiers) {
        /* This should never happen. */
        printf("remove_tier_from_solving: removing from empty list.\n");
        return;
    }
    if (strncmp(tier, solvingTiers->tier, TIER_STR_LENGTH_MAX) == 0) {
        /* Special case: remove head. */
        tier_tree_entry_t *toRemove = solvingTiers;
        solvingTiers = toRemove->next;
        free(toRemove);
        return;
    }
    tier_tree_entry_t *prev = solvingTiers;
    tier_tree_entry_t *curr = solvingTiers->next;
    while (curr && strncmp(curr->tier, tier, TIER_STR_LENGTH_MAX)) {
        prev = curr;
        curr = curr->next;
    }
    if (!curr) {
        /* This should never happen. */
        printf("remove_tier_from_solving: tier to remove is not found"
            " in the solving tiers list.\n");
        return;
    }
    prev->next = curr->next;
    free(curr);
}

static void move_solvable_head_to_solving(void) {
    if (!solvableTiersHead) {
        /* This should never happen. */
        printf("move_solvable_head_to_solving: solvable list is empty.\n");
        return;
    }
    tier_tree_entry_t *detached = solvableTiersHead;
    solvableTiersHead = detached->next;
    detached->next = solvingTiers;
    solvingTiers = detached;
    if (!solvableTiersHead) solvableTiersTail = NULL;
}

static double get_elapsed_time(struct timeval start, struct timeval end) {
    double elapsed_time = (start.tv_sec - end.tv_sec) * 1000000.0; // convert seconds to microseconds
    elapsed_time += (start.tv_usec - end.tv_usec); // add microseconds
    elapsed_time /= 1000000.0; // convert back to seconds
    return elapsed_time;
}

static void timed_send(void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm) {
    gettimeofday(&intervalStart, NULL);
    MPI_Send(buf, count, datatype, dest, tag, comm);
    gettimeofday(&intervalEnd, NULL);
    msgTime += get_elapsed_time(intervalStart, intervalEnd);
}

static void timed_recv(void *buf, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Status *status) {
    gettimeofday(&intervalStart, NULL);
    MPI_Recv(buf, count, datatype, source, tag, comm, status);
    gettimeofday(&intervalEnd, NULL);
    msgTime += get_elapsed_time(intervalStart, intervalEnd);
}

static void manager_solve_all(void) {
    MPI_Status status;
    char buf[MPI_MSG_LEN];

    /* Loop until all solvable tiers are solved. */
    while (solvableTiersHead || solvingTiers) {
        timed_recv(buf, MPI_MSG_LEN, MPI_INT8_T, MPI_ANY_SOURCE, MPI_MSG_TAG, MPI_COMM_WORLD, &status);
        if (strncmp(buf, "check", MPI_MSG_LEN) != 0) {
            /* Received solver result from a worker node. */
            if (buf[0] != '!') {
                /* Solve succeeded, update tier tree and solvable tier list. */
                printf("Process %d successfully solved %s.\n", status.MPI_SOURCE, buf);
                update_tier_tree(buf);
                remove_tier_from_solving(buf);
                ++solvedTiers;
            } else {
                /* Solve failed due to OOM. */
                printf("Process %d failed to solve %s.\n", status.MPI_SOURCE, buf);
                remove_tier_from_solving(buf + 1);
                ++failedTiers;
            }
        }
        /* The worker node that we received a message from is now idle. */

        /* Keep popping off non-nanonical tiers from the head of the solvable tier list
           until we see the first canonical one or the list becomes empty. */
        while (solvableTiersHead && !tier_is_canonical_tier(solvableTiersHead->tier)) {
            ++skippedTiers;
            solvable_tiers_remove_head();
        }
        if (solvableTiersHead) {
            /* A solvable tier is available, dispatch it to the worker node. */
            printf("Dispatching %s to process %d.\n", solvableTiersHead->tier, status.MPI_SOURCE);
            memcpy(buf, solvableTiersHead->tier, TIER_STR_LENGTH_MAX);
            move_solvable_head_to_solving();
        } else {
            /* No solvable tiers available, let the worker node go to sleep. */
            snprintf(buf, MPI_MSG_LEN, "sleep");
        }
        timed_send(buf, MPI_MSG_LEN, MPI_INT8_T, status.MPI_SOURCE, MPI_MSG_TAG, MPI_COMM_WORLD);
    }
}

static void manager_terminate_workers(void) {
    MPI_Status status;
    char buf[MPI_MSG_LEN];
    int clusterSize;
    int terminated = 0;

    MPI_Comm_size(MPI_COMM_WORLD, &clusterSize);
    while (terminated < (clusterSize - 1)) { // All nodes except the manager node.
        timed_recv(buf, MPI_MSG_LEN, MPI_INT8_T, MPI_ANY_SOURCE, MPI_MSG_TAG, MPI_COMM_WORLD, &status);
        sprintf(buf, "terminate");
        timed_send(buf, MPI_MSG_LEN, MPI_INT8_T, status.MPI_SOURCE, MPI_MSG_TAG, MPI_COMM_WORLD);
        tier_solver_stat_t stat;
        timed_recv(&stat, sizeof(stat), MPI_INT8_T, status.MPI_SOURCE, MPI_STAT_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        update_global_stat(stat);
        ++terminated;
    }
}

/* Assumes MPI_Init has already been called. */
void solve_mpi_manager(uint8_t nPiecesMax, uint64_t nthread, uint64_t mem) {
    gettimeofday(&globalStartTime, NULL); // record start time
    if (nPiecesMax == 255) {
        make_triangle();
        solvableTiersHead = tier_tree_init_from_file("../endgames", mem);
    } else {
        solvableTiersHead = tier_tree_init(nPiecesMax, nthread);
    }
    solvableTiersTail = get_tail(solvableTiersHead);

    manager_solve_all();
    manager_terminate_workers();

    printf("solve_mpi_manager: finished solving all tiers with less than or equal to %d pieces:\n"
        "Number of canonical tiers solved: %d\n"
        "Number of non-canonical tiers skipped: %d\n"
        "Number of tiers failed due to OOM: %d\n"
        "Total tiers scanned: %d\n",
        2 + nPiecesMax, solvedTiers, skippedTiers, failedTiers, solvedTiers + skippedTiers + failedTiers);
    print_stat(globalStat);
    printf("\n");

    gettimeofday(&globalEndTime, NULL); // record end time
    printf("Elapsed time: %f seconds.\n", get_elapsed_time(globalStartTime, globalEndTime));
    printf("Time wasted on messaging: %f seconds.\n", msgTime);
}

/* Assumes MPI_Init has already been called. */
void solve_mpi_worker(uint64_t mem, bool force) {
    char buf[MPI_MSG_LEN] = "check";
    char tier[TIER_STR_LENGTH_MAX];
    make_triangle();

    /* Spin forever until a "terminate" message is recieved from the manager node. */
    while (true) {
        MPI_Send(buf, MPI_MSG_LEN, MPI_INT8_T, MPI_MANAGER_NODE, MPI_MSG_TAG, MPI_COMM_WORLD);
        MPI_Recv(buf, MPI_MSG_LEN, MPI_INT8_T, MPI_MANAGER_NODE, MPI_MSG_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        if (strncmp(buf, "sleep", MPI_MSG_LEN) == 0) {
            /* No work to do. Wait for one second and check again. */
            sleep(1);
            snprintf(buf, MPI_MSG_LEN, "check");
        } else if (strncmp(buf, "terminate", MPI_MSG_LEN) == 0) {
            /* Terminate signal recieved from manager node.
               Send statistics and exit loop. */
            MPI_Send(&globalStat, sizeof(globalStat), MPI_INT8_T, MPI_MANAGER_NODE, MPI_STAT_TAG, MPI_COMM_WORLD);
            break;
        } else {
            /* Assmues the received string contains a valid tier
               that is now ready to be solved. */
            memcpy(tier, buf, TIER_STR_LENGTH_MAX);
            tier_solver_stat_t stat = tiersolver_solve_tier(tier, mem, force);
            if (stat.numLegalPos) {
                /* Solve succeeded. Update global statistics. */
                update_global_stat(stat);
            } else {
                /* Solve failed due to OOM. */
                snprintf(buf, MPI_MSG_LEN, "!%s", tier);
            }
        }
    }
}
