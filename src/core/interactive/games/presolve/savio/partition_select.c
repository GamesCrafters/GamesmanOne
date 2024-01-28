#include "core/interactive/games/presolve/savio/partition_select.h"

#include <stdbool.h>  // bool, true, false
#include <stddef.h>   // NULL
#include <stdio.h>    // sprintf
#include <string.h>   // strcpy

#include "core/interactive/automenu.h"
#include "core/interactive/games/presolve/savio/script_setup.h"
#include "core/savio/savio.h"
#include "core/types/gamesman_types.h"

static char items[kNumSavioPartitions][kSavioPartitionNameLengthMax + 1];
static char keys[kNumSavioPartitions][kKeyLengthMax + 1];
static HookFunctionPointer hooks[kNumSavioPartitions];

static void InitItems(void);
static void InitKeys(void);
static void InitHooks(void);

int InteractiveSavioPartitionSelect(ReadOnlyString key) {
    (void)key;  // Unused
    static ConstantReadOnlyString title = "Select a Savio Partition";
    InitItems();
    InitKeys();
    InitHooks();

    return AutoMenu(title, kNumSavioPartitions, items, keys, hooks, NULL);
}

static void InitItems(void) {
    static bool initialized = false;
    if (initialized) return;

    for (int i = 0; i < kNumSavioPartitions; ++i) {
        strcpy(items[i], kSavioPartitions[i].name);
    }
    initialized = true;
}

static void InitKeys(void) {
    static bool initialized = false;
    if (initialized) return;

    for (int i = 0; i < kNumSavioPartitions; ++i) {
        sprintf(keys[i], "%d", i);
    }
    initialized = true;
}

static void InitHooks(void) {
    static bool initialized = false;
    if (initialized) return;

    for (int i = 0; i < kNumSavioPartitions; ++i) {
        hooks[i] = &InteractiveSavioScriptSetup;
    }
    initialized = true;
}
