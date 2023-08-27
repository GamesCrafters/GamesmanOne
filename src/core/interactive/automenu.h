#ifndef GAMESMANEXPERIMENT_CORE_INTERACTIVE_AUTOMENU_H_
#define GAMESMANEXPERIMENT_CORE_INTERACTIVE_AUTOMENU_H_

#include <stddef.h>  // size_t

extern const size_t kKeyLengthMax;

typedef void (*HookFunctionPointer)(const char *key);

// IMPORTANT: keys "b" and "q" are reserved for back and quit.
// Do NOT use them as custom AutoMenu keys. Otherwise, custom keys
// will be overridden by default behaviors.
void AutoMenu(const char *title, int num_items, const char *const *items,
              const char *const *keys, const HookFunctionPointer *hooks);

#endif  // GAMESMANEXPERIMENT_CORE_INTERACTIVE_AUTOMENU_H_
