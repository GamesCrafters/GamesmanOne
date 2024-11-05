/**
 * @file uwapi_regular.h
 * @author Robert Shi (robertyishi@berkeley.edu)
 *         GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief The UwapiRegular type.
 * @details A UwapiRegular object defines a set of helper functions that must be
 * implemented by all regular (non-tier) games to facilitate the generation of
 * JSON responses for GamesCraftersUWAPI (Universal Web API). UWAPI is an
 * internal request-routing server framework that allows the backend solving and
 * serving systems such as GamesmanOne and GamesmanClassic to provide game rules
 * and database querying service for the GamesmanUni online game generator.
 * @link https://github.com/GamesCrafters/GamesCraftersUWAPI
 * @link https://github.com/GamesCrafters/GamesmanUni
 * @version 1.1.0
 * @date 2024-10-20
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

#ifndef GAMESMANONE_CORE_TYPES_UWAPI_UWAPI_REGULAR_H
#define GAMESMANONE_CORE_TYPES_UWAPI_UWAPI_REGULAR_H

#include "core/data_structures/cstring.h"
#include "core/types/base.h"
#include "core/types/uwapi/partmove_array.h"

/**
 * @brief A collection of helper methods that are used by regular games to
 * generate responses for GamesCraftersUWAPI (Universal Web API.)
 *
 * @note All member functions are REQUIRED unless otherwise specified.
 */
typedef struct UwapiRegular {
    /**
     * @brief Returns an array of available moves at POSITION.
     *
     * @details Assumes POSITION is legal. Results in undefined behavior
     * otherwise.
     *
     * @note This is typically set to the same function used by the regular
     * solver API.
     */
    MoveArray (*GenerateMoves)(Position position);

    /**
     * @brief Returns the resulting position after performing MOVE at POSITION.
     *
     * @note Assumes POSITION is valid and MOVE is a valid move at POSITION.
     * Passing an illegal POSITION or an illegal move at POSITION results in
     * undefined behavior.
     *
     * @note This is typically set to the same function used by the regular
     * solver API.
     */
    Position (*DoMove)(Position position, Move move);

    /**
     * @brief Returns the value of POSITION if POSITION is primitive. Returns
     * kUndecided otherwise.
     *
     * @note Assumes POSITION is valid. Results in undefined behavior otherwise.
     *
     * @note This is typically set to the same function used by the regular
     * solver API.
     */
    Value (*Primitive)(Position position);

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
     * @brief Returns the hashed Position corresponding to the given
     * FORMAL_POSITION.
     *
     * @details A formal position is a human-editable (and hopefully
     * human-readable) string that uniquely defines a position. For example, a
     * FEN notation string can be used as a formal position of a chess game.
     */
    Position (*FormalPositionToPosition)(ReadOnlyString formal_position);

    /**
     * @brief Returns the formal position as a dynamically allocated CString
     * corresponding to the given hashed POSITION. Returns \c kErrorCString if
     * an error occurred.
     *
     * @note The caller of this function is responsible for destroying the
     * CString returned.
     *
     * @details A formal position is a human-editable (and hopefully
     * human-readable) string that uniquely defines a position. For example, a
     * FEN notation string can be used as a formal position of a chess game.
     */
    CString (*PositionToFormalPosition)(Position position);

    /**
     * @brief Returns the AutoGUI position as a dynamically allocated CString
     * corresponding to the given hashed POSITION. Returns \c kErrorCString if
     * an error occurred.
     *
     * @note The caller of this function is responsible for destroying the
     * CString returned.
     *
     * @details An AutoGUI position is a position string recognized by the
     * GamesmanUni online game generator. It not only uniquely defines a
     * position, but also contains additional information such as the
     * coordinates for helper SVGs. These strings are usually not designed to be
     * human-readable and are therefore less suitable as database query inputs.
     * @link https://github.com/GamesCrafters/GamesmanUni
     */
    CString (*PositionToAutoGuiPosition)(Position position);

    /**
     * @brief Returns the formal move as a dynamically allocated CString
     * corresponding to the given MOVE at the given POSITION. Returns \c
     * kErrorCString if an error occurred.
     *
     * @note The caller of this function is responsible for destroying the
     * CString returned.
     *
     * @details A formal move is a human-redable string that uniquely defines
     * a move that is available at the given POSITION. It should be unambiguous
     * and as succinct as possible. For example, the moves at any non-primitive
     * (non-terminal) position in tic-tac-toe can be represented using digits
     * 1 thru 9, with the cells on the board labeled 1-9 in row-major order.
     *
     * @param position The position at which the move can be made.
     * @param move The move.
     *
     * @return The formal move as a dynamically allocated CString corresponding
     * to the given MOVE at the given POSITION.
     */
    CString (*MoveToFormalMove)(Position position, Move move);

    /**
     * @brief Returns the AutoGUI move as a dynamically allocated CString
     * corresponding to the given MOVE at the given POSITION if MOVE is a
     * full-move. Returns \c kNullCString if MOVE is a part-move. Returns \c
     * kErrorCString if an error occurred. Note that all moves are full moves if
     * the game does not implement multipart moves.
     *
     * @note The caller of this function is responsible for destroying the
     * CString returned.
     *
     * @details An AutoGUI move is a move string recognized by the GamesmanUni
     * online game generator. It not only unabiguously describes a move at a
     * position, but is also formatted in ways that indicate how the web
     * interface should render the move. Refer to the implementation guide
     * of GamesmanUni for formatting rules and examples.
     * @link https://github.com/GamesCrafters/GamesmanUni
     *
     * @param position The position at which the move can be made.
     * @param move The move.
     *
     * @return The AutoGUI move as a dynamically allocated CString corresponding
     * to the given MOVE at the given POSITION.
     */
    CString (*MoveToAutoGuiMove)(Position position, Move move);

    PartmoveArray (*GeneratePartmoves)(Position position);

    /**
     * @brief Returns the initial position of the current game variant.
     *
     * @note This is typically set to the same function used by the regular
     * solver API.
     */
    Position (*GetInitialPosition)(void);

    /**
     * @brief Returns a random position of the current game variant.
     *
     * @note This function is optional.
     *
     * @todo Decide if this function should only return legal positions as
     * defined by the game developer. Note that it's hard to determine whether
     * positions are actually reachable without solving and these API functions
     * are defined before the game is solved.
     */
    Position (*GetRandomLegalPosition)(void);
} UwapiRegular;

#endif  // GAMESMANONE_CORE_TYPES_UWAPI_UWAPI_REGULAR_H
