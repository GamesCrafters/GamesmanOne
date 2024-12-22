/**
 * @file reverse_tier_graph.h
 * @author Max Delgadillo: designed and implemented the original version
 * of tier solver (solveretrograde.c in GamesmanClassic.)
 * @author Robert Shi (robertyishi@berkeley.edu): Extracted the reverse tier
 * graph implementation as a separate module from the tier solver
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief The ReverseTierGraph type.
 * @version 1.1.1
 * @date 2024-12-22
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

#ifndef GAMESMANONE_CORE_SOLVERS_TIER_SOLVER_REVERSE_TIER_GRAPH_H_
#define GAMESMANONE_CORE_SOLVERS_TIER_SOLVER_REVERSE_TIER_GRAPH_H_

#include <stdint.h>  // int64_t

#include "core/types/gamesman_types.h"

/**
 * @brief A reverse graph of the tier graph which is built during the tier
 * discovery phase of tier solving and is used to find parents of a tier.
 */
typedef struct ReverseTierGraph {
    /**
     * An array of TierArrays where each array stores the parents of a tier.
     * Note that this array is not index by tier value. Instead, it is sorted in
     * a "first come first in" order, where tiers being discovered first in the
     * discovery phase is pushed into this array first. Use the
     * ReverseTierGraph::index_of hash map to find the index of a tier in this
     * array. */
    TierArray *parents_of;

    /**
     * The index of tiers in the ReverseTierGraph::parents_of array as a hash
     * map.
     */
    TierHashMap index_of;

    int64_t size;     /**< Number of tiers in the tier graph. */
    int64_t capacity; /**< Capacity of the tier graph (private). */
} ReverseTierGraph;

/**
 * @brief Initializes the given tier GRAPH.
 *
 * @param graph Tier graph to initialize.
 */
void ReverseTierGraphInit(ReverseTierGraph *graph);

/**
 * @brief Destroys the given tier GRAPH.
 *
 * @param graph Tier graph to destroy.
 */
void ReverseTierGraphDestroy(ReverseTierGraph *graph);

/**
 * @brief Adds PARENT as a tier parent to tier CHILD to the given tier GRAPH.
 *
 * @param graph Tier graph to add the parent-child pair to.
 * @param child Child tier.
 * @param parent Parent tier.
 * @return 0 on success, non-zero error code otherwise.
 */
int ReverseTierGraphAdd(ReverseTierGraph *graph, Tier child, Tier parent);

/**
 * @brief Pops the parent tier array of tier CHILD from GRAPH. The caller of
 * this function is responsible for destroying the TierArray returned.
 *
 * @note The returned TierArray is removed from graph. Hence this function
 * should only be called once on each CHILD tier. Future calls to this function
 * with the same CHILD argument will result in undefined behavior.
 *
 * @param graph Tier graph to pop the array from.
 * @param child Child tier.
 * @return An array of parent tiers of tier CHILD.
 */
TierArray ReverseTierGraphPopParentsOf(ReverseTierGraph *graph, Tier child);

/**
 * @brief Returns a copy of the parent tier array of tier CHILD from GRAPH. The
 * caller of this function is responsible for destroying the TierArray returned.
 *
 * @param graph Tier graph to get the array from.
 * @param child Child tier.
 * @return An array of parent tiers of tier CHILD.
 */
TierArray ReverseTierGraphGetParentsOf(ReverseTierGraph *graph, Tier child);

#endif  // GAMESMANONE_CORE_SOLVERS_TIER_SOLVER_REVERSE_TIER_GRAPH_H_
