/**
 * @file hjson.c
 * @author Robert Shi (robertyishi@berkeley.edu)
 *         GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Json parser helper method collection for headless mode.
 * @version 1.1.0
 * @date 2024-10-21
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

#include "core/headless/hjson.h"

#include <json-c/json_object.h>

#include "core/constants.h"
#include "core/types/gamesman_types.h"

static int AddStringHelper(json_object *dest, ReadOnlyString key,
                           ReadOnlyString value) {
    json_object *position_obj = json_object_new_string(value);
    if (position_obj == NULL) return kMallocFailureError;

    return json_object_object_add(dest, key, position_obj);
}

int HeadlessJsonAddPosition(json_object *dest, ReadOnlyString formal_position) {
    return AddStringHelper(dest, "position", formal_position);
}

int HeadlessJsonAddAutoGuiPosition(json_object *dest,
                                   ReadOnlyString autogui_position) {
    return AddStringHelper(dest, "autoguiPosition", autogui_position);
}

int HeadlessJsonAddMove(json_object *dest, ReadOnlyString formal_move) {
    return AddStringHelper(dest, "move", formal_move);
}

int HeadlessJsonAddAutoGuiMove(json_object *dest, ReadOnlyString autogui_move) {
    return AddStringHelper(dest, "autoguiMove", autogui_move);
}

int HeadlessJsonAddFrom(json_object *dest, ReadOnlyString from) {
    return AddStringHelper(dest, "from", from);
}

int HeadlessJsonAddTo(json_object *dest, ReadOnlyString to) {
    return AddStringHelper(dest, "to", to);
}

int HeadlessJsonAddFull(json_object *dest, ReadOnlyString full) {
    return AddStringHelper(dest, "full", full);
}

int HeadlessJsonAddValue(json_object *dest, Value value) {
    ReadOnlyString value_string = value < 0 ? "unsolved" : kValueStrings[value];

    return AddStringHelper(dest, "positionValue", value_string);
}

int HeadlessJsonAddRemoteness(json_object *dest, int remoteness) {
    json_object *remoteness_obj = json_object_new_int(remoteness);
    if (remoteness_obj == NULL) return kMallocFailureError;

    return json_object_object_add(dest, "remoteness", remoteness_obj);
}

int HeadlessJsonAddMovesArray(json_object *dest, json_object *moves_array_obj) {
    return json_object_object_add(dest, "moves", moves_array_obj);
}

int HeadlessJsonAddPartmovesArray(json_object *dest,
                                  json_object *partmoves_array_obj) {
    return json_object_object_add(dest, "partMoves", partmoves_array_obj);
}
