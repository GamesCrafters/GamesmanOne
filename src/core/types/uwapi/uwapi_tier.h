/**
 * @file uwapi_tier.h
 * @author Robert Shi (robertyishi@berkeley.edu)
 *         GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Declaration of the UwapiTier type.
 * @details A UwapiTier object defines a set of helper functions that must be
 * implemented by all tier games to facilitate the generation of JSON responses
 * for GamesCraftersUWAPI (Universal Web API). UWAPI is an internal
 * request-routing server framework that allows the backend solving and serving
 * systems such as GamesmanOne and GamesmanClassic to provide game rules and
 * database querying service for the GamesmanUni online game generator.
 * @link https://github.com/GamesCrafters/GamesCraftersUWAPI
 * @link https://github.com/GamesCrafters/GamesmanUni
 *
 * @version 1.0.0
 * @date 2024-01-23
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

#ifndef GAMESMANONE_CORE_TYPES_UWAPI_UWAPI_TIER_H
#define GAMESMANONE_CORE_TYPES_UWAPI_UWAPI_TIER_H

#include <stdbool.h>  // bool

#include "core/data_structures/cstring.h"
#include "core/types/base.h"

/**
 * @brief A collection of helper methods that are used by tier games to
 * generate responses for GamesCraftersUWAPI (Universal Web API.)
 *
 * @note All member functions are REQUIRED unless otherwise specified.
 */
typedef struct UwapiTier {
    /**
     * @brief Returns an array of moves available at TIER_POSITION.
     *
     * @note Assumes TIER_POSITION is valid. Passing an invalid tier or illegal
     * position within the tier results in undefined behavior.
     *
     * @note This is typically set to the same function used by the tier
     * solver API.
     */
    MoveArray (*GenerateMoves)(TierPosition tier_position);

    /**
     * @brief Returns the resulting tier position after performing MOVE at
     * TIER_POSITION.
     *
     * @note Assumes TIER_POSITION is valid and MOVE is a valid move at
     * TIER_POSITION. Passing an invalid tier, an illegal position within the
     * tier, or an illegal move results in undefined behavior.
     *
     * @note This is typically set to the same function used by the tier
     * solver API.
     */
    TierPosition (*DoMove)(TierPosition tier_position, Move move);

    /**
     * @brief Returns whether the given FORMAL_POSITION is legal.
     *
     * @details A formal position is a human-editable (and hopefully
     * human-readable) string that uniquely defines a position. For example, a
     * FEN notation string can be used as a formal position of a chess game.
     * @link https://en.wikipedia.org/wiki/Forsyth%E2%80%93Edwards_Notation
     *
     * @note IMPORTANT: The security of this function is crucial as
     * FORMAL_POSITION is unsanitized user input from a UWAPI query that
     * potentially contains malicious code. If this function returns true,
     * the input is considered trusted and passed into other position
     * querying functions.
     */
    bool (*IsLegalFormalPosition)(ReadOnlyString formal_position);

    /**
     * @brief Returns the hashed tier position corresponding to the given
     * FORMAL_POSITION.
     *
     * @details A formal position is a human-editable (and hopefully
     * human-readable) string that uniquely defines a position. For example, a
     * FEN notation string can be used as a formal position of a chess game.
     */
    TierPosition (*FormalPositionToTierPosition)(
        ReadOnlyString formal_position);

    /**
     * @brief Returns the formal position as a dynamically allocated CString
     * corresponding to the given hashed TIER_POSITION. The caller of this
     * function is responsible for destroying the CString returned.
     *
     * @details A formal position is a human-editable (and hopefully
     * human-readable) string that uniquely defines a position. For example, a
     * FEN notation string can be used as a formal position of a chess game.
     */
    CString (*TierPositionToFormalPosition)(TierPosition tier_position);

    /**
     * @brief Returns the AutoGUI position as a dynamically allocated CString
     * corresponding to the given hashed TIER_POSITION. The caller of this
     * function is responsible for destroying the CString returned.
     *
     * @details An AutoGUI position is a position string recognized by the
     * GamesmanUni online game generator. It not only uniquely defines a
     * position, but also contains additional information such as the
     * coordinates for helper SVGs. These strings are usually not designed to be
     * human-readable and are therefore less suitable as database query inputs.
     * @link https://github.com/GamesCrafters/GamesmanUni
     */
    CString (*TierPositionToAutoGuiPosition)(TierPosition tier_position);

    /**
     * @brief Returns the formal move as a dynamically allocated CString
     * corresponding to the given MOVE at the given TIER_POSITION. The caller of
     * this function is responsible for destroying the CString returned.
     *
     * @details A formal move is a human-redable string that uniquely defines
     * a move that is available at the given TIER_POSITION. It should be
     * unambiguous and as succinct as possible. For example, the moves at any
     * non-primitive (non-terminal) tier position in tic-tac-toe can be
     * represented using digits 1 thru 9, with the cells on the board labeled
     * 1-9 in row-major order.
     *
     * @param tier_position The tier position at which the move can be made.
     * @param move The move.
     *
     * @return The formal move as a dynamically allocated CString corresponding
     * to the given MOVE at the given TIER_POSITION.
     */
    CString (*MoveToFormalMove)(TierPosition tier_position, Move move);

    /**
     * @brief Returns the AutoGUI move as a dynamically allocated CString
     * corresponding to the given MOVE at the given TIER_POSITION. The caller of
     * this function is responsible for destroying the CString returned.
     *
     * @details An AutoGUI move is a move string recognized by the GamesmanUni
     * online game generator. It not only unabiguously describes a move at a
     * position, but is also formatted in ways that indicate how the web
     * interface should render the move. Refer to the implementation guide
     * of GamesmanUni for formatting rules and examples.
     * @link https://github.com/GamesCrafters/GamesmanUni
     *
     * @param tier_position The tier position at which the move can be made.
     * @param move The move.
     *
     * @return The AutoGUI move as a dynamically allocated CString corresponding
     * to the given MOVE at the given TIER_POSITION.
     */
    CString (*MoveToAutoGuiMove)(TierPosition tier_position, Move move);

    /**
     * @brief Returns the initial tier of the current game variant.
     *
     * @note This is typically set to the same function used by the tier
     * solver API.
     */
    Tier (*GetInitialTier)(void);

    /**
     * @brief Returns the initial position (within the initial tier) of the
     * current game variant.
     *
     * @note This is typically set to the same function used by the tier
     * solver API.
     */
    Position (*GetInitialPosition)(void);

    /**
     * @brief Returns a random tier position of the current game variant.
     *
     * @note This function is optional.
     *
     * @todo Decide if this function should only return legal tier positions as
     * defined by the game developer. Note that it's hard to determine whether
     * positions are actually reachable without solving and these API functions
     * are defined before the game is solved.
     */
    TierPosition (*GetRandomLegalTierPosition)(void);
} UwapiTier;

#endif  // GAMESMANONE_CORE_TYPES_UWAPI_UWAPI_TIER_H
