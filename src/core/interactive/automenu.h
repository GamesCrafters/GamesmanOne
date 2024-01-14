#ifndef GAMESMANONE_CORE_INTERACTIVE_AUTOMENU_H_
#define GAMESMANONE_CORE_INTERACTIVE_AUTOMENU_H_

#include <stddef.h>  // size_t

#include "core/types/gamesman_types.h"

extern const size_t kKeyLengthMax;

typedef void (*HookFunctionPointer)(ReadOnlyString key);

// IMPORTANT: keys "b" and "q" are reserved for back and quit.
// Do NOT use them as custom AutoMenu keys. Otherwise, custom keys
// will be overridden by default behaviors.
void AutoMenu(ReadOnlyString title, int num_items,
              ConstantReadOnlyString *items, ConstantReadOnlyString *keys,
              const HookFunctionPointer *hooks, void (*Update)(void));

#endif  // GAMESMANONE_CORE_INTERACTIVE_AUTOMENU_H_
