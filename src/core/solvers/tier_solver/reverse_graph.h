/**
 * @file reverse_graph.h
 * @author Max Delgadillo: designed and implemented the original version
 * of tier solver (solveretrograde.c in GamesmanClassic.)
 * @author Robert Shi (robertyishi@berkeley.edu): Extracted the reverse graph
 * implementation as a separate module from the tier solver, replaced
 * linked-lists with dynamic arrays for optimization, and implemented
 * thread-safety for the new OpenMP multithreaded tier solver.
 *         GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Declaration of the ReverseGraph type.
 *
 * @version 1.1.0
 * @date 2023-10-20
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

#ifndef GAMESMANONE_CORE_SOLVERS_TIER_SOLVER_REVERSE_GRAPH_H_
#define GAMESMANONE_CORE_SOLVERS_TIER_SOLVER_REVERSE_GRAPH_H_

#include <stdbool.h>  // bool
#include <stdint.h>   // int64_t

#include "core/types/gamesman_types.h"

#ifdef _OPENMP
#include <omp.h>
#endif

/**
 * @brief Reverse Position graph generated from the Position graph of the game.
 *
 * @details The reverse graph G' of a directed graph G is another directed graph
 * on the same set of vertices with all of the edges in the reverse direction.
 * That is, for each edge (v, u) in graph G, there exists an edge (u, v) in G'.
 * The reverse Position graph is represented as a 2-dimensional Position array,
 * where each array stores the parents of the Position of hash equal to its
 * index minus its tier offset.
 *
 * @note The reverse Position graph is used by the tier solver to get parent
 * positions of a given position.
 */
typedef struct ReverseGraph {
    /** 2-dimensional Position array where each array stores the parents of the
     * Position of hash equal to its index minus its tier offset. */
    PositionArray *parents_of;

#ifdef _OPENMP
    /** An array of locks for each parents_of array. */
    omp_lock_t *locks;
#endif

    /** Size (first dimension) of the parents_of array. This is typically set to
     * the number of positions in the solving tier plus the total number of
     * positions in all of its child tiers. */
    int64_t size;

    /**
     * @brief Maps tiers (relavent to the solving of current tier) to tier
     * offsets.
     *
     * @par
     * Relavent tiers include the current solving tier and all of its child
     * tiers.
     *
     * @par
     * A tier offset is the number of indices to skip into the parents_of array
     * to reach the Position of hash value 0 in that tier. Note that this
     * requires the positions within the same tier to be packed in consecutive
     * chunks in the parents_of array.
     */
    TierHashMap offset_map;
} ReverseGraph;

/**
 * @brief Initializes the reverse GRAPH.
 *
 * @param graph Reverse graph to initialize.
 * @param child_tiers Array of child tiers of the current solving tier.
 * @param this_tier Current solving tier.
 * @param GetTierSize Method to get the size of a tier in number of positions.
 * @return true on success,
 * @return false otherwise.
 */
bool ReverseGraphInit(ReverseGraph *graph, const TierArray *child_tiers,
                      Tier this_tier, int64_t (*GetTierSize)(Tier tier));

/** @brief Destroys the reverse GRAPH, freeing all allocated memory. */
void ReverseGraphDestroy(ReverseGraph *graph);

/**
 * @brief Pops out the array of parents of TIER_POSITION from reverse GRAPH.
 * The caller is responsible for destroying the PositionArray returned.
 */
PositionArray ReverseGraphPopParentsOf(ReverseGraph *graph,
                                       TierPosition tier_position);

/**
 * @brief Adds position CHILD as a child of position PARENT into the reverse
 * GRAPH.
 *
 * @param graph Reverse graph to which the new parent-child pair is added.
 * @param child Child position.
 * @param parent Parent position.
 * @return true on success,
 * @return false otherwise.
 */
bool ReverseGraphAdd(ReverseGraph *graph, TierPosition child, Position parent);

#endif  // GAMESMANONE_CORE_SOLVERS_TIER_SOLVER_REVERSE_GRAPH_H_
