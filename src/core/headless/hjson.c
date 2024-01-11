#include "core/headless/hjson.h"

#include <json-c/json_object.h>

#include "core/constants.h"
#include "core/types/gamesman_types.h"

int HeadlessJsonAddPosition(json_object *dest, ReadOnlyString formal_position) {
    json_object *position_obj = json_object_new_string(formal_position);
    if (position_obj == NULL) return kMallocFailureError;

    return json_object_object_add(dest, "position", position_obj);
}

int HeadlessJsonAddAutoGuiPosition(json_object *dest,
                                   ReadOnlyString autogui_position) {
    json_object *position_obj = json_object_new_string(autogui_position);
    if (position_obj == NULL) return kMallocFailureError;

    return json_object_object_add(dest, "autoguiPosition", position_obj);
}

int HeadlessJsonAddMove(json_object *dest, ReadOnlyString formal_move) {
    json_object *move_obj = json_object_new_string(formal_move);
    if (move_obj == NULL) return kMallocFailureError;

    return json_object_object_add(dest, "move", move_obj);
}

int HeadlessJsonAddAutoGuiMove(json_object *dest, ReadOnlyString autogui_move) {
    json_object *move_obj = json_object_new_string(autogui_move);
    if (move_obj == NULL) return kMallocFailureError;

    return json_object_object_add(dest, "autoguiMove", move_obj);
}

int HeadlessJsonAddValue(json_object *dest, Value value) {
    ReadOnlyString value_string = value < 0 ? "unsolved" : kValueStrings[value];
    json_object *value_obj = json_object_new_string(value_string);
    if (value_obj == NULL) return kMallocFailureError;

    return json_object_object_add(dest, "value", value_obj);
}

int HeadlessJsonAddRemoteness(json_object *dest, int remoteness) {
    json_object *remoteness_obj = json_object_new_int(remoteness);
    if (remoteness_obj == NULL) return kMallocFailureError;

    return json_object_object_add(dest, "remoteness", remoteness_obj);
}

int HeadlessJsonAddMovesArray(json_object *dest, json_object *moves_array_obj) {
    return json_object_object_add(dest, "moves", moves_array_obj);
}
