/**
 * @file tier_worker.h
 * @author Max Delgadillo: designed and implemented the original version
 * of tier solver (solveretrograde.c in GamesmanClassic.)
 * @author Robert Shi (robertyishi@berkeley.edu): Separated functions for
 * solving of a single tier into its own module, implemented multithreading
 * using OpenMP, and reformatted functions for readability. New algorithm
 * using value iteration.
 *         GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Worker module for the Loopy Tier Solver.
 * @version 1.4.0
 * @date 2024-09-07
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

#ifndef GAMESMANONE_CORE_SOLVERS_TIER_SOLVER_TIER_WORKER_H_
#define GAMESMANONE_CORE_SOLVERS_TIER_SOLVER_TIER_WORKER_H_

#include <stdbool.h>  // bool

#include "core/solvers/tier_solver/tier_solver.h"
#include "core/types/gamesman_types.h"

/**
 * @brief Initializes the Tier Worker Module using the given API functions.
 *
 * @param api Game-specific implementation of the Tier Solver API functions.
 * @param db_chunk_size Number of positions in each database compression block.
 * @param memlimit Approximate maximum amount of heap memory that can be used by
 * the tier worker.
 */
void TierWorkerInit(const TierSolverApi *api, int64_t db_chunk_size,
                    intptr_t memlimit);

/** @brief Solving methods for \c TierWorkerSolve. */
enum TierWorkerSolveMethod {
    /**
     * @brief Method of simple k-pass tier scanning assuming an immediate tier
     * transition happens at all positions in the solving tier (for all
     * positions P in the solving tier T, no child positions of P are in T.)
     * This also implies that the solving tier is loop-free.
     *
     * @details For each pass, loads as many child tiers of the solving tier as
     * possible into memory and scan the solving tier to update the values using
     * minimax. In the best case, assuming enough memory to load all child tiers
     * at once, the solver finishes in one pass.
     *
     * Worst case runtime: O(N * (V + E)), where N is the number of child tiers
     * of the tier being solved, V is the number of vertices in the position
     * graph of the tier being solved, and E is the number of edges in the said
     * graph. Note that this only happens when there is not enough memory to
     * load more than one child tier at a time. Assuming unlimited memory, the
     * runtime becomes O(V + E).
     *
     * Worst case memory: O(V).
     */
    kTierWorkerSolveMethodImmediateTransition,

    /**
     * @brief Method of backward induction for loopy tiers.
     *
     * @details Starts with all primitive positions and solved positions in
     * child tiers as the frontier. Solve by pushing the frontier up using
     * the reverse position graph of the tier being solved.
     *
     * Worst case runtime: O(V + E), where V is the number of vertices in the
     * reverse position graph of the tier being solved, and E is the number of
     * edges in the said graph.
     *
     * Worst case memory (implicit reverse position graph): O(V).
     * Worst case memory (generated reverse position graph): O(V + E).
     */
    kTierWorkerSolveMethodBackwardInduction,

    /**
     * @brief Method of value iteration for loopy tiers.
     *
     * @details Starts with all legal positions marked as drawing. The first
     * iteration assigns values and remotenesses to all primitive positions.
     * Then, for each subsequent iteration, each legal position is scanned
     * for a possible update on its value and remoteness by examining their
     * child positions. Terminates when the previous iteration makes no update
     * on any position.
     *
     * Worst case runtime: O(R * E), where R is the maximum remoteness of the
     * tier being solved, and E is the number of edges in the position graph of
     * the said tier.
     *
     * Worst case memory: O(V). Note that although the asymptotic memory is the
     * same as the method of backward induction, the actual memory usage is much
     * less in practice due to smaller constant factors.
     */
    kTierWorkerSolveMethodValueIteration,
};

/**
 * @brief Solves the given \p tier using the given \p method.
 *
 * @param method Method to use. See \c TierWorkerSolveMethod for details.
 * @param tier Tier to solve.
 * @param force If set to true, the Module will perform the solving process
 * regardless of the current database status. Otherwise, the solving process is
 * skipped if the Module believes that TIER has been correctly solved already.
 * @param compare If set to true, the Module will compare the newly solved
 * database with the reference database, which is assumed to exist. The function
 * will fail if there is a discrepancy.
 * @param solved If not NULL, a truth value indicating whether the given TIER is
 * actually solved instead of loaded from the existing database will be stored
 * in this variable.
 * @return 0 on success, non-zero error code otherwise.
 */
int TierWorkerSolve(int method, Tier tier, bool force, bool compare,
                    bool *solved);

#ifdef USE_MPI
/**
 * @brief Serve as a MPI worker until terminated.
 *
 * @return kNoError on success, or
 * @return non-zero error code otherwise.
 */
int TierWorkerMpiServe(void);
#endif  // USE_MPI

/**
 * @brief Tests the given TIER.
 *
 * @param tier Tier to test.
 * @param parent_tiers Array of parent tiers of TIER.
 * @param seed Seed for psuedorandom number generator.
 * @return 0 on success or one of the error codes enumerated in
 * TierSolverTestErrors otherwise.
 */
int TierWorkerTest(Tier tier, const TierArray *parent_tiers, long seed);

#endif  // GAMESMANONE_CORE_SOLVERS_TIER_SOLVER_TIER_WORKER_H_
