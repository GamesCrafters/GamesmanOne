#ifndef GAMESMANONE_CORE_HEADLESS_HJSON_H_
#define GAMESMANONE_CORE_HEADLESS_HJSON_H_

#include <json-c/json_object.h>

#include "core/types/gamesman_types.h"

int HeadlessJsonAddPosition(json_object *dest, ReadOnlyString formal_position);
int HeadlessJsonAddAutoGuiPosition(json_object *dest,
                                   ReadOnlyString autogui_position);
int HeadlessJsonAddMove(json_object *dest, ReadOnlyString formal_move);
int HeadlessJsonAddAutoGuiMove(json_object *dest, ReadOnlyString autogui_move);
int HeadlessJsonAddValue(json_object *dest, Value value);
int HeadlessJsonAddRemoteness(json_object *dest, int remoteness);
int HeadlessJsonAddMovesArray(json_object *dest, json_object *moves_array_obj);
int HeadlessJsonAddError(json_object *dest, ReadOnlyString message);

#endif  // GAMESMANONE_CORE_HEADLESS_HJSON_H_
