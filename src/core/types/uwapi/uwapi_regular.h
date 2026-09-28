/**
 * @file uwapi_regular.h
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief The UwapiRegular type.
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

#ifndef GAMESMANONE_CORE_TYPES_UWAPI_UWAPI_REGULAR_H_
#define GAMESMANONE_CORE_TYPES_UWAPI_UWAPI_REGULAR_H_

#include "core/data_structures/cstring.h"
#include "core/types/base.h"
#include "core/types/uwapi/partmove_array.h"

/**
 * @brief A collection of helper methods used by regular games to generate
 * responses for GamesCraftersUWAPI (Universal Web API).
 *
 * @details A `UwapiRegular` object defines a set of helper functions that must
 * be implemented by all regular (non-tier) games to facilitate the generation
 * of JSON responses for GamesCraftersUWAPI (Universal Web API). UWAPI is an
 * internal request-routing server framework that allows the backend solving and
 * serving systems such as GamesmanOne and GamesmanClassic to provide game rules
 * and database querying services for the GamesmanUni online game generator.
 *
 * @note All member functions are REQUIRED unless otherwise specified.
 *
 * @see
 * [GamesCraftersUWAPI](https://github.com/GamesCrafters/GamesCraftersUWAPI)
 *
 * @see [GamesmanUni](https://github.com/GamesCrafters/GamesmanUni)
 */
typedef struct UwapiRegular {
    /**
     * @brief Returns an array of available moves at `position`.
     *
     * @details Assumes `position` is legal. Results in undefined behavior
     * otherwise.
     *
     * @note This is typically set to a wrapper of the same function used by the
     * regular solver API.
     *
     * @param[in] position The position from which moves are generated.
     *
     * @returns An array of available moves at `position`.
     */
    MoveArray (*GenerateMoves)(Position position);

    /**
     * @brief Returns the resulting position after performing `move` at
     * `position`.
     *
     * @note Assumes `position` is valid and `move` is a valid move at
     * `position`. Passing an illegal position or an illegal move at `position`
     * results in undefined behavior.
     *
     * @note This is typically set to the same function used by the regular
     * solver API.
     *
     * @param[in] position The position at which the move is made.
     * @param[in] move The move to perform.
     *
     * @returns The resulting position.
     */
    Position (*DoMove)(Position position, Move move);

    /**
     * @brief Returns the value of `position` if `position` is primitive.
     *
     * @details Returns `kUndecided` otherwise.
     *
     * @note Assumes `position` is valid. Results in undefined behavior
     * otherwise.
     *
     * @note This is typically set to the same function used by the regular
     * solver API.
     *
     * @param[in] position The position to evaluate.
     *
     * @returns The value of `position` if it is primitive; otherwise,
     * `kUndecided`.
     */
    Value (*Primitive)(Position position);

    /**
     * @brief Returns whether the given `formal_position` is legal.
     *
     * @details A formal position is a human-editable (and hopefully
     * human-readable) string that uniquely defines a position. For example, a
     * [FEN
     * string](https://en.wikipedia.org/wiki/Forsyth%E2%80%93Edwards_Notation)
     * can be used as a formal position for a chess game.
     *
     * @note IMPORTANT: The security of this function is crucial as
     * `formal_position` is unsanitized user input from a UWAPI query that
     * potentially contains malicious code. If this function returns true,
     * the input is considered trusted and passed into other position
     * querying functions.
     *
     * @param[in] formal_position The formal position string to check.
     *
     * @returns `true` if the formal position is legal; otherwise, `false`.
     */
    bool (*IsLegalFormalPosition)(ReadOnlyString formal_position);

    /**
     * @brief Returns the hashed position corresponding to the given
     * `formal_position`.
     *
     * @details A formal position is a human-editable (and hopefully
     * human-readable) string that uniquely defines a position. For example, a
     * [FEN
     * string](https://en.wikipedia.org/wiki/Forsyth%E2%80%93Edwards_Notation)
     * can be used as a formal position for a chess game.
     *
     * @param[in] formal_position The formal position string.
     *
     * @returns The hashed position.
     */
    Position (*FormalPositionToPosition)(ReadOnlyString formal_position);

    /**
     * @brief Returns the formal position as a dynamically allocated `CString`
     * corresponding to the given hashed `position`.
     *
     * @details A formal position is a human-editable (and hopefully
     * human-readable) string that uniquely defines a position. For example, a
     * [FEN
     * string](https://en.wikipedia.org/wiki/Forsyth%E2%80%93Edwards_Notation)
     * can be used as a formal position for a chess game.
     *
     * @note The caller of this function is responsible for destroying the
     * `CString` returned.
     *
     * @param[in] position The hashed position.
     *
     * @returns The formal position as a dynamically allocated `CString`.
     * Returns a "null" `CString`, which can be tested using the
     * `CStringIsNull()` function, on error.
     */
    CString (*PositionToFormalPosition)(Position position);

    /**
     * @brief Returns the AutoGUI position as a dynamically allocated `CString`
     * corresponding to the given hashed `position`.
     *
     * @details An AutoGUI position is a position string recognized by the
     * GamesmanUni online game generator. It not only uniquely defines a
     * position, but also contains additional information such as the
     * coordinates for helper SVGs. These strings are usually not designed to be
     * human-readable and are therefore less suitable as database query inputs.
     *
     * @note The caller of this function is responsible for destroying the
     * `CString` returned.
     *
     * @param[in] position The hashed position.
     *
     * @returns The AutoGUI position as a dynamically allocated `CString`.
     * Returns a "null" `CString`, which can be tested using the
     * `CStringIsNull()` function, on error.
     *
     * @see https://github.com/GamesCrafters/GamesmanUni
     */
    CString (*PositionToAutoGuiPosition)(Position position);

    /**
     * @brief Returns the formal move as a dynamically allocated `CString`
     * corresponding to the given `move` at the given `position`.
     *
     * @details A formal move is a human-readable string that uniquely defines
     * a move available at the given `position`. It should be unambiguous
     * and as succinct as possible. For example, the moves at any non-primitive
     * (non-terminal) position in tic-tac-toe can be represented using digits
     * 1 through 9, with the cells on the board labeled 1-9 in row-major order.
     *
     * @note The caller of this function is responsible for destroying the
     * `CString` returned.
     *
     * @param[in] position The position at which the move can be made.
     * @param[in] move The move.
     *
     * @returns The formal move as a dynamically allocated `CString`
     * corresponding to the given `move` at the given `position`. Returns a
     * "null" `CString`, which can be tested using the `CStringIsNull()`
     * function, on error.
     */
    CString (*MoveToFormalMove)(Position position, Move move);

    /**
     * @brief Returns the AutoGUI move as a dynamically allocated `CString`
     * corresponding to the given `move` at the given `position` if `move` is a
     * single-part move.
     *
     * @details An AutoGUI move is a move string recognized by the GamesmanUni
     * online game generator. It not only unambiguously describes a move at a
     * position, but is also formatted in ways that indicate how the web
     * interface should render the move. Refer to the implementation guide
     * of GamesmanUni for formatting rules and examples.
     * Note that all moves are single-part if the game does not implement
     * multipart moves.
     *
     * @note The caller of this function is responsible for destroying the
     * `CString` returned.
     *
     * @param[in] position The position at which the move can be made.
     * @param[in] move The move.
     *
     * @returns The AutoGUI move as a dynamically allocated `CString`
     * corresponding to the given `move` at the given `position`. Returns the
     * "null" `CString`, which can be tested with the `CStringIsNull()`
     * function, if `move` is a multipart move. Returns a "null" `CString` on
     * error.
     *
     * @see https://github.com/GamesCrafters/GamesmanUni
     */
    CString (*MoveToAutoGuiMove)(Position position, Move move);

    /**
     * @brief Returns an array containing all the part-moves obtained by
     * disassembling all available full moves with at least two parts at
     * `position`.
     *
     * @details The part-moves may be returned in any order.
     *
     * @param[in] position The parent position from which the moves are
     * generated.
     *
     * @returns An array of all part-moves from `position`, whose ownership is
     * then transferred to the caller of this function.
     */
    PartmoveArray (*GeneratePartmoves)(Position position);

    /**
     * @brief Returns the initial position of the current game variant.
     *
     * @note This is typically set to the same function used by the regular
     * solver API.
     *
     * @returns The initial position.
     */
    Position (*GetInitialPosition)(void);

    /**
     * @brief Returns a random position of the current game variant.
     *
     * @note This function is optional.
     *
     * @returns A random position.
     *
     * @todo Decide if this function should only return legal positions as
     * defined by the game developer. Note that it's hard to determine whether
     * positions are actually reachable without solving and these API functions
     * are defined before the game is solved.
     */
    Position (*GetRandomLegalPosition)(void);
} UwapiRegular;

#endif  // GAMESMANONE_CORE_TYPES_UWAPI_UWAPI_REGULAR_H_
